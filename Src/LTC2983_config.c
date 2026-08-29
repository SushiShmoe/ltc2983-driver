/*
 * LTC2983_config.c
 *
 *  Created on: Feb 21, 2026
 *      Author: vrnak
 */

#include "LTC2983.h"

extern SPI_HandleTypeDef hspi2;

LTC2983IfaceConfig_t ltc1IFaceConfig = {
		.hspi = &hspi2,
		.GpioChipSelect = {.Pin = LTC1_CS_Pin, .Port = LTC1_CS_GPIO_Port},
		.GpioReset = {.Pin = LTC1_RST_Pin, .Port = LTC1_RST_GPIO_Port},
		.GpioInterrupt = {.Pin = LTC1_EXTI_Pin, .Port = LTC1_EXTI_GPIO_Port}
};

const float rsenseResistorValue = 5050.0; // ohms
LTC2983ChannelConfig_t ltc1ChannelArray[5] = {
		{ // Rsense resistor
			.Channel = 	2,
			.Data = LTC2983_SENSOR_TYPE__SENSE_RESISTOR | (uint32_t)(rsenseResistorValue * 1024) // resolution per bit
		},
		{ // rtd1, sensor type and excitation current is an unknown
			.Channel = 4,
			.Data = LTC2983_SENSOR_TYPE__RTD_PT_1000 | LTC2983_RTD_RSENSE_CHANNEL__2 | LTC2983_RTD_EXCITATION_MODE__NO_ROTATION_SHARING \
			| LTC2983_RTD_EXCITATION_CURRENT__100UA | LTC2983_RTD_STANDARD__EUROPEAN
		},
		{
				.Channel = 6,
				.Data = LTC2983_SENSOR_TYPE__RTD_PT_1000 | LTC2983_RTD_RSENSE_CHANNEL__2 | LTC2983_RTD_EXCITATION_MODE__NO_ROTATION_SHARING \
				| LTC2983_RTD_EXCITATION_CURRENT__250UA | LTC2983_RTD_STANDARD__EUROPEAN
		},
		{
				.Channel = 8,
				.Data = LTC2983_SENSOR_TYPE__RTD_PT_1000 | LTC2983_RTD_RSENSE_CHANNEL__2 | LTC2983_RTD_EXCITATION_MODE__NO_ROTATION_SHARING \
				| LTC2983_RTD_EXCITATION_CURRENT__100UA | LTC2983_RTD_STANDARD__EUROPEAN
		},
		{
				.Channel = 10,
				.Data = LTC2983_SENSOR_TYPE__RTD_PT_1000 | LTC2983_RTD_RSENSE_CHANNEL__2 | LTC2983_RTD_EXCITATION_MODE__NO_ROTATION_SHARING \
				| LTC2983_RTD_EXCITATION_CURRENT__100UA | LTC2983_RTD_STANDARD__EUROPEAN
		}
};

LTC2983ChannelConfigs_t ltc1ChannelConfigs = {
		.Configs = ltc1ChannelArray,
		.Count = sizeof(ltc1ChannelArray) / sizeof(LTC2983ChannelConfig_t)
};

LTC2983ConvResult_t ltc1ConvResultArray[20] = {
		{ .Channel = 1,  .Status = 0, .Temperature = 0, .Raw = 0 },
		{ .Channel = 2,  .Status = 0, .Temperature = 0, .Raw = 0 },
		{ .Channel = 3,  .Status = 0, .Temperature = 0, .Raw = 0 },
		{ .Channel = 4,  .Status = 0, .Temperature = 0, .Raw = 0 },
		{ .Channel = 5,  .Status = 0, .Temperature = 0, .Raw = 0 },
		{ .Channel = 6,  .Status = 0, .Temperature = 0, .Raw = 0 },
		{ .Channel = 7,  .Status = 0, .Temperature = 0, .Raw = 0 },
		{ .Channel = 8,  .Status = 0, .Temperature = 0, .Raw = 0 },
		{ .Channel = 9,  .Status = 0, .Temperature = 0, .Raw = 0 },
		{ .Channel = 10, .Status = 0, .Temperature = 0, .Raw = 0 },
		{ .Channel = 11, .Status = 0, .Temperature = 0, .Raw = 0 },
		{ .Channel = 12, .Status = 0, .Temperature = 0, .Raw = 0 },
		{ .Channel = 13, .Status = 0, .Temperature = 0, .Raw = 0 },
		{ .Channel = 14, .Status = 0, .Temperature = 0, .Raw = 0 },
		{ .Channel = 15, .Status = 0, .Temperature = 0, .Raw = 0 },
		{ .Channel = 16, .Status = 0, .Temperature = 0, .Raw = 0 },
		{ .Channel = 17, .Status = 0, .Temperature = 0, .Raw = 0 },
		{ .Channel = 18, .Status = 0, .Temperature = 0, .Raw = 0 },
		{ .Channel = 19, .Status = 0, .Temperature = 0, .Raw = 0 },
		{ .Channel = 20, .Status = 0, .Temperature = 0, .Raw = 0 }
};

LTC2983ConvResults_t ltc1ConvResults = {
		.Results = ltc1ConvResultArray,
		.Count = sizeof(ltc1ConvResultArray) / sizeof(LTC2983ConvResult_t)
};

LTC2983RuntimeState_t ltc1State = {0};

LTC2983Handle_t ltc1Handle = {
		.IfaceConfig = &ltc1IFaceConfig,
		.ChannelConfigs = &ltc1ChannelConfigs,
		.Results = &ltc1ConvResults,
		.State = &ltc1State,
		.BitMask = 0b11111111111111111111,//1 << 3 | 1 << 5 | 1 << 7 | 1 << 9, // channel 4, 6, 8, 10
		.GlobalConfigurationRegister = LTC2983_REJECTION__50_60_HZ | LTC2983_TEMP_UNIT__C,
		.MuxConfigDelay = 10 // for 1ms delay, dunno why i just decided so
};

LTC2983Handle_t* handlesArray[1] = { &ltc1Handle };

LTC2983HandleRegistry_t handleRegistry = {
		.Handles = handlesArray,
		.Count = 1
};


