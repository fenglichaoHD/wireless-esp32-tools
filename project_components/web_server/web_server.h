#ifndef WEB_SERVER_H_GUARD
#define WEB_SERVER_H_GUARD

#include <stdint.h>
#include <esp_event_base.h>

void disconnect_handler(void* arg, esp_event_base_t event_base,
                        int32_t event_id, void* event_data);
void connect_handler(void* arg, esp_event_base_t event_base,
                     int32_t event_id, void* event_data);

void start_webserver(void);
int server_is_running();


#endif //WEB_SERVER_H_GUARD