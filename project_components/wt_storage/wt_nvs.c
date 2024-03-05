#include "wt_nvs.h"

#include <nvs_flash.h>

void wt_nvs_init()
{
	// Initialize default NVS
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		// NVS partition was truncated and needs to be erased
		// Retry nvs_flash_init
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);
}

int wt_nvs_open(const char* namespace, nvs_handle_t *out_handle)
{
	return nvs_open(namespace, NVS_READWRITE, out_handle);
}

void wt_nvs_close(nvs_handle_t handle)
{
	ESP_ERROR_CHECK(nvs_commit(handle));
	nvs_close(handle);
}

int wt_nvs_get(nvs_handle_t handle, const uint32_t key, void *data, uint32_t data_size)
{
	switch (data_size) {
	case 0: {
		uint8_t tmp;
		return nvs_get_u8(handle, (const char *) &key, &tmp);
	}
	case 1:
		return nvs_get_u8(handle, (const char *) &key, data);
	case 2:
		return nvs_get_u16(handle, (const char *) &key, data);
	case 4:
		return nvs_get_u32(handle, (const char *) &key, data);
	case 8:
		return nvs_get_u64(handle, (const char *) &key, data);
	default:
		return nvs_get_blob(handle, (const char *) &key, data,
							(size_t *) &data_size);
	}
}

int wt_nvs_set(nvs_handle_t handle, const uint32_t key, void *data, uint32_t data_size)
{
	switch (data_size) {
	case 0: {
		uint8_t tmp = 0xFF;
		return nvs_set_u8(handle, (const char *) &key, tmp);
	}
	case 1:
		return nvs_set_u8(handle, (const char *) &key, *(uint8_t *)data);
	case 2:
		return nvs_set_u16(handle, (const char *) &key, *(uint16_t *)data);
	case 4:
		return nvs_set_u32(handle, (const char *) &key, *(uint32_t *)data);
	case 8:
		return nvs_set_u64(handle, (const char *) &key, *(uint64_t *)data);
	default:
		return nvs_set_blob(handle, (const char *) &key, data, data_size);
	}
}

int wt_nvs_get_once(const char* namespace, const uint32_t key, void *data, uint32_t data_size)
{
	nvs_handle_t handle;
	int err;
	err = wt_nvs_open(namespace, &handle);
	if (err) {
		return err;
	}

	err = wt_nvs_get(handle, key, data, data_size);
	if (err) {
		return err;
	}

	wt_nvs_close(handle);
	return 0;
}
