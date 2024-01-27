#ifndef WIFI_EVENT_HANDLER_H_GUARD
#define WIFI_EVENT_HANDLER_H_GUARD

#include <stdint.h>
#include <esp_event_base.h>

void ip_event_handler(void *handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

void wifi_event_handler(void *handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);


#endif //WIFI_EVENT_HANDLER_H_GUARD