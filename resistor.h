/*
	Copyright 2021 Benjamin Vedder	benjamin@vedder.se

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

#ifndef RESISTOR_H_
#define RESISTOR_H_

// Functions
void resistor_init(void);
void resistor_set_pwm(float pwm);
float resistor_get_current_filtered(void);

#endif /* RESISTOR_H_ */
