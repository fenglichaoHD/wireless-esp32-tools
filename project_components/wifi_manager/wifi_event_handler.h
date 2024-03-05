#ifndef WIFI_EVENT_HANDLER_H_GUARD
#define WIFI_EVENT_HANDLER_H_GUARD

#include <stdint.h>
#include <esp_event_base.h>
#include <esp_wifi_types.h>

void ip_event_handler(void *handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

void wifi_event_handler(void *handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);


typedef void (*wifi_event_scan_done_cb)(uint16_t ap_count, wifi_ap_record_t *aps_info);
int wifi_event_trigger_scan(uint8_t channel, wifi_event_scan_done_cb cb, uint16_t number, wifi_ap_record_t *aps);
void wifi_event_set_disco_handler(void (*disconn_handler)(void));

/**
 * @brief
 * @param connect_status 0: SUCCESS, OTHER: ERROR
 */
typedef void (*wifi_event_connect_done_cb)(void *arg, wifi_event_sta_connected_t *event);
int wifi_event_trigger_connect(uint8_t attempt, wifi_event_connect_done_cb cb, void *arg);


#endif //WIFI_EVENT_HANDLER_H_GUARD