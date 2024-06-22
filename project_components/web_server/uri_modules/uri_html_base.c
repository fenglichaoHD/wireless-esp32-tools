#include "web_uri_module.h"

#include <esp_http_server.h>
#include <esp_log.h>

#define TAG __FILE_NAME__

#define URI_ROOT          0x0000002F /* /    */
#define URI_WS_DOT        0x2E73772F /* /ws. */

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
