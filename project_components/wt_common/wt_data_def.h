/*
 * SPDX-FileCopyrightText: 2024 kerms <kerms@niazo.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WT_DATA_DEF_H_GUARD
#define WT_DATA_DEF_H_GUARD

#include <stdint.h>

#define DECLARE_HANDLE(name) struct name##__ { int unused[0]; }; \
                             typedef struct name##__ *name

typedef enum wt_data_type_t {
	WT_DATA_RESERVED         = 0x00,
	/* primitive type */
	WT_DATA_EVENT            = 0x02,
	WT_DATA_ROUTE_HDR        = 0x03,
	WT_DATA_RAW_BROADCAST    = 0x04,

	/* data_type */
	/* broadcast data */
	WT_DATA_CMD_BROADCAST    = 0x11,

	/* targeted data */
	WT_DATA_RAW              = 0x20,
	WT_DATA_CMD              = 0x21,
	WT_DATA_RESPONSE         = 0x22,

	/* standard protocols */
	WT_DATA_PROTOBUF         = 0x40,
	WT_DATA_JSON             = 0x41,
	WT_DATA_MQTT             = 0x42,
} __attribute__((packed)) wt_data_type_t;
_Static_assert(sizeof(wt_data_type_t) == 1, "wt_data_type_t must be 1 byte");

typedef struct wt_bin_data_hdr_t {
	wt_data_type_t data_type; /* type of the payload */
	union {
		/* when targeted message -> bin data handle */
		struct {
			uint8_t module_id; /* src when broadcast, else target module */
			uint8_t sub_id;    /* src when broadcast, else target sub_id */
		};
		/* not used, only for make the union == 3B */
		struct {
			uint8_t dummy1;
			uint8_t dummy2;
			uint8_t dummy3;
		} dummy;
	};
} wt_bin_data_hdr_t;
_Static_assert(sizeof(wt_bin_data_hdr_t) == 4, "wt_data_4B_hdr_t must be 4 byte");

typedef struct wt_bin_data_t {
	wt_bin_data_hdr_t hdr;
	uint8_t payload[0];
} wt_bin_data_t;

typedef struct wt_bin_data_internal_t {
	struct {
		uint64_t Dummy1;
		uint64_t Dummy2;
	} ws_frame_slot; /* 16 byte padding for httpd_ws_frame */
	struct { /*  */
		uint16_t data_len;
		uint8_t src_module;
		uint8_t src_sub_module;
	};
	struct { /*  */
		uint8_t send_count;
		uint8_t reserved1;
		uint16_t reserved2;
	};
	wt_bin_data_t data;
} wt_bin_data_internal_t;

typedef union wt_port_info {
	uint8_t name[32]; /* for ease identification */
	struct { /* for socket */
		uint32_t foreign_ip;
		uint16_t local_port;
		uint16_t foreign_port;
	};
	struct { /* for peripheral port number */
		uint8_t periph_num;
	};
} wt_port_info_t;

#endif //WT_DATA_DEF_H_GUARD