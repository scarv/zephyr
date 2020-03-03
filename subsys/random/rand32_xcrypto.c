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

/*
 * Symbols used to ensure a rapid series of calls to random number generator
 * return different values.
 */
static atomic_val_t _rand32_counter;

u32_t sys_rand32_get(void)
{
	return k_cycle_get_32() + atomic_add(&_rand32_counter, _RAND32_INC);
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
