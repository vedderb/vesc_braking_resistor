/*
	Copyright 2019 - 2020 Benjamin Vedder	benjamin@vedder.se

	This file is part of the VESC BMS firmware.

	The VESC BMS firmware is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The VESC BMS firmware is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

#include "pwr.h"
#include "main.h"
#include <math.h>
#include <string.h>

// Settings
#define ADC_CHANNELS	11

// Private variables
static volatile float m_v_in = 0.0;
static volatile float m_i_in = 0.0;
static volatile float m_temps[HW_ADC_TEMP_SENSORS] = {0.0};

static THD_WORKING_AREA(adc_thd_wa, 2048);

const ADCConversionGroup adcgrpcfg1 = {
		.circular     = false,
		.num_channels = ADC_CHANNELS,
		.end_cb       = NULL,
		.error_cb     = NULL,
		.cfgr         = ADC_CFGR_CONT,
		.cfgr2        = 0U,
		.tr1          = ADC_TR(0, 4095),
		.smpr         = {
				ADC_SMPR1_SMP_AN0(ADC_SMPR_SMP_92P5) | ADC_SMPR1_SMP_AN1(ADC_SMPR_SMP_92P5) |
				ADC_SMPR1_SMP_AN2(ADC_SMPR_SMP_92P5) | ADC_SMPR1_SMP_AN3(ADC_SMPR_SMP_92P5) |
				ADC_SMPR1_SMP_AN4(ADC_SMPR_SMP_92P5) | ADC_SMPR1_SMP_AN5(ADC_SMPR_SMP_92P5) |
				ADC_SMPR1_SMP_AN6(ADC_SMPR_SMP_92P5) | ADC_SMPR1_SMP_AN7(ADC_SMPR_SMP_92P5) |
				ADC_SMPR1_SMP_AN8(ADC_SMPR_SMP_92P5) | ADC_SMPR1_SMP_AN9(ADC_SMPR_SMP_92P5),
				ADC_SMPR2_SMP_AN10(ADC_SMPR_SMP_92P5) | ADC_SMPR2_SMP_AN11(ADC_SMPR_SMP_92P5) |
				ADC_SMPR2_SMP_AN12(ADC_SMPR_SMP_92P5) | ADC_SMPR2_SMP_AN13(ADC_SMPR_SMP_92P5) |
				ADC_SMPR2_SMP_AN14(ADC_SMPR_SMP_92P5) | ADC_SMPR2_SMP_AN15(ADC_SMPR_SMP_92P5) |
				ADC_SMPR2_SMP_AN16(ADC_SMPR_SMP_92P5) | ADC_SMPR2_SMP_AN17(ADC_SMPR_SMP_92P5) |
				ADC_SMPR2_SMP_AN18(ADC_SMPR_SMP_92P5)
		},
		.sqr          = {
				ADC_SQR1_SQ1_N(ADC_CHANNEL_IN0) | ADC_SQR1_SQ2_N(ADC_CH_EX1) |
				ADC_SQR1_SQ3_N(ADC_CH_CURRENT) | ADC_SQR1_SQ4_N(ADC_CH_TEMP0),
				ADC_SQR2_SQ5_N(ADC_CH_TEMP1) | ADC_SQR2_SQ6_N(ADC_CH_TEMP2) |
				ADC_SQR2_SQ7_N(ADC_CH_TEMP3) | ADC_SQR2_SQ8_N(ADC_CH_TEMP4) |
				ADC_SQR2_SQ9_N(ADC_CH_TEMP5),
				ADC_SQR3_SQ10_N(ADC_CH_TEMP6) | ADC_SQR3_SQ11_N(ADC_CH_VIN),
				0U
		}
};

static THD_FUNCTION(adc_thd, p) {
	(void)p;
	chRegSetThreadName("ADC");

	adcStart(&ADCD1, NULL);
	adcSTM32EnableVREF(&ADCD1);
	chThdSleepMilliseconds(1);

	while (!chThdShouldTerminateX()) {
		int num_samp = 8;
		adcsample_t samples[num_samp * ADC_CHANNELS];

		adcConvert(&ADCD1, &adcgrpcfg1, samples, num_samp);

		float v_ex1 = 0.0;
		float ref = 0.0;
		float i_in = 0.0;
		float v_in = 0.0;
		float temps[HW_ADC_TEMP_SENSORS];
		memset(temps, 0, sizeof(temps));

		for (int i = 0;i < num_samp;i++) {
			ref += samples[ADC_CHANNELS * i + 0];
			v_ex1 += samples[ADC_CHANNELS * i + 1];
			i_in += samples[ADC_CHANNELS * i + 2];

			for (int j = 0;j < HW_ADC_TEMP_SENSORS;j++) {
				temps[j] += samples[ADC_CHANNELS * i + 3 + j];
			}

			v_in += samples[ADC_CHANNELS * i + HW_ADC_TEMP_SENSORS + 4];
		}

		ref /= (float)num_samp;
		v_ex1 /= (float)num_samp;
		i_in /= (float)num_samp;
		v_in /= (float)num_samp;

		for (int j = 0;j < HW_ADC_TEMP_SENSORS;j++) {
			temps[j] /= (float)num_samp;
		}

		uint16_t vrefint_cal = *((uint16_t*)((uint32_t)0x1FFF75AA));
		float vdda = (3.0 * (float)vrefint_cal) / (float)ref;

		m_v_in = (v_in / (4095.0 / vdda)) * ((R_IN_TOP + R_IN_BOTTOM) / R_IN_BOTTOM);
		m_i_in = (3.3 * (i_in / 4095.0)) * (1.0 / HW_SHUNT_AMP_GAIN) * (1.0 / HW_SHUNT_RES);

		for (int j = 0;j < HW_ADC_TEMP_SENSORS;j++) {
			m_temps[j] = NTC_TEMP_WITH_IND(temps[j], j);
		}

		chThdSleepMilliseconds(1);
	}
}

void pwr_init(void) {
	HW_INIT_HOOK();

	palSetLineMode(LINE_EX1, PAL_MODE_INPUT_ANALOG);
	palSetLineMode(LINE_VIN, PAL_MODE_INPUT_ANALOG);
	palSetLineMode(LINE_CURRENT, PAL_MODE_INPUT_ANALOG);
	palSetLineMode(LINE_TEMP_0, PAL_MODE_INPUT_ANALOG);
	palSetLineMode(LINE_TEMP_1, PAL_MODE_INPUT_ANALOG);
	palSetLineMode(LINE_TEMP_2, PAL_MODE_INPUT_ANALOG);
	palSetLineMode(LINE_TEMP_3, PAL_MODE_INPUT_ANALOG);
	palSetLineMode(LINE_TEMP_4, PAL_MODE_INPUT_ANALOG);
	palSetLineMode(LINE_TEMP_5, PAL_MODE_INPUT_ANALOG);
	palSetLineMode(LINE_TEMP_6, PAL_MODE_INPUT_ANALOG);

	palSetLineMode(LINE_TEMP_0_EN, PAL_MODE_OUTPUT_PUSHPULL);
	palSetLineMode(LINE_TEMP_1_EN, PAL_MODE_OUTPUT_PUSHPULL);
	palSetLineMode(LINE_TEMP_2_EN, PAL_MODE_OUTPUT_PUSHPULL);
	palSetLineMode(LINE_TEMP_3_EN, PAL_MODE_OUTPUT_PUSHPULL);
	palSetLineMode(LINE_TEMP_4_EN, PAL_MODE_OUTPUT_PUSHPULL);
	palSetLineMode(LINE_TEMP_5_EN, PAL_MODE_OUTPUT_PUSHPULL);
	palSetLineMode(LINE_TEMP_6_EN, PAL_MODE_OUTPUT_PUSHPULL);
	TEMP_MEASURE_ON();

	CURR_MEASURE_ON();
	HW_CAN_ON();

	chThdSleepMilliseconds(10);

	chThdCreateStatic(adc_thd_wa, sizeof(adc_thd_wa), NORMALPRIO, adc_thd, 0);
}

float pwr_get_vin(void) {
	return m_v_in;
}

float pwr_get_iin(void) {
	return m_i_in;
}

float pwr_get_temp(int sensor) {
	if (sensor < 0 || sensor >= HW_ADC_TEMP_SENSORS) {
		return -1.0;
	}

	return m_temps[sensor];
}
