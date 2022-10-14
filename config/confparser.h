// This file is autogenerated by VESC Tool

#ifndef CONFPARSER_H_
#define CONFPARSER_H_

#include "datatypes.h"
#include <stdint.h>
#include <stdbool.h>

// Constants
#define MAIN_CONFIG_T_SIGNATURE		2740403854

// Functions
int32_t confparser_serialize_main_config_t(uint8_t *buffer, const main_config_t *conf);
bool confparser_deserialize_main_config_t(const uint8_t *buffer, main_config_t *conf);
void confparser_set_defaults_main_config_t(main_config_t *conf);

// CONFPARSER_H_
#endif
