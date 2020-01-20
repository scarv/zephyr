/**
 * @file SoC configuration macros for the SCARV SOC processor
 */

#ifndef __RISCV_SCARV_SOC_H_
#define __RISCV_SCARV_SOC_H_

#include <soc_common.h>
#include <generated_dts_board.h>

/* Timer configuration */
#define RISCV_MTIME_BASE             0x00001000
#define RISCV_MTIMECMP_BASE          0x00001008

/* lib-c hooks required RAM defined variables */
#define RISCV_RAM_BASE               DT_SRAM_BASE_ADDRESS
#define RISCV_RAM_SIZE               KB(DT_SRAM_SIZE)

#endif /* __RISCV_SCARV_SOC_H_ */
