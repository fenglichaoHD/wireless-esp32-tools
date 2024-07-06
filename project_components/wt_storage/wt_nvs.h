#ifndef WT_NVS_H_GUARD
#define WT_NVS_H_GUARD

#include <stdint.h>
#include <nvs.h>

typedef enum wt_nvs_error_enum {
	WT_NVS_OK = 0,
	WT_NVS_ERR,
	WT_NVS_NO_MEM,
	WT_NVS_ERR_NOT_FOUND,

} wt_nvs_key_error;

int wt_nvs_open(const char* namespace, nvs_handle_t *out_handle);

void wt_nvs_close(nvs_handle_t handle);

int wt_nvs_get(nvs_handle_t handle, uint32_t key, void *data, uint32_t data_size);

int wt_nvs_set(nvs_handle_t handle, uint32_t key, void *data, uint32_t data_size);

int wt_nvs_get_once(const char* namespace, const uint32_t key, void *data, uint32_t data_size);
int wt_nvs_set_once(const char* namespace, const uint32_t key, void *data, uint32_t data_size);

int wt_nvs_flush(nvs_handle_t handle);

void wt_nvs_init();

#endif //WT_NVS_H_GUARD