#include "wifi_storage.h"
#include "wifi_storage_priv.h"
#include "wt_nvs.h"
#include "wifi_api.h"

#include <stdio.h>

#define WIFI_NVS_NAMESPACE "wt_wifi"

int wifi_data_get_sta_last_conn_cred(wifi_credential_t *ap_credential)
{
	uint32_t ap_bitmap = 0;
	nvs_handle_t handle;
	int err;

	err = wt_nvs_open(WIFI_NVS_NAMESPACE, &handle);
	if (err) {
		return err;
	}

	err = wt_nvs_get(handle, KEY_WIFI_STA_AP_BITMAP, &ap_bitmap, sizeof(ap_bitmap));
	if (err) {
		goto end;
	}
	if (ap_bitmap == 0) {
		err = WT_NVS_ERR_NOT_FOUND;
		goto end;
	}

	err = wt_nvs_get(handle, KEY_WIFI_STA_LAST_AP_CRED,
	                 ap_credential, sizeof(wifi_credential_t));
	if (err) {
		goto end;
	}

end:
	wt_nvs_close(handle);
	return err;
}

/*
 * Called when connect to an AP,
 */
int wifi_data_save_sta_ap_credential(wifi_credential_t *ap_credential)
{
	uint32_t ap_bitmap;
	int err;
	nvs_handle_t handle;

	err = wt_nvs_open(WIFI_NVS_NAMESPACE, &handle);
	if (err) {
		return err;
	}

	err = wt_nvs_get(handle, KEY_WIFI_STA_AP_BITMAP, &ap_bitmap, sizeof(ap_bitmap));
	if (err) {
		if (err != ESP_ERR_NVS_NOT_FOUND) {
			goto end;
		}
		ap_bitmap = 0;
	}
	if (ap_bitmap == 0) {
		ap_bitmap = 1;
		err = wt_nvs_set(handle, KEY_WIFI_STA_AP_BITMAP, &ap_bitmap, sizeof(ap_bitmap));
	}


	wifi_credential_t credential;
	err = wt_nvs_get(handle, KEY_WIFI_STA_LAST_AP_CRED, &credential, sizeof(ap_bitmap));
	if (err) {
		if (err != ESP_ERR_NVS_NOT_FOUND) {
			goto end;
		}
	}

	err = wt_nvs_set(handle, KEY_WIFI_STA_LAST_AP_CRED, ap_credential, sizeof(wifi_credential_t));
	if (err) {
		goto end;
	}

end:
	wt_nvs_close(handle);
	return err;
}

int wifi_data_save_wifi_mode(wifi_apsta_mode_e mode)
{
	nvs_handle_t handle;
	int err;

	uint8_t mode_u8 = mode;

	err = wt_nvs_open(WIFI_NVS_NAMESPACE, &handle);
	if (err) {
		return err;
	}

	err = wt_nvs_set(handle, KEY_WIFI_APSTA_MODE, &mode_u8, sizeof(mode_u8));

	wt_nvs_close(handle);
	return err;
}

int wifi_data_get_wifi_mode(wifi_apsta_mode_e *mode)
{
	nvs_handle_t handle;
	int err;

	uint8_t mode_u8;

	err = wt_nvs_open(WIFI_NVS_NAMESPACE, &handle);
	if (err) {
		return err;
	}

	err = wt_nvs_get(handle, KEY_WIFI_APSTA_MODE, &mode_u8, sizeof(mode_u8));

	*mode = mode_u8;
	wt_nvs_close(handle);
	return err;
}

int wifi_data_save_ap_credential(wifi_credential_t *ap_credential)
{
	nvs_handle_t handle;
	int err;

	err = wt_nvs_open(WIFI_NVS_NAMESPACE, &handle);
	if (err) {
		return err;
	}

	err = wt_nvs_set(handle, KEY_WIFI_AP_CRED, ap_credential, sizeof(wifi_credential_t));

	wt_nvs_close(handle);
	return err;
}

int wifi_data_get_ap_credential(wifi_credential_t *ap_credential)
{
	nvs_handle_t handle;
	int err;

	err = wt_nvs_open(WIFI_NVS_NAMESPACE, &handle);
	if (err) {
		return err;
	}

	err = wt_nvs_get(handle, KEY_WIFI_AP_CRED, ap_credential, sizeof(wifi_credential_t));

	wt_nvs_close(handle);
	return err;
}

int wifi_data_get_static(wifi_api_sta_ap_static_info_t *static_info)
{
	nvs_handle_t handle;
	int err;

	err = wt_nvs_open(WIFI_NVS_NAMESPACE, &handle);
	if (err) {
		return err;
	}

	err = wt_nvs_get(handle, KEY_WIFI_STA_STATIC_BASE,
	                 static_info, sizeof(wifi_api_sta_ap_static_info_t));
	if (err == ESP_ERR_NVS_NOT_FOUND) {
		static_info->static_ip_en = 0;
		static_info->static_dns_en = 0;
		err = ESP_OK;
	} else if (err) {
		printf("err %d\n", err);
		goto end;
	}

	if (static_info->static_ip_en > 1) {
		static_info->static_ip_en = 0;
	}
	if (static_info->static_dns_en > 1) {
		static_info->static_dns_en = 0;
	}

end:
	wt_nvs_close(handle);
	return err;
}

int wifi_data_save_static(wifi_api_sta_ap_static_info_t *static_info)
{
	nvs_handle_t handle;
	int err;

	err = wt_nvs_open(WIFI_NVS_NAMESPACE, &handle);
	if (err) {
		return err;
	}

	err = wt_nvs_set(handle, KEY_WIFI_STA_STATIC_BASE,
	                 static_info, sizeof(wifi_api_sta_ap_static_info_t));
	if (err) {
		goto end;
	}

	printf("static info saved\n");

end:
	wt_nvs_close(handle);
	return err;
}
