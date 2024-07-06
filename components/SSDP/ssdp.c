#include "ssdp.h"

#include <stdio.h>

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "memory_pool.h"
#include "wt_nvs.h"

#define TAG "SSDP"

#define SSDP_PORT 1900
#define SSDP_MULTICAST_TTL 4
#define SSDP_UUID_BASE "9079d7e9-92c6-45b3-aa28-77cdcc"
#define SSDP_MULTICAST_ADDR "239.255.255.250"
#define HEADER_SEARCH "M-SEARCH"
#define SSDP_DEVICE_TYPE "urn:schemas-upnp-org:device:Basic:1"
#define SSDP_MODEL_NAME "wireless-proxy"
#define SSDP_DEFAULT_FRIENDLY_NAME "允斯调试器"
#define SSDP_MANUFACTURER "允斯工作室"

#ifndef SSDP_MODEL_URL
#define SSDP_MODEL_URL "https://yunsi.studio/"
#endif

static struct ssdp_ctx_t {
	TaskHandle_t ssdp_service_task;
	ip4_addr_t ip;
	ip4_addr_t gw;
	uint8_t uuid_end[3]; /* actually use MAC[3:5] */
	char friendly_name[32];
} ssdp_ctx;

static volatile uint8_t ssdp_running = false;
static int ssdp_socket = -1;

static void ssdp_service_task(void *pvParameters);
static int ssdp_handle_data(int sock, in_addr_t remote_addr, uint16_t remote_port,
                            char *buf, int len);
static int ssdp_get_friendly_name();

static int create_ssdp_socket(int *sock)
{
	int err;

	*sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (*sock < 0) {
		ESP_LOGE(TAG, "Failed to create socket. Error %d", errno);
		return -1;
	}

	int opt = 1;
	if (setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
		ESP_LOGE(TAG, "setsockopt SO_REUSEADDR failed %d\n", errno);
		goto err;
	}

	struct sockaddr_in addr = {
		.sin_family      = AF_INET,
		.sin_port        = htons(SSDP_PORT),
		.sin_addr.s_addr = htonl(INADDR_ANY)
	};
	err = bind(*sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	if (err < 0) {
		ESP_LOGE(TAG, "Failed to bind socket. Error %d", errno);
		goto err;
	}

	/* Multicast TTL (independent to normal interface TTL)
	 * should be set to small value like 2~4 */
	opt = SSDP_MULTICAST_TTL;
	if (setsockopt(*sock, IPPROTO_IP, IP_MULTICAST_TTL, &opt, sizeof(opt)) < 0) {
		ESP_LOGE(TAG, "Failed to set IP_MULTICAST_TTL. Error %d", errno);
		err = errno;
		goto err;
	}

	/* Receive multicast packet sent from this device,
	 * useful when other service need to be aware of multicast notification */
	opt = 0;
	if (setsockopt(*sock, IPPROTO_IP, IP_MULTICAST_LOOP, &opt, sizeof(opt)) < 0) {
		ESP_LOGE(TAG, "Failed to unset IP_MULTICAST_LOOP. Error %d", errno);
		err = errno;
		goto err;
	}

	struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = inet_addr(SSDP_MULTICAST_ADDR);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(*sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
		perror("Adding to multicast group failed");
		goto err;
	}

	return 0;

err:
	close(*sock);
	return -1;
}

static int ssdp_send_notify(int sock, char *buf, int buf_len)
{
	int msg_len = snprintf(
		buf, buf_len,
		"NOTIFY * HTTP/1.1\r\n"
		"HOST: 239.255.255.250:1900\r\n"
		"NTS: ssdp:alive\r\n"
		"NT: upnp:rootdevice\r\n"
		"CACHE-CONTROL: max-age=3600\r\n"
		"SERVER: FreeRTOS/v10 UPnP/1.1 product/version\r\n"
		"USN: uuid:"SSDP_UUID_BASE"%02x%02x%02x::upnp:rootdevice\r\n"
		"LOCATION: http://%s:%u/SSDP.xml\r\n",
		ssdp_ctx.uuid_end[0], ssdp_ctx.uuid_end[1], ssdp_ctx.uuid_end[2],
		ip4addr_ntoa((const ip4_addr_t *)&ssdp_ctx.ip), 80
	);
	buf[msg_len] = '\0';
	struct sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(SSDP_PORT);
	dest_addr.sin_addr.s_addr = inet_addr(SSDP_MULTICAST_ADDR);

	int err = sendto(sock, buf, msg_len, 0,
	                 (struct sockaddr *)&dest_addr, sizeof(dest_addr));
	if (err < 0) {
		ESP_LOGE(TAG, "IPV4 sendto failed. errno: %d", errno);
		return errno;
	}

	return 0;
}

static int ssdp_send_response(int sock, in_addr_t remote_addr, uint16_t remote_port, char *buf, int buf_len)
{
	int msg_len = snprintf(
		buf, buf_len,
		"HTTP/1.1 200 OK\r\n"
		"EXT:\r\n"
		"ST: ""upnp:rootdevice""\r\n"
		"CACHE-CONTROL: max-age=3600\r\n"
		"SERVER: FreeRTOS/v10 UPnP/1.1 product/version\r\n"
		"USN: uuid:"SSDP_UUID_BASE"%02x%02x%02x" /* MAC=UUID */
		"::""upnp:rootdevice""\r\n"
		"LOCATION: http://%s:%u/SSDP.xml\r\n" /* ip:port */
		"\r\n",
		ssdp_ctx.uuid_end[0], ssdp_ctx.uuid_end[1], ssdp_ctx.uuid_end[2], /* MAC=UUID */
		ip4addr_ntoa((const ip4_addr_t *)&ssdp_ctx.ip), 80 /* ip:port */
	);

	buf[msg_len] = '\0';

	struct sockaddr_in dest_addr;
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = remote_port;
	dest_addr.sin_addr.s_addr = remote_addr;

	int err = sendto(sock, buf, msg_len, 0,
	                 (struct sockaddr *)&dest_addr, sizeof(dest_addr));
	if (err < 0) {
		ESP_LOGE(TAG, "IPV4 sendto failed. errno: %d", errno);
		return errno;
	}

	return 0;
}

static int ssdp_handle_data(int sock, in_addr_t remote_addr, uint16_t remote_port,
                            char *buf, int len)
{
	/* on M-SEARCH: send RESPONSE anyway (even if this device doesn't match search type)
	 * on NOTIFY  : ignore
	 * on RESPONSE: HTTP/1.1 200 OK : ignore
	 * */
	if (memcmp(buf, HEADER_SEARCH, strlen(HEADER_SEARCH)) != 0) {
		return 0;
	}

	return ssdp_send_response(sock, remote_addr, remote_port, buf, len);
}

static void ssdp_service_handle_socket()
{
	char *buf = memory_pool_get(portMAX_DELAY);

	while (1) {
		struct sockaddr_storage remote_addr;
		socklen_t socklen = sizeof(remote_addr);
		int len = recvfrom(ssdp_socket, buf, memory_pool_get_buf_size(), 0,
		                   (struct sockaddr *)&remote_addr, &socklen);
		if (len < 0) {
			ESP_LOGE(TAG, "multicast recvfrom failed: errno %d", errno);
			break;
		} else if (len == 0) {
			continue;
		}
		buf[len] = '\0';

		uint16_t remote_port = ((struct sockaddr_in *)&remote_addr)->sin_port;
		in_addr_t remote_ip = ((struct sockaddr_in *)&remote_addr)->sin_addr.s_addr;
		ssdp_handle_data(ssdp_socket, remote_ip, remote_port,
		                 buf, memory_pool_get_buf_size());
	}

	memory_pool_put(buf);
}

static void ssdp_service_task(void *pvParameters)
{
	ESP_LOGD(TAG, "ssdp_service_task()");
	ssdp_running = true;

	int err;
	while (ssdp_running) {
		err = create_ssdp_socket(&ssdp_socket);
		if (err) {
			ssdp_socket = -1;
			ESP_LOGE(TAG, "Failed to create multicast socket");
			vTaskDelay(pdMS_TO_TICKS(1000));
			continue;
		}

		ssdp_service_handle_socket();

		ESP_LOGE(TAG, "Shutting down socket and restarting...");
		shutdown(ssdp_socket, 0);
		close(ssdp_socket);
		ssdp_socket = -1;
	}

	vTaskDelete(NULL);
}

/*
 * Global Functions
 */
esp_err_t ssdp_init()
{
	ssdp_ctx.ssdp_service_task = NULL;
	ssdp_socket = -1;
	ssdp_running = false;
	ssdp_ctx.ip.addr = 0;
	ssdp_ctx.gw.addr = 0;

	uint8_t mac[6];
	esp_base_mac_addr_get(mac);
	for (int i = 0; i < 3; ++i) {
		/* use MAC last 3 bytes, as the first 3 bytes do not change */
		ssdp_ctx.uuid_end[i] = mac[i + 3];
	}

	/* get name from nvs, if failed: use default */
	if (ssdp_get_friendly_name())
		ssdp_set_friendly_name(SSDP_DEFAULT_FRIENDLY_NAME);
	return ESP_OK;
}

esp_err_t ssdp_start()
{
	if (ssdp_socket != -1 || ssdp_ctx.ssdp_service_task) {
		ESP_LOGE(TAG, "SSDP already started");
		return ESP_ERR_INVALID_STATE;
	}

	/* get a default ip from available netif */
	esp_err_t err;
	esp_netif_ip_info_t ip_info = {0};
	esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
	if (netif == NULL) {
		netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
	}
	if (netif == NULL) {
		netif = esp_netif_get_handle_from_ifkey("ETH_DEF");
	}

	err = esp_netif_get_ip_info(netif, &ip_info);
	if (err != ESP_OK) {
		/* fallback to 0.0.0.0 default IP */
		ssdp_ctx.ip.addr = 0;
		ssdp_ctx.gw.addr = 0;
	} else {
		ssdp_ctx.ip.addr = ip_info.ip.addr;
		ssdp_ctx.gw.addr = ip_info.gw.addr;
	}

	BaseType_t res = xTaskCreatePinnedToCore(
		ssdp_service_task, "ssdp task", 4096, NULL,
		tskIDLE_PRIORITY + 1, &ssdp_ctx.ssdp_service_task,
		0);
	if (res != pdPASS || ssdp_ctx.ssdp_service_task == NULL) {
		ESP_LOGE(TAG, "Failed to create task");
		err = ESP_FAIL;
	}
	return err;
}

void ssdp_stop()
{
	ESP_LOGD(TAG, "Stopping SSDP");
	if (ssdp_socket != -1) {
		shutdown(ssdp_socket, 0);
		close(ssdp_socket);
		ssdp_socket = -1;
	}
	ssdp_running = false;
	ssdp_ctx.ssdp_service_task = NULL;
}


#define WT_NVS_KEY_SSDP_FRIENDLY_NAME 0x80
#define WT_NVS_NS_NETWORK "net"

int ssdp_get_friendly_name()
{
	return wt_nvs_get_once(WT_NVS_NS_NETWORK, WT_NVS_KEY_SSDP_FRIENDLY_NAME,
	                       ssdp_ctx.friendly_name, sizeof(ssdp_ctx.friendly_name));
}

int ssdp_set_friendly_name(const char *name)
{
	snprintf(ssdp_ctx.friendly_name, sizeof(ssdp_ctx.friendly_name),
	         "%s", name);
	ssdp_ctx.friendly_name[sizeof(ssdp_ctx.friendly_name) - 1] = '\0';
	return 0;
}

int ssdp_set_ip_gw(const uint32_t *ip, const uint32_t *gw)
{
	ssdp_ctx.ip.addr = *ip;
	ssdp_ctx.gw.addr = *gw;

	char *buf = memory_pool_get(portMAX_DELAY);

	/* send a ssdp notification to the new connected network */
	ssdp_send_notify(ssdp_socket, buf, memory_pool_get_buf_size());
	memory_pool_put(buf);
	return 0;
}

int ssdp_get_schema_str(char *buf, uint32_t buf_len)
{
	int write_size = snprintf(
		buf, buf_len - 1,
		"<?xml version=\"1.0\"?>"
		"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
		"<specVersion>"
		"<major>1</major>"
		"<minor>0</minor>"
		"</specVersion>"
		"<URLBase>http://%s:%u</URLBase>" /* ip, port */
		"<device>"
		"<friendlyName>%s</friendlyName>" /* friendly_name */
		"<deviceType>"SSDP_DEVICE_TYPE"</deviceType>"
		"<presentationURL>/</presentationURL>"
		"<serialNumber>0000</serialNumber>"
		"<modelName>"SSDP_MODEL_NAME"</modelName>"
		"<modelNumber>0000</modelNumber>"
		"<modelURL>"SSDP_MODEL_URL"</modelURL>"
		"<manufacturer>"SSDP_MANUFACTURER"</manufacturer>"
		"<manufacturerURL>https://yunsi.studio</manufacturerURL>"
		"</device>"
		"</root>\r\n"
		"\r\n",
		ip4addr_ntoa((const ip4_addr_t *)&ssdp_ctx.ip), 80, /* ip, port */
		ssdp_ctx.friendly_name /* friendly_name */
	);

	buf[write_size] = '\0';
	return write_size < buf_len - 1;
}