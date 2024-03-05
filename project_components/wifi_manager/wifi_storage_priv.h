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
	KEY_WIFI_AP_SSID,
	KEY_WIFI_AP_PASSWORD,

/* TODO: should have 1 for each AP */
	KEY_WIFI_STA_USE_STATIC, /* bit[0:31]=[IP, MASK, GATEWAY, DNS] */
	KEY_WIFI_STA_STATIC_IP, /* 4B */
	KEY_WIFI_STA_STATIC_MASK, /* 4B */
	KEY_WIFI_STA_STATIC_GATEWAY, /* 4B */
	KEY_WIFI_STA_STATIC_DNS, /* 4B */

/* AP's information */
	KEY_WIFI_STA_LAST_AP_CRED, /*!< ssid[32] + password[64] */
	KEY_WIFI_STA_AP_BITMAP,
} wt_wifi_key;

#endif //WIFI_STORAGE_PRIV_H_GUARD