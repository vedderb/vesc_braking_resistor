# VESC® Braking Resistor

This is the source code of the **VESC Braking Resistor** firmware. It is designed to integrate well with the [VESC motor control firmware](https://github.com/vedderb/bldc) as well as with [VESC Tool](https://github.com/vedderb/vesc_tool).

This is a very early attempt based on the VESC BMS and IO board firmwares, so there is a lot of functionality missing and several old settings still from the BMS. That will be fixed once I work in this project again.

At the moment it behaves like an IO board and can be connected over CAN to a VESC. Then, from VESC Tool on the VESC Dev Tools->QML page, the file VescToolUI.qml from this repository can be loaded to control the resistor while connected to the VESC over USB.