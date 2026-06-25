/*
 * Generated using zcbor version 0.9.1
 * https://github.com/NordicSemiconductor/zcbor
 * Generated with a --default-max-qty of 16
 */

#ifndef DAEMON_API_CLIENT_DECODE_H__
#define DAEMON_API_CLIENT_DECODE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "daemon_api_client_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if DEFAULT_MAX_QTY != 16
#error "The type file was generated with a different default_max_qty than this file"
#endif


int cbor_decode_api_request(
		const uint8_t *payload, size_t payload_len,
		struct api_request_r *result,
		size_t *payload_len_out);


int cbor_decode_api_response(
		const uint8_t *payload, size_t payload_len,
		struct api_response_r *result,
		size_t *payload_len_out);


#ifdef __cplusplus
}
#endif

#endif /* DAEMON_API_CLIENT_DECODE_H__ */
