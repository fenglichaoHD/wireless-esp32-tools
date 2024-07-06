#include "web_uri_module.h"
#include "ssdp.h"

#include <esp_http_server.h>
#include <esp_log.h>
#include "memory_pool.h"

#define TAG __FILE_NAME__

#define MAKE_U32(b0, b1, b2, b3) ((b0) | (b1) << 8 | (b2) << 16 | (b3)  << 24)
#define URI_ROOT MAKE_U32  ('/', '\0', '\0', '\0')/* /    */
#define URI_WS_DOT MAKE_U32('/', 'w', 's', '.') /* /ws. */
#define URI_SSDP MAKE_U32  ('/', 'S', 'S', 'D')   /* /SSD */

#define HTML_INDEX "index_html_gz"
#define JS_WS_SHARED "ws_sharedworker_js_gz"

#define SEND_FILE(req, filename) do {                                        \
extern const unsigned char filename##_start[] asm("_binary_" filename  "_start"); \
extern const unsigned char filename##_end[] asm("_binary_" filename  "_end");     \
const ssize_t file_size = filename##_end - filename##_start;                          \
httpd_resp_send(req, (const char *)filename##_start, file_size);                \
} while(0)

static esp_err_t html_base_get_handler(httpd_req_t *req)
{
	/* this "hash" actually use the first 4 chars as an int32_t "hash" */
	const int *URI_HASH = (const int *)req->uri;

	httpd_resp_set_hdr(req, "Connection", "close");

	if (*URI_HASH == URI_WS_DOT) {
		httpd_resp_set_type(req, "text/javascript");
		httpd_resp_set_hdr(req, "Content-encoding", "gzip");
		SEND_FILE(req, JS_WS_SHARED);
	} else if (*URI_HASH == URI_SSDP) {
		httpd_resp_set_type(req, "text/xml");
		char *buf = memory_pool_get(portMAX_DELAY);
		ssdp_get_schema_str(buf, memory_pool_get_buf_size());
		httpd_resp_send(req, buf, strlen(buf));
	} else {
		httpd_resp_set_type(req, "text/html");
		httpd_resp_set_hdr(req, "Content-encoding", "gzip");
		SEND_FILE(req, HTML_INDEX);
	}

	return ESP_OK;
}

/**
 * REGISTER MODULE
 * */

static const httpd_uri_t hello = {
	.uri       = "/",
	.method    = HTTP_GET,
	.handler   = html_base_get_handler,
};

static int URI_HTML_BASE_INIT(const httpd_uri_t **uri_conf) {
	*uri_conf = &hello;
	return 0;
}

static int URI_HTML_BASE_EXIT(const httpd_uri_t **uri_conf) {
	*uri_conf = &hello;
	return 0;
}

WEB_URI_MODULE_REGISTER(0x90, URI_HTML_BASE_INIT, URI_HTML_BASE_EXIT);
