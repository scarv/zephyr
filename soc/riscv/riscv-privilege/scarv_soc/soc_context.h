#ifndef _SCARV_SOC_SOC_CONTEXT_H_
#define _SCARV_SOC_SOC_CONTEXT_H_

/* Extra state for SCARV SoC registers */
#define SOC_ESF_MEMBERS \
    u32_t uxcrypto;

/* Initial saved state. */
#define SOC_ESF_INIT \
    0xdeadbaad

#endif
