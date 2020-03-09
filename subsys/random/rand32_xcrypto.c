/**
 * @file
 * @brief Random number generator which utilises the XCrypto instructions.
 *
 * This module provides a random implementation of sys_rand32_get().
 */

#include <random/rand32.h>
#include <drivers/timer/system_timer.h>
#include <kernel.h>
#include <sys/atomic.h>
#include <string.h>

static inline void _xc_rngseed(int32_t rs1) {
    __asm__("xc.rngseed %0" : : "r"(rs1));
}

static inline int32_t _xc_rngsamp() {
    int rd;

    __asm__("xc.rngsamp %0" : "=r"(rd));

    return rd;
}

static inline bool _xc_rngtest() {
    bool rd;

    __asm__("xc.rngtest %0" : "=r"(rd));

    return rd;
}

u32_t sys_rand32_get(void)
{
    _xc_rngseed(k_cycle_get_32());

    while (!_xc_rngtest()) {
        _xc_rngseed(k_cycle_get_32());
    }

	return _xc_rngsamp();
}

void sys_rand_get(void *dst, size_t outlen)
{
	u32_t len = 0;
	u32_t blocksize = 4;
	u32_t ret;
	u32_t *udst = (u32_t *)dst;

	while (len < outlen) {
		ret = sys_rand32_get();
		if ((outlen-len) < sizeof(ret)) {
			blocksize = len;
			(void *)memcpy(udst, &ret, blocksize);
		} else {
			(*udst++) = ret;
		}
		len += blocksize;
	}
}
