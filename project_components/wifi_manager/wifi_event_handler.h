#ifndef WIFI_EVENT_HANDLER_H_GUARD
#define WIFI_EVENT_HANDLER_H_GUARD

#include <stdint.h>
#include <esp_event_base.h>
#include <esp_wifi_types.h>

void ip_event_handler(void *handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

void wifi_event_handler(void *handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

typedef void (*wifi_event_scan_done_cb)(uint16_t ap_count, wifi_ap_record_t *aps_info);

int wifi_event_trigger_scan(uint8_t channel, wifi_event_scan_done_cb cb, uint16_t number, wifi_ap_record_t *aps);



#endif //WIFI_EVENT_HANDLER_H_GUARD