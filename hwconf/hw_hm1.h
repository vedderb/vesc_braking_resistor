/*
	Copyright 2021 Benjamin Vedder	benjamin@vedder.se

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

#ifndef HWCONF_HW_HM1_H_
#define HWCONF_HW_HM1_H_

#define HW_NAME					"hm1"

// HW-specific
#define HW_INIT_HOOK()

#define CURR_MEASURE_ON()
#define CURR_MEASURE_OFF()

// Settings
#define HW_SHUNT_RES			(0.5e-3)
#define HW_SHUNT_AMP_GAIN		(20.0)
#define V_REG					3.3
#define R_IN_TOP				(520e3 + 2.5e3 + 100.0)
#define R_IN_BOTTOM				(10e3)

// LEDs
#define LINE_LED_RED			PAL_LINE(GPIOA, 9)
#define LINE_LED_GREEN			PAL_LINE(GPIOA, 10)

// CAN
#define LINE_CAN_RX				PAL_LINE(GPIOB, 8)
#define LINE_CAN_TX				PAL_LINE(GPIOB, 9)
#define HW_CAN_DEV				CAND1
#define HW_CAN_AF				9

// UART
#define LINE_UART_RX			PAL_LINE(GPIOC, 5)
#define LINE_UART_TX			PAL_LINE(GPIOC, 4)
#define HW_UART_DEV				SD3
#define HW_UART_AF				7
#define CONF_UART_BAUD_RATE		115200

// Analog
#define LINE_EX1				PAL_LINE(GPIOC, 4)
#define LINE_VIN				PAL_LINE(GPIOC, 4)
#define LINE_CURRENT			PAL_LINE(GPIOC, 0)
#define LINE_TEMP_0				PAL_LINE(GPIOC, 1)
#define LINE_TEMP_1				PAL_LINE(GPIOC, 2)
#define LINE_TEMP_2				PAL_LINE(GPIOC, 2)
#define LINE_TEMP_3				PAL_LINE(GPIOC, 2)
#define LINE_TEMP_4				PAL_LINE(GPIOB, 2)
#define LINE_TEMP_5				PAL_LINE(GPIOB, 2)

#define LINE_TEMP_0_EN			PAL_LINE(GPIOA, 2)
#define LINE_TEMP_1_EN			PAL_LINE(GPIOC, 8)
#define LINE_TEMP_2_EN			PAL_LINE(GPIOC, 9)
#define LINE_TEMP_3_EN			PAL_LINE(GPIOC, 10)
#define LINE_TEMP_4_EN			PAL_LINE(GPIOC, 11)
#define LINE_TEMP_5_EN			PAL_LINE(GPIOB, 2)

#define NTC_RES(adc)					(10000.0 / ((4095.0 / (float)adc) - 1.0))
#define NTC_RES_2(adc)					((4095.0 / (float)adc) * 10000.0 - 10000.0)
#define NTC_TEMP_WITH_IND(adc, ind)		(1.0 / ((logf(NTC_RES(adc) / 10000.0) / 3380.0) + (1.0 / 298.15)) - 273.15)

// ADC Channels
#define ADC_CH_CURRENT			ADC_CHANNEL_IN1
#define ADC_CH_TEMP0			ADC_CHANNEL_IN2
#define ADC_CH_TEMP1			ADC_CHANNEL_IN3
#define ADC_CH_TEMP2			ADC_CHANNEL_IN3
#define ADC_CH_TEMP3			ADC_CHANNEL_IN3
#define ADC_CH_TEMP4			ADC_CHANNEL_IN3
#define ADC_CH_TEMP5			ADC_CHANNEL_IN3
#define ADC_CH_EX1				ADC_CHANNEL_IN4
#define ADC_CH_VIN				ADC_CHANNEL_IN4

// Other
#define LINE_CURR_MEASURE_EN	PAL_LINE(GPIOB, 6)

#endif /* HWCONF_HW_HM1_H_ */
