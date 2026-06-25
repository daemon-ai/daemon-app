/*
 * Generated using zcbor version 0.9.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 16
 */

#ifndef DAEMON_API_SMOKE_ENCODE_H__
#define DAEMON_API_SMOKE_ENCODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "daemon_api_smoke_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if DEFAULT_MAX_QTY != 16
#error "The type file was generated with a different default_max_qty than this file"
#endif


int cbor_encode_api_request(
		uint8_t *payload, size_t payload_len,
		const struct api_request_r *input,
		size_t *payload_len_out);


int cbor_encode_api_response(
		uint8_t *payload, size_t payload_len,
		const struct api_response_r *input,
		size_t *payload_len_out);


#ifdef __cplusplus
}
#endif

#endif /* DAEMON_API_SMOKE_ENCODE_H__ */
