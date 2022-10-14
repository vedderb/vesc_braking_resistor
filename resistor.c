/*
	Copyright 2021 - 2022 Benjamin Vedder	benjamin@vedder.se

	This file is part of the VESC Braking Resistor firmware.

	The VESC Braking Resistor firmware is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The VESC Braking Resistor firmware is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

#include "ch.h"
#include "hal.h"
#include "resistor.h"
#include "stm32l4xx_hal_conf.h"
#include "stm32l4xx_ll_tim.h"
#include "stm32l4xx_ll_bus.h"
#include "utils.h"
#include "terminal.h"
#include "commands.h"
#include "stdlib.h"
#include "pwr.h"
#include "main.h"

// Threads
static THD_WORKING_AREA(resistor_thread_wa, 512);
static THD_FUNCTION(resistor_thread, arg);

// Private functions
static void terminal_pwm(int argc, const char **argv);
static void terminal_pwm_to(int argc, const char **argv);

// Private variables
static volatile systime_t m_resistor_set_time = 0;
static volatile float m_curr_filter = 0.0;
static volatile float m_voltage_filter = 0.0;
static volatile float m_temp_max = 0.0;
static volatile float m_temp_max_filter = 0.0;
static volatile float m_pwm_now = 0.0;
static volatile float m_pwm_max = 1.0;

// Settings
#define DEADTIME_NS			300
#define F_SW				150000

void resistor_init(void) {
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM1);

	LL_TIM_DeInit(TIM1);
	TIM1->CNT = 0;

	LL_TIM_SetCounterMode(TIM1, LL_TIM_COUNTERMODE_UP);
	LL_TIM_SetPrescaler(TIM1, 0);
	LL_TIM_SetAutoReload(TIM1, __LL_TIM_CALC_ARR(SystemCoreClock, LL_TIM_GetPrescaler(TIM1), F_SW));
	LL_TIM_EnableARRPreload(TIM1);

	LL_TIM_OC_SetMode(TIM1,  LL_TIM_CHANNEL_CH1,  LL_TIM_OCMODE_PWM1);
	LL_TIM_OC_ConfigOutput(TIM1, LL_TIM_CHANNEL_CH1, LL_TIM_OCPOLARITY_HIGH | LL_TIM_OCIDLESTATE_HIGH);
	LL_TIM_OC_SetCompareCH1(TIM1, 0);
	LL_TIM_OC_EnablePreload(TIM1, LL_TIM_CHANNEL_CH1);

	LL_TIM_BDTR_InitTypeDef TIM_BDTRInitStruct;
	LL_TIM_BDTR_StructInit(&TIM_BDTRInitStruct);
	TIM_BDTRInitStruct.BreakPolarity = LL_TIM_BREAK_POLARITY_HIGH;
	TIM_BDTRInitStruct.AutomaticOutput = LL_TIM_AUTOMATICOUTPUT_DISABLE;
	TIM_BDTRInitStruct.OSSRState = LL_TIM_OSSR_ENABLE;
	TIM_BDTRInitStruct.OSSIState = LL_TIM_OSSI_ENABLE;
	TIM_BDTRInitStruct.BreakState = LL_TIM_BREAK_DISABLE;
	TIM_BDTRInitStruct.LockLevel = LL_TIM_LOCKLEVEL_OFF;
	// Set dead time to 0,1 us (100 ns)
	TIM_BDTRInitStruct.DeadTime = __LL_TIM_CALC_DEADTIME(SystemCoreClock, LL_TIM_GetClockDivision(TIM1), DEADTIME_NS);
	LL_TIM_BDTR_Init(TIM1, &TIM_BDTRInitStruct);

	LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH1 | LL_TIM_CHANNEL_CH1N);
	LL_TIM_EnableAllOutputs(TIM1);
	LL_TIM_EnableCounter(TIM1);

	LL_TIM_GenerateEvent_UPDATE(TIM1);

	// Connect pins to timer
	palSetLineMode(PAL_LINE(GPIOA, 7), PAL_MODE_ALTERNATE(1));
	palSetLineMode(PAL_LINE(GPIOA, 8), PAL_MODE_ALTERNATE(1));

	chThdCreateStatic(resistor_thread_wa, sizeof(resistor_thread_wa), NORMALPRIO, resistor_thread, NULL);

	terminal_register_command_callback(
			"res_pwm",
			"Set resistor PWM",
			"[0 to 1000]",
			terminal_pwm);

	terminal_register_command_callback(
			"res_pwm_to",
			"Set resistor PWM with timeout",
			"[0 to 1000]",
			terminal_pwm_to);
}

static THD_FUNCTION(resistor_thread, arg) {
	(void)arg;

	chRegSetThreadName("Resistor");

	for (;;) {
		float temp = pwr_get_temp(0);
		if (pwr_get_temp(1) > temp) {
			temp = pwr_get_temp(1);
		}
		if (pwr_get_temp(2) > temp) {
			temp = pwr_get_temp(2);
		}
		m_temp_max = temp;

		UTILS_LP_FAST(m_temp_max_filter, m_temp_max, 0.05);
		UTILS_LP_FAST(m_curr_filter, pwr_get_iin(), 0.01);
		UTILS_LP_FAST(m_voltage_filter, pwr_get_vin(), 0.5);

		// Apply limits
		float lo_temp = 0.0;
		if (m_temp_max_filter < backup.config.temp_lim_start) {
			lo_temp = 1.0;
		} else if (m_temp_max_filter > backup.config.temp_lim_end) {
			lo_temp = 0.0;
		} else {
			lo_temp = utils_map(m_temp_max_filter,
					backup.config.temp_lim_start,
					backup.config.temp_lim_end,
					1.0, 0.0);
		}

		if (lo_temp < 0.9) {
			LED_ON(LINE_LED_RED);
		} else {
			LED_OFF(LINE_LED_RED);
		}

		float lo_volts = 0.0;
		float volts = m_voltage_filter;
		if (volts > backup.config.volt_lower_lim_start) {
			lo_volts = 1.0;
		} else if (volts < backup.config.volt_lower_lim_end) {
			lo_volts = 0.0;
		} else {
			lo_volts = utils_map(volts,
					backup.config.volt_lower_lim_start,
					backup.config.volt_lower_lim_end,
					1.0, 0.0);
		}

		m_pwm_max = utils_min_abs(lo_temp, lo_volts);

		if (m_pwm_now > m_pwm_max) {
			resistor_set_pwm(m_pwm_max);
		}

		// Timeout
		if (UTILS_AGE_S(m_resistor_set_time) > 2.0) {
			resistor_set_pwm(0.0);
		}

		// Automatic control
		if (backup.config.load_volt_max_fraction > 0.02) {
			float auto_ctrl = -1.0;
			if (volts < backup.config.load_volt_start) {
				auto_ctrl = -1.0;
			} else if (volts > backup.config.load_volt_max) {
				auto_ctrl = backup.config.load_volt_max_fraction;
			} else {
				auto_ctrl = utils_map(volts,
						backup.config.load_volt_start,
						backup.config.load_volt_max,
						0.0, backup.config.load_volt_max_fraction);
			}

			if (auto_ctrl > 0.0) {
				resistor_set_pwm(auto_ctrl);
			}
		}

		chThdSleepMilliseconds(1);
	}
}

void resistor_set_pwm(float pwm) {
	utils_truncate_number(&pwm, 0.0, m_pwm_max);
	m_pwm_now = pwm;

	uint32_t val = (uint32_t)((float)LL_TIM_GetAutoReload(TIM1) * pwm);
	LL_TIM_OC_SetCompareCH1(TIM1, val);
	LL_TIM_GenerateEvent_UPDATE(TIM1);
	m_resistor_set_time = chVTGetSystemTimeX();

	if (m_pwm_now > 0.001) {
		LED_ON(LINE_LED_GREEN);
	} else {
		LED_OFF(LINE_LED_GREEN);
	}
}

float resistor_get_current_filtered(void) {
	return m_curr_filter;
}

static void terminal_pwm(int argc, const char **argv) {
	if (argc == 2) {
		int d = -1;
		d = atoi(argv[1]);

		if (d >= 0 && d <= 1000) {
			resistor_set_pwm((float)d / 1000.0);
			commands_printf("ok\n");
		} else {
			commands_printf("Invalid argument\n");
		}
	} else {
		commands_printf("This command requires one argument.\n");
	}
}

static void terminal_pwm_to(int argc, const char **argv) {
	if (argc == 2) {
		int d = -1;
		d = atoi(argv[1]);

		if (d >= 0 && d <= 1000) {
			resistor_set_pwm((float)d / 1000.0);
			commands_printf("ON");

			float t = 3.0;
			while (t > 0) {
				commands_printf("Time     : %.0f", t);
				commands_printf("V In     :%.2f", pwr_get_vin());
				commands_printf("I In     :%.2f", resistor_get_current_filtered());
				commands_printf("T Center :%.2f", pwr_get_temp(0));
				commands_printf("T MOS    :%.2f", pwr_get_temp(1));
				commands_printf("T PCB    :%.2f", pwr_get_temp(2));
				commands_printf(" ");
				t -= 0.2;
				resistor_set_pwm((float)d / 1000.0);
				chThdSleepMilliseconds(200);
			}

			resistor_set_pwm(0.0);
			commands_printf("OFF");
		} else {
			commands_printf("Invalid argument\n");
		}
	} else {
		commands_printf("This command requires one argument.\n");
	}
}
