#include <offsets.h>
#include <toolchain.h>

#include <soc.h>

/* Exports */
#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
GTEXT(__soc_save_context)
GTEXT(__soc_restore_context)
#endif

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
/*
 * The SCARV core has an extra register uxcrypto.
 *
 * For details, see the Kconfig help for CONFIG_RISCV_SOC_CONTEXT_SAVE.
 */
SECTION_FUNC(exception.other, __soc_save_context)
    csrr t0, SCARV_SOC_UXCRYPTO
    sw t0, __soc_esf_t_uxcrypto_OFFSET(a0)

    ret

SECTION_FUNC(exception.other, __soc_restore_context)
    lw t0, __soc_esf_t_uxcrypto_OFFSET(a0)
    csrw SCARV_SOC_UXCRYPTO, t0

    ret
#endif
