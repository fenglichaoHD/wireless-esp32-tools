#ifndef SSDP_HEADER_GUARD
#define SSDP_HEADER_GUARD
#include <esp_err.h>
#include <stdint.h>

esp_err_t ssdp_init();
esp_err_t ssdp_start();
void ssdp_stop();

int ssdp_set_friendly_name(const char *name);
int ssdp_set_ip_gw(const uint32_t *ip, const uint32_t *gw);
int ssdp_get_schema_str(char *buf, uint32_t buf_len);


#endif /* SSDP_HEADER_GUARD */