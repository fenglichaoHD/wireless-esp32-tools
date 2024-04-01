#include "web_server.h"
#include "web_uri_module.h"

#include <esp_http_server.h>
#include <esp_event.h>

#include <esp_system.h>
#include <esp_log.h>
#include <sys/unistd.h>

#define TAG "web server"

#define MAKE_U32(b0, b1, b2, b3) ((b0) | (b1) << 8 | (b2) << 16 | (b3)  << 24)
#define URI_WS_STR MAKE_U32('/', 'w', 's', '\0')
#define URI_API_STR MAKE_U32('/', 'a', 'p', 'i')

static httpd_handle_t http_server = NULL;

/**
 * 3 types of ref:
 *   - 1. "/"
 *   - 2. "/entry"
 *   - 3. "/dir/"
 * 4 types of target
 *   - 1. "/.*"
 *   - 2. "/entry"
 *   - 2. "/dir/"
 *   - 3. "/dir/.*"
 * */
static bool uri_match(const char *reference_uri, const char *uri_to_match, size_t match_upto)
{
	const uint32_t *ptr32_ref = (const uint32_t *) reference_uri;
	const uint32_t *ptr32_target = (const uint32_t *) uri_to_match;
	size_t ref_length;

	/* optimized for /ws quick access */
	if (likely(match_upto == 3)) {
		if (likely(*ptr32_ref == URI_WS_STR)) {
			ESP_LOGI(TAG, "cmp ws");
			return *ptr32_target == *ptr32_ref;
		}
	}

	/* ref should be shorter than target */
	ref_length = strlen(reference_uri);
	if (ref_length > match_upto) {
		ESP_LOGI(TAG, "no match length ref %s t: %s", reference_uri, uri_to_match);
		return false;
	}

	// strict matching "/entry" for ref = "/entry"
	if (ref_length == match_upto) {
		if (memcmp(reference_uri, uri_to_match, ref_length) == 0) {
			ESP_LOGI(TAG, "strict match %s", reference_uri);
			return true;
		}
	}

	// if ref = /dir/ targets are /dir/ and /dir/*
	if (reference_uri[ref_length - 1] == '/' && ref_length > 1) {
		if (memcmp(reference_uri, uri_to_match, ref_length) == 0) {
			ESP_LOGI(TAG, "match /dir/");
			return true;
		} else {
			ESP_LOGI(TAG, "no match /dir/");
			return false;
		}
	}

	/* Fall back to root pages */
	// match /* when ref if /
	if (reference_uri[ref_length - 1] == '/') {
		ESP_LOGI(TAG, "match /");
		return true;
	}

	ESP_LOGI(TAG, "fall back false %s t: %s", reference_uri, uri_to_match);
	return false;
}

static int8_t opened_socket = 0;

static esp_err_t web_server_on_open(httpd_handle_t hd, int sockfd)
{
	opened_socket++;
	ESP_LOGI(TAG, "%d open, now: %d", sockfd, opened_socket);
	return ESP_OK;
}

static void web_server_on_close(httpd_handle_t hd, int sockfd)
{
	opened_socket--;
	ESP_LOGI(TAG, "%d closed, now: %d", sockfd, opened_socket);
	close(sockfd);
}

void start_webserver(void)
{
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	int err;

	config.enable_so_linger = true;
	config.linger_timeout = 0;
	config.lru_purge_enable = true;
	config.uri_match_fn = uri_match;
	config.open_fn = web_server_on_open;
	config.close_fn = web_server_on_close;
	config.keep_alive_enable = 1;
	config.keep_alive_count = 1;

	ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
	if ((err = httpd_start(&http_server, &config)) != ESP_OK) {
		ESP_LOGE(TAG, "Error starting server!");
		ESP_ERROR_CHECK(err);
	}

	ESP_LOGI(TAG, "Registering URI handlers");
	uri_module_init(http_server);
}

static esp_err_t stop_webserver(httpd_handle_t server)
{
	// Stop the httpd server
	return httpd_stop(server);
}

int server_is_running()
{
	return http_server != NULL;
}
