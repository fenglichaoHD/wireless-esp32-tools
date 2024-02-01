#ifndef WEB_URI_MODULE_H_GUARD
#define WEB_URI_MODULE_H_GUARD

#include <esp_http_server.h>

typedef int (*uri_module_func)(const httpd_uri_t **uri);

typedef struct uri_module_t {
	uri_module_func init;
	uri_module_func exit;
} uri_module_t;

int uri_module_add(uri_module_func init, uri_module_func exit);

/**
 * @brief Register a uri module that will be init with PRI(priority) order.
 */
#define WEB_URI_MODULE_REGISTER(PRI, INIT, EXIT) \
  __attribute__((used, constructor(PRI))) void cons_ ## INIT(); \
  void cons_ ## INIT() { uri_module_add(INIT, EXIT); }

int uri_module_init(httpd_handle_t server);
int uri_module_exit(httpd_handle_t server);

#endif //WEB_URI_MODULE_H_GUARD