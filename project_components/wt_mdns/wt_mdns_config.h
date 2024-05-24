#ifndef WT_MDNS_CONFIG_H_GUARD
#define WT_MDNS_CONFIG_H_GUARD

#define MDSN_DEFAULT_HOSTNAME "dap" // + serial number (4 char)
#define MDSN_INSTANCE_DESC "ESP32 无线透传"

void wt_mdns_init();

int wt_mdns_set_hostname(const char* hostname);


#endif //WT_MDNS_CONFIG_H_GUARD