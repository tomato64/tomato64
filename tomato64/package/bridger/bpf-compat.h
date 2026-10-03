/*
 * Force-included when compiling bridger-bpf.c.
 *
 * OpenWrt builds the eBPF object against a prepared kernel source tree, which
 * is where the uintN_t and uN types it uses come from. Here it is built
 * against the sanitized kernel headers in staging instead, which define only
 * the __uN family, so supply the rest from the compiler's freestanding headers.
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
