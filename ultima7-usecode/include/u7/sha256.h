#ifndef U7_SHA256_H
#define U7_SHA256_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The SHA-256 of a buffer as 64 lowercase hex digits and a terminating zero. */
void u7_sha256_hex(const void *data, size_t size, char out[65]);

#ifdef __cplusplus
}
#endif

#endif
