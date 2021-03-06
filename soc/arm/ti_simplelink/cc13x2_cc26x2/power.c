/*
 * Copyright (c) 2019 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <init.h>
#include <power/power.h>

#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26X2.h>

#include <ti/drivers/dpl/ClockP.h>

#include <ti/devices/cc13x2_cc26x2/driverlib/cpu.h>
#include <ti/devices/cc13x2_cc26x2/driverlib/vims.h>
#include <ti/devices/cc13x2_cc26x2/driverlib/sys_ctrl.h>

#include <logging/log.h>
#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

/* Configuring TI Power module to not use its policy function (we use Zephyr's
 * instead), and disable oscillator calibration functionality for now.
 */
const PowerCC26X2_Config PowerCC26X2_config = {
	.policyInitFxn      = NULL,
	.policyFxn          = NULL,
	.calibrateFxn       = NULL,
	.enablePolicy       = false,
	.calibrateRCOSC_LF  = false,
	.calibrateRCOSC_HF  = false
};

extern PowerCC26X2_ModuleState PowerCC26X2_module;

/*
 * Power state mapping:
 * SYS_POWER_STATE_SLEEP_1: Idle
 * SYS_POWER_STATE_SLEEP_2: Standby
 */

/* Invoke Low Power/System Off specific Tasks */
void sys_set_power_state(enum power_states state)
{
#ifdef CONFIG_SYS_POWER_SLEEP_STATES
	u32_t modeVIMS;
	u32_t constraints;
#endif

	LOG_DBG("SoC entering power state %d", state);

	/* Switch to using PRIMASK instead of BASEPRI register, since
	 * we are only able to wake up from standby while using PRIMASK.
	 */
	/* Set PRIMASK */
	CPUcpsid();
	/* Set BASEPRI to 0 */
	irq_unlock(0);

	switch (state) {
#ifdef CONFIG_SYS_POWER_SLEEP_STATES
	case SYS_POWER_STATE_SLEEP_1:
		/* query the declared constraints */
		constraints = Power_getConstraintMask();
		/* 1. Get the current VIMS mode */
		do {
			modeVIMS = VIMSModeGet(VIMS_BASE);
		} while (modeVIMS == VIMS_MODE_CHANGING);

		/* 2. Configure flash to remain on in IDLE or not and keep
		 *    VIMS powered on if it is configured as GPRAM
		 * 3. Always keep cache retention ON in IDLE
		 * 4. Turn off the CPU power domain
		 * 5. Ensure any possible outstanding AON writes complete
		 * 6. Enter IDLE
		 */
		if ((constraints & (1 << PowerCC26XX_NEED_FLASH_IN_IDLE)) ||
			(modeVIMS == VIMS_MODE_DISABLED)) {
			SysCtrlIdle(VIMS_ON_BUS_ON_MODE);
		} else {
			SysCtrlIdle(VIMS_ON_CPU_ON_MODE);
		}

		/* 7. Make sure MCU and AON are in sync after wakeup */
		SysCtrlAonUpdate();
		break;

	case SYS_POWER_STATE_SLEEP_2:
		/* schedule the wakeup event */
		ClockP_start(ClockP_handle((ClockP_Struct *)
			&PowerCC26X2_module.clockObj));

		/* go to standby mode */
		Power_sleep(PowerCC26XX_STANDBY);
		ClockP_stop(ClockP_handle((ClockP_Struct *)
			&PowerCC26X2_module.clockObj));
		break;
#endif
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}

	LOG_DBG("SoC leaving power state %d", state);
}

/* Handle SOC specific activity after Low Power Mode Exit */
void _sys_pm_power_state_exit_post_ops(enum power_states state)
{
	/*
	 * System is now in active mode. Reenable interrupts which were disabled
	 * when OS started idling code.
	 */
	CPUcpsie();
}

/* Initialize TI Power module */
static int power_initialize(struct device *dev)
{
	unsigned int ret;

	ARG_UNUSED(dev);

	ret = irq_lock();
	Power_init();
	irq_unlock(ret);

	return 0;
}

/*
 *  ======== PowerCC26XX_schedulerDisable ========
 */
void PowerCC26XX_schedulerDisable(void)
{
	/*
	 * We are leaving this empty because Zephyr's
	 * scheduler would not get to run with interrupts being disabled
	 * in the context of Power_sleep() in any case.
	 */
}

/*
 *  ======== PowerCC26XX_schedulerRestore ========
 */
void PowerCC26XX_schedulerRestore(void)
{
	/*
	 * We are leaving this empty because Zephyr's
	 * scheduler would not get to run with interrupts being disabled
	 * in the context of Power_sleep() in any case.
	 */
}

SYS_INIT(power_initialize, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
