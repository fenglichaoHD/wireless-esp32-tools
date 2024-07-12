#ifndef WIFI_STORAGE_PRIV_H_GUARD
#define WIFI_STORAGE_PRIV_H_GUARD

#define WIFI_MAX_AP_CRED_RECORD 1

typedef struct w_cache_t {
	uint32_t ap_bitmap;
	wifi_credential_t ap_creds[WIFI_MAX_AP_CRED_RECORD];
} w_cache_t;

typedef enum wt_wifi_key_enum {
	KEY_WIFI_RESERVED = 0x000,
/* WIFI */
	KEY_WIFI_AP_SSID = 0x1,
	KEY_WIFI_AP_PASSWORD = 0x02,

	/* TODO: should have 1 for each AP */
	KEY_WIFI_STA_USE_STATIC = 0x03, /* bit[0:31]=[IP, MASK, GATEWAY, DNS] */
	KEY_WIFI_STA_STATIC_IP = 0x04, /* 4B */
	KEY_WIFI_STA_STATIC_MASK = 0x05, /* 4B */
	KEY_WIFI_STA_STATIC_GATEWAY = 0x06, /* 4B */
	KEY_WIFI_STA_STATIC_DNS = 0x07, /* 4B */

	/* STA information */
	KEY_WIFI_STA_LAST_AP_CRED = 0x08, /*!< ssid[32] + password[64] */
	KEY_WIFI_STA_AP_BITMAP = 0x09, /* 32 bit */

	KEY_UNUSED_100      = 0x0100,
	KEY_WIFI_APSTA_MODE = 0x0101, /* 1B */
} wt_wifi_key;

#endif //WIFI_STORAGE_PRIV_H_GUARD