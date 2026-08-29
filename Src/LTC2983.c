/*
 * LTC2983.c
 *
 *  Created on: Dec 21, 2025
 *      Author: Vrnak Matyas
 */

#include <assert.h>
#include <stdbool.h>
#include "LTC2983.h"

/** LTC2983 Memory Address */
typedef uint16_t LTC2983MemoryAddress_t;

/** LTC2983 SPI Instruction */
typedef uint8_t LTC2983SpiInstruction_t;

/** Task Waiting time */
typedef uint16_t WaitingTime_t;

//**********************************************************************************************************
// -- ADDRESSES --
//**********************************************************************************************************
#define LTC2983_COMMAND_STATUS_REGISTER          ((LTC2983MemoryAddress_t) 0x0000)
#define LTC2983_GLOBAL_CONFIG_REGISTER           ((LTC2983MemoryAddress_t) 0x00F0)
#define LTC2983_MULTI_CHANNELS_MASK_REGISTER     ((LTC2983MemoryAddress_t) 0x00F4)
#define LTC2983_MUX_CONFIG_DELAY_REGISTER        ((LTC2983MemoryAddress_t) 0x00FF)
#define LTC2983_CH_ADDRESS_BASE                  ((LTC2983MemoryAddress_t) 0x0200)
#define LTC2983_VOUT_CH_BASE                     ((LTC2983MemoryAddress_t) 0x0060)
#define LTC2983_READ_CH_BASE                     ((LTC2983MemoryAddress_t) 0x0010)
#define LTC2983_CONVERSION_RESULT_MEMORY_BASE    ((LTC2983MemoryAddress_t) 0x0010)

//**********************************************************************************************************
// -- SPI INSTRUCTIONS --
//**********************************************************************************************************
#define LTC2983_WRITE_TO_RAM            ((LTC2983SpiInstruction_t) 0x02)
#define LTC2983_READ_FROM_RAM           ((LTC2983SpiInstruction_t) 0x03)

//**********************************************************************************************************
// -- COMMAND STATUS REGISTER --
//**********************************************************************************************************
#define LTC2983_COMMAND_STATUS_START    ((uint8_t) 0x80)
#define LTC2983_COMMAND_STATUS_DONE     ((uint8_t) 0x40)
#define LTC2983_COMMAND_STATUS_SLEEP    ((uint8_t) 0x17)

//**********************************************************************************************************
// -- DATA LENGHT --
//**********************************************************************************************************
#define LTC2983_4BYTE_MESSAGE_LENGTH    ((uint8_t) 0x07)
#define LTC2983_BYTE_MESSAGE_LENGTH     ((uint8_t) 0x04)


//**********************************************************************************************************
// -- GLOBAL VARIABLES --
//**********************************************************************************************************
static LTC2983HandleRegistry_t * globalHandleRegistry = NULL;

//**********************************************************************************************************
// -- PRIVATE FUNCTIONS --
//**********************************************************************************************************


/** SPI FUNCTIONS */

static HAL_StatusTypeDef _LTC2983_WriteByte(LTC2983Handle_t * const handle, const LTC2983MemoryAddress_t address, uint8_t data){
	assert(handle != NULL);

	const GpioChannel_t * const CSpin = &handle->IfaceConfig->GpioChipSelect;
	HAL_GPIO_WritePin(CSpin->Port, CSpin->Pin, GPIO_PIN_RESET);

	uint8_t * const txBuffer = handle->State->TxBuffer;
	uint8_t * const rxBuffer = handle->State->RxBuffer;

	txBuffer[0] = LTC2983_WRITE_TO_RAM;
	txBuffer[1] = (uint8_t)(address >> 8);
	txBuffer[2] = (uint8_t)(address & 0xFF);

	txBuffer[3] = data;

	SPI_HandleTypeDef * const hspi = handle->IfaceConfig->hspi;
	HAL_StatusTypeDef status = HAL_SPI_TransmitReceive_DMA(hspi, txBuffer, rxBuffer, LTC2983_BYTE_MESSAGE_LENGTH);

	return status;
}

static HAL_StatusTypeDef _LTC2983_ReadByte(LTC2983Handle_t * const handle, LTC2983MemoryAddress_t address){
	assert(handle != NULL);

	const GpioChannel_t * const CSpin = &handle->IfaceConfig->GpioChipSelect;
	HAL_GPIO_WritePin(CSpin->Port, CSpin->Pin, GPIO_PIN_RESET);

	uint8_t * const txBuffer = handle->State->TxBuffer;
	uint8_t * const rxBuffer = handle->State->RxBuffer;

	txBuffer[0] = LTC2983_READ_FROM_RAM;
	txBuffer[1] = (uint8_t)(address >> 8);
	txBuffer[2] = (uint8_t)(address & 0xFF);

	// Dummy byte for transaction
	txBuffer[3] = 0x0;

	SPI_HandleTypeDef * const hspi = handle->IfaceConfig->hspi;
	HAL_StatusTypeDef status = HAL_SPI_TransmitReceive_DMA(hspi, txBuffer, rxBuffer, LTC2983_BYTE_MESSAGE_LENGTH);

	return status;
}

static HAL_StatusTypeDef _LTC2983_Write4Bytes(LTC2983Handle_t * const handle, const LTC2983MemoryAddress_t address, uint32_t data){
	assert(handle != NULL);

	const GpioChannel_t * const CSpin = &handle->IfaceConfig->GpioChipSelect;
	HAL_GPIO_WritePin(CSpin->Port, CSpin->Pin, GPIO_PIN_RESET);

	uint8_t * const txBuffer = handle->State->TxBuffer;
	uint8_t * const rxBuffer = handle->State->RxBuffer;

	txBuffer[0] = LTC2983_WRITE_TO_RAM;
	txBuffer[1] = (uint8_t)(address >> 8);
	txBuffer[2] = (uint8_t)(address & 0xFF);

	txBuffer[3] = (uint8_t)(data >> 24);
	txBuffer[4] = (uint8_t)(data >> 16);
	txBuffer[5] = (uint8_t)(data >> 8);
	txBuffer[6] = (uint8_t)(data & 0xFF);

	SPI_HandleTypeDef * const hspi = handle->IfaceConfig->hspi;
	HAL_StatusTypeDef status = HAL_SPI_TransmitReceive_DMA(hspi, txBuffer, rxBuffer, LTC2983_4BYTE_MESSAGE_LENGTH);

	return status;
}

static HAL_StatusTypeDef _LTC2983_Read4Bytes(LTC2983Handle_t * const handle, const LTC2983MemoryAddress_t address){
	assert(handle != NULL);

	const GpioChannel_t * const CSpin = &handle->IfaceConfig->GpioChipSelect;
	HAL_GPIO_WritePin(CSpin->Port, CSpin->Pin, GPIO_PIN_RESET);

	uint8_t * const txBuffer = handle->State->TxBuffer;
	uint8_t * const rxBuffer = handle->State->RxBuffer;

	txBuffer[0] = LTC2983_READ_FROM_RAM;
	txBuffer[1] = (uint8_t)(address >> 8);
	txBuffer[2] = (uint8_t)(address & 0xFF);

	// Dummy bytes for transaction
	txBuffer[3] = 0x0;
	txBuffer[4] = 0x0;
	txBuffer[5] = 0x0;
	txBuffer[6] = 0x0;

	SPI_HandleTypeDef * const hspi = handle->IfaceConfig->hspi;
	HAL_StatusTypeDef status = HAL_SPI_TransmitReceive_DMA(hspi, txBuffer, rxBuffer, LTC2983_4BYTE_MESSAGE_LENGTH);

	return status;
}

/** LTC FUNCTIONS */

/** Get LTC2983 Channel Memory Start Address */
static LTC2983MemoryAddress_t _LTC2983_GetChannelStartAddress(const LTC2983MemoryAddress_t baseAddress, const LTC2983Channel_t channel)
{
    assert((LTC2983_CHANNEL_MIN <= channel) && (channel <= LTC2983_CHANNEL_MAX));

    return baseAddress + 4 * (channel - 1);
}

static void _LTC2983_FireCallback(LTC2983Handle_t * const handle)
{
    if (handle->TaskDoneCallback)
    {
        handle->TaskDoneCallback(handle);
    }
}


static HAL_StatusTypeDef _LTC2983_WriteSingleChannelAssignmentData(LTC2983Handle_t * const handle, const LTC2983ChannelConfig_t * const config){
	assert(handle != NULL);
	assert(config != NULL);

	LTC2983Channel_t targetChannel = config->Channel;
	const LTC2983MemoryAddress_t channelAddress = _LTC2983_GetChannelStartAddress(LTC2983_CH_ADDRESS_BASE, targetChannel);

	const LTC2983ChannelAssignmentData_t transferData = config->Data;
	HAL_StatusTypeDef status = _LTC2983_Write4Bytes(handle, channelAddress, transferData);

	return status;
}

static HAL_StatusTypeDef _LTC2983_ReadSingleChannelAssignmentData(LTC2983Handle_t * const handle, const LTC2983ChannelConfig_t * const config){
	assert(handle != NULL);
	assert(config != NULL);

	const LTC2983Channel_t targetChannel = config->Channel;
	const LTC2983MemoryAddress_t channelAddress = _LTC2983_GetChannelStartAddress(LTC2983_CH_ADDRESS_BASE, targetChannel);

	HAL_StatusTypeDef status = _LTC2983_Read4Bytes(handle, channelAddress);

	return status;
}

static HAL_StatusTypeDef _LTC2983_Convert(LTC2983Handle_t * const handle, const LTC2983Channel_t channel){
	assert(handle != NULL);

	uint8_t data = 0;
	data = LTC2983_COMMAND_STATUS_START;
	data |= channel;

	HAL_StatusTypeDef status = _LTC2983_WriteByte(handle, LTC2983_COMMAND_STATUS_REGISTER, data);

	return status;
}

static HAL_StatusTypeDef _LTC2983_ReadTemperatureResults(LTC2983Handle_t * const handle, const LTC2983Channel_t channel){
	assert(handle != NULL);
	//assert(); channel >= 0 && < max

	const LTC2983MemoryAddress_t convChannelAddress = _LTC2983_GetChannelStartAddress(LTC2983_CONVERSION_RESULT_MEMORY_BASE, channel);

	HAL_StatusTypeDef status = _LTC2983_Read4Bytes(handle, convChannelAddress);

	return status;
}

static void _LTC2983_ProcessTempRead(LTC2983Handle_t * const handle, LTC2983RuntimeState_t * const state, LTC2983ConvResult_t * const result){
	uint8_t * const rxBuffer = state->RxBuffer;

	uint8_t status = rxBuffer[3];

	if (((status & LTC2983_CONV_STATUS_VALID) == LTC2983_CONV_STATUS_VALID) && ((status & 0xFE) == 0)){
		int32_t tempRaw = 0;
		tempRaw |= ((uint32_t)rxBuffer[4]) << 16;
		tempRaw |= ((uint32_t)rxBuffer[5]) << 8;
		tempRaw |= ((uint32_t)rxBuffer[6]);

		if (tempRaw & 0x800000) {
			tempRaw |= 0xFF000000;
		}

		result->Temperature = (float)tempRaw / 1024.0f;
		result->Status = LTC2983_ENUM_CONV_STATUS_VALID;
	}else{
		if ((status & LTC2983_CONV_STATUS_SENSOR_HARD_FAILURE) == LTC2983_CONV_STATUS_SENSOR_HARD_FAILURE){
			result->Status = LTC2983_ENUM_CONV_STATUS_SENSOR_HARD_FAILURE;
		} else if ((status & LTC2983_CONV_STATUS_ADC_HARD_FAILURE) == LTC2983_CONV_STATUS_ADC_HARD_FAILURE){
			result->Status = LTC2983_ENUM_CONV_STATUS_ADC_HARD_FAILURE;
		} else if ((status & LTC2983_CONV_STATUS_CJ_HARD_FAILURE) == LTC2983_CONV_STATUS_CJ_HARD_FAILURE){
			result->Status = LTC2983_ENUM_CONV_STATUS_CJ_HARD_FAILURE;
		} else if ((status & LTC2983_CONV_STATUS_CJ_SOFT_FAILURE) == LTC2983_CONV_STATUS_CJ_SOFT_FAILURE){
			result->Status = LTC2983_ENUM_CONV_STATUS_CJ_SOFT_FAILURE;
		} else if ((status & LTC2983_CONV_STATUS_SENSOR_ABOVE) == LTC2983_CONV_STATUS_SENSOR_ABOVE){
			result->Status = LTC2983_ENUM_CONV_STATUS_SENSOR_ABOVE;
		} else if ((status & LTC2983_CONV_STATUS_SENSOR_BELOW) == LTC2983_CONV_STATUS_SENSOR_BELOW){
			result->Status = LTC2983_ENUM_CONV_STATUS_SENSOR_BELOW;
		} else if ((status & LTC2983_CONV_STATUS_ADC_RANGE_ERROR) == LTC2983_CONV_STATUS_ADC_RANGE_ERROR){
			result->Status = LTC2983_ENUM_CONV_STATUS_ADC_RANGE_ERROR;
		}else if ((status & LTC2983_CONV_STATUS_INVALID) == LTC2983_CONV_STATUS_INVALID){
			result->Status = LTC2983_ENUM_CONV_STATUS_INVALID;
		}
	}

	return;
}

static bool _LTC2983_IsChannelInTempResults(const LTC2983ConvResults_t * const results, const LTC2983Channel_t targetChannel){
	const LTC2983ConvResult_t * const realResults = results->Results;
	const uint8_t range = results->Count;
	for (int i = 0; i < range; i++){
		const LTC2983ConvResult_t * const curResult = &realResults[i];
		if (curResult->Channel == targetChannel){
			return true;
		}
	}

	return false;
}

static LTC2983ConvResult_t * _LTC2983_FindConvResult(LTC2983ConvResults_t * const results, const LTC2983Channel_t targetChannel){
	LTC2983ConvResult_t * const realResults = results->Results;
	const uint8_t range = results->Count;
	for (int i = 0; i < range; i++){
		LTC2983ConvResult_t * const curResult = &realResults[i];
		if (curResult->Channel == targetChannel){
			return curResult;
		}
	}

	return NULL;
}

static LTC2983Handle_t * _LTC2983_GetHandleByHspi(SPI_HandleTypeDef * const targetSpi){
	assert(targetSpi != NULL);
	assert(globalHandleRegistry != NULL);

	const uint8_t count = globalHandleRegistry->Count;
	for (int i = 0; i < count; i++){
		LTC2983Handle_t * const handle = globalHandleRegistry->Handles[i];

		const SPI_HandleTypeDef * const spi = handle->IfaceConfig->hspi;
		if (spi == targetSpi){
			const LTC2983DriverStatus_t status = handle->State->Status;

			if (status == LTC2983_DRIVER_STATUS_BUSY){
				return handle;
			}
		}
	}

	return NULL;
}

static LTC2983Handle_t * _LTC2983_GetHandleByExtiPin(uint16_t targetPin){
	assert(globalHandleRegistry != NULL);

	const uint8_t count = globalHandleRegistry->Count;
	for (int i = 0; i < count; i++){
		LTC2983Handle_t * const handle = globalHandleRegistry->Handles[i];

		const GpioChannel_t * const extiGPIO = &handle->IfaceConfig->GpioInterrupt;
		if (extiGPIO->Pin == targetPin){
			return handle;
		}
	}

	return NULL;
}

static bool _LTC2983_IsSpiBusy(SPI_HandleTypeDef * const targetSpi){
	assert(targetSpi != NULL);
	assert(globalHandleRegistry != NULL);

	if (targetSpi->State != HAL_SPI_STATE_READY){
		return true;
	}

	const uint8_t count = globalHandleRegistry->Count;
	for (int i = 0; i < count; i++){
		LTC2983Handle_t * const handle = globalHandleRegistry->Handles[i];

		const SPI_HandleTypeDef * const spi = handle->IfaceConfig->hspi;
		if (spi == targetSpi){
			const LTC2983DriverStatus_t status = handle->State->Status;

			if (status == LTC2983_DRIVER_STATUS_BUSY){
				return true;
			}
		}
	}

	return false;
}

void _LTC2983_Reset(LTC2983Handle_t * const handle){
	const GpioChannel_t * const RSpin = &handle->IfaceConfig->GpioReset;
	HAL_GPIO_WritePin(RSpin->Port, RSpin->Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(RSpin->Port, RSpin->Pin, GPIO_PIN_SET);
}

//**********************************************************************************************************
// -- PUBLIC FUNCTIONS --
//**********************************************************************************************************


LTC2983RegistryStatus_t LTC2983_RegisterLTC2983HandleRegistry(LTC2983HandleRegistry_t * const handleRegistry){
	assert(handleRegistry != NULL);
	assert(globalHandleRegistry == NULL);

	if (handleRegistry == NULL){
		return LTC2983_REGISTRY_STATUS_NO_POINTER;
	}

	if (globalHandleRegistry != NULL){
		return LTC2983_REGISTRY_STATUS_ALREADY_INITIALIZED;
	}

	globalHandleRegistry = handleRegistry;

	return LTC2983_REGISTRY_STATUS_OKAY;
}

LTC2983RegistryStatus_t LTC2983_UnRegisterLTC2983HandleRegistry(void){
	assert(globalHandleRegistry != NULL);

	globalHandleRegistry = NULL;

	return LTC2983_REGISTRY_STATUS_OKAY;
}

LTC2983DriverStatus_t LTC2983_Init(LTC2983Handle_t * const handle){
	assert(handle != NULL);
	assert(handle->State != NULL);
	assert(handle->IfaceConfig != NULL);

	LTC2983RuntimeState_t * const state = handle->State;
	const LTC2983IfaceConfig_t * const iface = handle->IfaceConfig;

	if (state != NULL && iface != NULL){
		if (state->Initialized != true){
			const GpioChannel_t * const CSpin = &handle->IfaceConfig->GpioChipSelect;
			HAL_GPIO_WritePin(CSpin->Port, CSpin->Pin, GPIO_PIN_SET);

			const GpioChannel_t * const RSpin = &handle->IfaceConfig->GpioReset;
			HAL_GPIO_WritePin(RSpin->Port, RSpin->Pin, GPIO_PIN_SET);


			state->Status = LTC2983_DRIVER_STATUS_SLEEP;
			state->Error = LTC2983_DRIVER_ERROR_NONE;
			state->TaskState = TASK_STATE_IDLE;
			state->WriteChannelsAssignmentDataIndex = 0;
			state->ReadChannelsAssignmentDataIndex = 0;
			state->ReadAllIndex = 0;
			state->LastChannelRead = 0;

			state->Reset = false;

			state->Initialized = true;
			state->StartupDone = false;
			state->ChannelsConfigured = false;

			handle->TaskDoneCallback = NULL;

			return LTC2983_DRIVER_STATUS_NONE;
		}
	}

	handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
	return LTC2983_DRIVER_STATUS_ERROR;
}

LTC2983DriverStatus_t LTC2983_GetDriverStatus(const LTC2983Handle_t * const handle){
	const LTC2983DriverStatus_t status = handle->State->Status;
	return status;
}

LTC2983DriverError_t LTC2983_GetDriverError(const LTC2983Handle_t * const handle){
	const LTC2983DriverError_t error = handle->State->Error;
	return error;
}

void LTC2983_RegisterTaskDoneCallback(LTC2983Handle_t * const handle, LTC2983TaskDoneCallback_t callback){
	assert(handle != NULL);

	handle->TaskDoneCallback = callback;
}

void LTC2983_UnRegisterTaskDoneCallback(LTC2983Handle_t * const handle){
	assert(handle != NULL);

	handle->TaskDoneCallback = NULL;
}

LTC2983DriverStatus_t LTC2983_StartUp(LTC2983Handle_t * const handle){
	//assert(handle != NULL);
	//assert(handle->State->Status != LTC2983_DRIVER_STATUS_BUSY);
	//assert(handle->State->Initialized != false);
	//assert(_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == false);

	if (_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == true){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_BUSY){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Initialized == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_NOT_INITIALIZED;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	LTC2983RuntimeState_t * const pState = handle->State;

	if (!pState->Reset){
		_LTC2983_Reset(handle);
		pState->Reset = true;
	}

	pState->Status = LTC2983_DRIVER_STATUS_BUSY;
	pState->TaskState = TASK_STATE_STARTUP_TRANSFER;

	HAL_StatusTypeDef status = _LTC2983_ReadByte(handle, LTC2983_COMMAND_STATUS_REGISTER);

	if (status != HAL_OK){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		switch (status){
			case HAL_ERROR:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_ERROR;
			} break;
			case HAL_BUSY:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
			} break;
			case HAL_TIMEOUT:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_TIMEOUT;
			} break;
		}

		handle->State->TaskState = TASK_STATE_IDLE;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	return LTC2983_DRIVER_STATUS_NONE;
}

LTC2983DriverStatus_t LTC2983_Sleep(LTC2983Handle_t * const handle){
	/*assert(handle != NULL);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_SLEEP);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_BUSY);
	assert(handle->State->Initialized != false);
	assert(handle->State->StartupDone != false);
	assert(_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == false);*/

	if (_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == true){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_BUSY){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_SLEEP){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_SLEEPING;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Initialized == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_NOT_INITIALIZED;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->StartupDone == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_SLEEPING;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	LTC2983RuntimeState_t * const pState = handle->State;

	pState->Status = LTC2983_DRIVER_STATUS_BUSY;
	pState->TaskState = TASK_STATE_SLEEP_TRANSFER;

	pState->StartupDone = false;
	pState->ChannelsConfigured = false;

	HAL_StatusTypeDef status = _LTC2983_WriteByte(handle, LTC2983_COMMAND_STATUS_REGISTER, LTC2983_COMMAND_STATUS_SLEEP);

	if (status != HAL_OK){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		switch (status){
			case HAL_ERROR:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_ERROR;
			} break;
			case HAL_BUSY:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
			} break;
			case HAL_TIMEOUT:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_TIMEOUT;
			} break;
		}

		handle->State->TaskState = TASK_STATE_IDLE;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	return LTC2983_DRIVER_STATUS_NONE;
}

LTC2983DriverStatus_t LTC2983_WriteChannelsAssignmentData(LTC2983Handle_t * const handle){
	/*assert(handle != NULL);
	assert(handle->ChannelConfigs != NULL);
	assert(handle->ChannelConfigs->Count > 0);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_BUSY);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_BUSY);
	assert(handle->State->Initialized != false);
	assert(handle->State->StartupDone != false);
	assert(handle->IfaceConfig != NULL);
	assert(_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == false);*/

	if (_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == true){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_BUSY){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_SLEEP){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_SLEEPING;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Initialized == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_NOT_INITIALIZED;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->StartupDone == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_SLEEPING;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	LTC2983RuntimeState_t * const pState = handle->State;

	pState->Status = LTC2983_DRIVER_STATUS_BUSY;
	pState->TaskState = TASK_STATE_WRITE_CHANNELS_ASSIGN_TRANSFER;

	const LTC2983ChannelConfig_t * const configs = handle->ChannelConfigs->Configs;
	const uint8_t channelsIndex = handle->State->WriteChannelsAssignmentDataIndex;

	const LTC2983ChannelConfig_t * const config = &configs[channelsIndex];

	HAL_StatusTypeDef status = _LTC2983_WriteSingleChannelAssignmentData(handle, config);

	if (status != HAL_OK){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		switch (status){
			case HAL_ERROR:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_ERROR;
			} break;
			case HAL_BUSY:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
			} break;
			case HAL_TIMEOUT:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_TIMEOUT;
			} break;
		}

		handle->State->TaskState = TASK_STATE_IDLE;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	return LTC2983_DRIVER_STATUS_NONE;
}

LTC2983DriverStatus_t LTC2983_ReadChannelsAssignmentData(LTC2983Handle_t * const handle){
	/*assert(handle != NULL);
	assert(handle->ChannelConfigs != NULL);
	assert(handle->ChannelConfigs->Count > 0);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_BUSY);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_BUSY);
	assert(handle->State->Initialized != false);
	assert(handle->State->StartupDone != false);
	assert(handle->IfaceConfig != NULL);
	assert(_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == false);*/

	if (_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == true){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_BUSY){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_SLEEP){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_SLEEPING;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Initialized == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_NOT_INITIALIZED;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->StartupDone == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_SLEEPING;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	LTC2983RuntimeState_t * const pState = handle->State;

	pState->Status = LTC2983_DRIVER_STATUS_BUSY;
	pState->TaskState = TASK_STATE_READ_CHANNELS_ASSIGN_TRANSFER;

	const LTC2983ChannelConfig_t * const configs = handle->ChannelConfigs->Configs;
	const uint8_t channelsIndex = handle->State->ReadChannelsAssignmentDataIndex;

	const LTC2983ChannelConfig_t * const config = &configs[channelsIndex];

	HAL_StatusTypeDef status = _LTC2983_ReadSingleChannelAssignmentData(handle, config);

	if (status != HAL_OK){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		switch (status){
			case HAL_ERROR:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_ERROR;
			} break;
			case HAL_BUSY:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
			} break;
			case HAL_TIMEOUT:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_TIMEOUT;
			} break;
		}

		handle->State->TaskState = TASK_STATE_IDLE;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	return LTC2983_DRIVER_STATUS_NONE;
}

LTC2983DriverStatus_t LTC2983_WriteGlobalConfigReg(LTC2983Handle_t * const handle){
	/*assert(handle != NULL);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_SLEEP);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_BUSY);
	assert(handle->State->Initialized != false);
	assert(handle->State->StartupDone != false);
	assert(handle->IfaceConfig != NULL);
	assert(_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == false);*/

	if (_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == true){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_BUSY){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_SLEEP){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_SLEEPING;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Initialized == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_NOT_INITIALIZED;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->StartupDone == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_SLEEPING;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	handle->State->Status = LTC2983_DRIVER_STATUS_BUSY;
	handle->State->TaskState = TASK_STATE_WRITE_BYTE_TRANSFER;

	LTC2983GlobalConfigReg_t globalConfig = handle->GlobalConfigurationRegister;

	HAL_StatusTypeDef status = _LTC2983_WriteByte(handle, LTC2983_GLOBAL_CONFIG_REGISTER, globalConfig);

	if (status != HAL_OK){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		switch (status){
			case HAL_ERROR:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_ERROR;
			} break;
			case HAL_BUSY:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
			} break;
			case HAL_TIMEOUT:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_TIMEOUT;
			} break;
		}

		handle->State->TaskState = TASK_STATE_IDLE;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	return LTC2983_DRIVER_STATUS_NONE;
}

LTC2983DriverStatus_t LTC2983_ReadGlobalConfigReg(LTC2983Handle_t * const handle){
	/*assert(handle != NULL);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_SLEEP);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_BUSY);
	assert(handle->State->Initialized != false);
	assert(handle->State->StartupDone != false);
	assert(handle->IfaceConfig != NULL);
	assert(_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == false);*/

	if (_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == true){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_BUSY){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_SLEEP){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_SLEEPING;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Initialized == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_NOT_INITIALIZED;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->StartupDone == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_SLEEPING;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	handle->State->Status = LTC2983_DRIVER_STATUS_BUSY;
	handle->State->TaskState = TASK_STATE_READ_GLOBAL_CONFIG_TRANSFER;

	HAL_StatusTypeDef status = _LTC2983_ReadByte(handle, LTC2983_GLOBAL_CONFIG_REGISTER);

	if (status != HAL_OK){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		switch (status){
			case HAL_ERROR:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_ERROR;
			} break;
			case HAL_BUSY:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
			} break;
			case HAL_TIMEOUT:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_TIMEOUT;
			} break;
		}

		handle->State->TaskState = TASK_STATE_IDLE;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	return LTC2983_DRIVER_STATUS_NONE;
}

LTC2983DriverStatus_t LTC2983_WriteMuxConfigDelay(LTC2983Handle_t * const handle){
	/*assert(handle != NULL);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_SLEEP);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_BUSY);
	assert(handle->State->Initialized != false);
	assert(handle->State->StartupDone != false);
	assert(handle->IfaceConfig != NULL);
	assert(_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == false);*/

	if (_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == true){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_BUSY){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_SLEEP){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_SLEEPING;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Initialized == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_NOT_INITIALIZED;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->StartupDone == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_SLEEPING;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	handle->State->Status = LTC2983_DRIVER_STATUS_BUSY;
	handle->State->TaskState = TASK_STATE_WRITE_BYTE_TRANSFER;

	LTC2983MuxConfigDelay_t muxConfig = handle->MuxConfigDelay;

	HAL_StatusTypeDef status = _LTC2983_WriteByte(handle, LTC2983_MUX_CONFIG_DELAY_REGISTER, muxConfig);

	if (status != HAL_OK){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		switch (status){
			case HAL_ERROR:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_ERROR;
			} break;
			case HAL_BUSY:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
			} break;
			case HAL_TIMEOUT:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_TIMEOUT;
			} break;
		}

		handle->State->TaskState = TASK_STATE_IDLE;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	return LTC2983_DRIVER_STATUS_NONE;
}

LTC2983DriverStatus_t LTC2983_ReadMuxConfigDelay(LTC2983Handle_t * const handle){
	/*assert(handle != NULL);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_SLEEP);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_BUSY);
	assert(handle->State->Initialized != false);
	assert(handle->State->StartupDone != false);
	assert(handle->IfaceConfig != NULL);
	assert(_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == false);*/

	if (_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == true){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_BUSY){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_SLEEP){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_SLEEPING;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Initialized == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_NOT_INITIALIZED;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->StartupDone == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_SLEEPING;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	handle->State->Status = LTC2983_DRIVER_STATUS_BUSY;
	handle->State->TaskState = TASK_STATE_READ_MUX_CONFIG_TRANSFER;

	HAL_StatusTypeDef status = _LTC2983_ReadByte(handle, LTC2983_MUX_CONFIG_DELAY_REGISTER);

	if (status != HAL_OK){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		switch (status){
			case HAL_ERROR:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_ERROR;
			} break;
			case HAL_BUSY:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
			} break;
			case HAL_TIMEOUT:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_TIMEOUT;
			} break;
		}

		handle->State->TaskState = TASK_STATE_IDLE;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	return LTC2983_DRIVER_STATUS_NONE;
}

LTC2983DriverStatus_t LTC2983_WriteMeasMultiChannelsMask(LTC2983Handle_t * const handle){
	/*assert(handle != NULL);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_SLEEP);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_BUSY);
	assert(handle->State->Initialized != false);
	assert(handle->State->StartupDone != false);
	assert(handle->IfaceConfig != NULL);
	assert(_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == false);*/

	if (_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == true){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_BUSY){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_SLEEP){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_SLEEPING;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Initialized == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_NOT_INITIALIZED;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->StartupDone == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_SLEEPING;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	handle->State->Status = LTC2983_DRIVER_STATUS_BUSY;
	handle->State->TaskState = TASK_STATE_WRITE_4BYTES_TRANSFER;

	LTC2983MeasMultiChannelsMask_t mask = handle->BitMask;
	HAL_StatusTypeDef status = _LTC2983_Write4Bytes(handle, LTC2983_MULTI_CHANNELS_MASK_REGISTER, mask);

	if (status != HAL_OK){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		switch (status){
			case HAL_ERROR:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_ERROR;
			} break;
			case HAL_BUSY:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
			} break;
			case HAL_TIMEOUT:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_TIMEOUT;
			} break;
		}

		handle->State->TaskState = TASK_STATE_IDLE;

		return LTC2983_DRIVER_STATUS_ERROR;
	}

	return LTC2983_DRIVER_STATUS_NONE;
}

LTC2983DriverStatus_t LTC2983_ReadMeasMultiChannelsMask(LTC2983Handle_t * const handle){
	/*assert(handle != NULL);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_SLEEP);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_BUSY);
	assert(handle->State->Initialized != false);
	assert(handle->State->StartupDone != false);
	assert(handle->IfaceConfig != NULL);
	assert(_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == false);*/

	if (_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == true){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_BUSY){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_SLEEP){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_SLEEPING;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Initialized == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_NOT_INITIALIZED;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->StartupDone == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_SLEEPING;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	handle->State->Status = LTC2983_DRIVER_STATUS_BUSY;
	handle->State->TaskState = TASK_STATE_READ_MULTIMASK_TRANSFER;

	HAL_StatusTypeDef status = _LTC2983_Read4Bytes(handle, LTC2983_MULTI_CHANNELS_MASK_REGISTER);

	if (status != HAL_OK){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		switch (status){
			case HAL_ERROR:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_ERROR;
			} break;
			case HAL_BUSY:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
			} break;
			case HAL_TIMEOUT:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_TIMEOUT;
			} break;
		}

		handle->State->TaskState = TASK_STATE_IDLE;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	return LTC2983_DRIVER_STATUS_NONE;
}

LTC2983DriverStatus_t LTC2983_Convert(LTC2983Handle_t * const handle, const LTC2983Channel_t channel){
	/*assert(handle != NULL);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_BUSY);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_SLEEP);
	assert(handle->State->Initialized != false);
	assert(handle->State->StartupDone != false);
	assert(_LTC2983_IsChannelInTempResults(handle->Results, channel) != false || channel == LTC2983_MULTIPLE_CHANNELS);
	assert(handle->IfaceConfig != NULL);
	assert(_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == false);*/

	if (_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == true){
		handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_BUSY){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_SLEEP){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_SLEEPING;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Initialized == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_NOT_INITIALIZED;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->StartupDone == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_SLEEPING;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (_LTC2983_IsChannelInTempResults(handle->Results, channel) == false && channel != LTC2983_MULTIPLE_CHANNELS){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_INVALID_CHANNEL;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	handle->State->Status = LTC2983_DRIVER_STATUS_BUSY;

	handle->State->TaskState = TASK_STATE_CONVERT_TRANSFER;

	HAL_StatusTypeDef status = _LTC2983_Convert(handle, channel);

	if (status != HAL_OK){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		switch (status){
			case HAL_ERROR:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_ERROR;
			} break;
			case HAL_BUSY:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
			} break;
			case HAL_TIMEOUT:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_TIMEOUT;
			} break;
		}

		handle->State->TaskState = TASK_STATE_IDLE;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	return LTC2983_DRIVER_STATUS_NONE;
}

LTC2983DriverStatus_t LTC2983_ReadTemperatureResults(LTC2983Handle_t * const handle, const LTC2983Channel_t channel){
	/*assert(handle != NULL);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_BUSY);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_SLEEP);
	assert(handle->State->Initialized != false);
	assert(handle->State->StartupDone != false);
	assert(_LTC2983_IsChannelInTempResults(handle->Results, channel) != false || channel == LTC2983_MULTIPLE_CHANNELS);
	assert(handle->IfaceConfig != NULL);
	assert(_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == false);*/

	if (_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == true){
		handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_BUSY){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_SLEEP){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_SLEEPING;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Initialized == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_NOT_INITIALIZED;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->StartupDone == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_SLEEPING;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (_LTC2983_IsChannelInTempResults(handle->Results, channel) == false && channel != LTC2983_MULTIPLE_CHANNELS){
		handle->State->Error = LTC2983_DRIVER_ERROR_INVALID_CHANNEL;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	handle->State->Status = LTC2983_DRIVER_STATUS_BUSY;

	if (channel == LTC2983_MULTIPLE_CHANNELS){
		handle->State->TaskState = TASK_STATE_TEMP_READ_ALL_RESULTS_TRANSFER;

		LTC2983MeasMultiChannelsMask_t multimask = handle->BitMask;
		for (uint8_t ch = 0; ch < LTC2983_CHANNEL_MAX; ch++){
			if (multimask & (1 << ch)){
				uint8_t targetChannel = ch + 1;

				handle->State->LastChannelRead = targetChannel;

				HAL_StatusTypeDef status = _LTC2983_ReadTemperatureResults(handle, targetChannel);

				if (status != HAL_OK){
					handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
					switch (status){
						case HAL_ERROR:{
							handle->State->Error = LTC2983_DRIVER_ERROR_SPI_ERROR;
						} break;
						case HAL_BUSY:{
							handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
						} break;
						case HAL_TIMEOUT:{
							handle->State->Error = LTC2983_DRIVER_ERROR_SPI_TIMEOUT;
						} break;
					}

					handle->State->TaskState = TASK_STATE_IDLE;
					return LTC2983_DRIVER_STATUS_ERROR;
				}

				break;
			}
		}
	} else {
		handle->State->TaskState = TASK_STATE_TEMP_READ_RESULTS_TRANSFER;

		handle->State->LastChannelRead = channel;

		HAL_StatusTypeDef status = _LTC2983_ReadTemperatureResults(handle, channel);

		if (status != HAL_OK){
			handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
			switch (status){
				case HAL_ERROR:{
					handle->State->Error = LTC2983_DRIVER_ERROR_SPI_ERROR;
				} break;
				case HAL_BUSY:{
					handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
				} break;
				case HAL_TIMEOUT:{
					handle->State->Error = LTC2983_DRIVER_ERROR_SPI_TIMEOUT;
				} break;
			}

			handle->State->TaskState = TASK_STATE_IDLE;
			return LTC2983_DRIVER_STATUS_ERROR;
		}
	}

	return LTC2983_DRIVER_STATUS_NONE;
}

LTC2983DriverStatus_t LTC2983_ReadRawVoltage(LTC2983Handle_t * const handle, const LTC2983Channel_t channel){
	/*assert(handle != NULL);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_BUSY);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_SLEEP);
	assert(handle->State->Initialized != false);
	assert(handle->State->StartupDone != false);
	assert(_LTC2983_IsChannelInTempResults(handle->Results, channel) != false || channel == LTC2983_MULTIPLE_CHANNELS);
	assert(handle->IfaceConfig != NULL);
	assert(_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == false);*/

	if (_LTC2983_IsSpiBusy(handle->IfaceConfig->hspi) == true){
		handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_BUSY){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_SLEEP){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_SLEEPING;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Initialized == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_NOT_INITIALIZED;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->StartupDone == false){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_SLEEPING;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (_LTC2983_IsChannelInTempResults(handle->Results, channel) == false && channel != LTC2983_MULTIPLE_CHANNELS){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		handle->State->Error = LTC2983_DRIVER_ERROR_INVALID_CHANNEL;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	handle->State->Status = LTC2983_DRIVER_STATUS_BUSY;

	handle->State->TaskState = TASK_STATE_READ_RAW_VOLTAGE_TRANSFER;

	handle->State->LastChannelRead = channel;

	uint32_t mem_offset = _LTC2983_GetChannelStartAddress(LTC2983_VOUT_CH_BASE, channel);

	HAL_StatusTypeDef status = _LTC2983_Read4Bytes(handle, mem_offset);

	if (status != HAL_OK){
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		switch (status){
			case HAL_ERROR:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_ERROR;
			} break;
			case HAL_BUSY:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
			} break;
			case HAL_TIMEOUT:{
				handle->State->Error = LTC2983_DRIVER_ERROR_SPI_TIMEOUT;
			} break;
		}

		handle->State->TaskState = TASK_STATE_IDLE;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	return LTC2983_DRIVER_STATUS_NONE;
}


//**********************************************************************************************************
// -- TASK STATE/CALLBACK FUNCTIONS --
//**********************************************************************************************************


void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
	assert(globalHandleRegistry != NULL);

	LTC2983Handle_t * const handle = _LTC2983_GetHandleByHspi(hspi);

	if (!handle){
		return;
	}

	const GpioChannel_t * const CSpin = &handle->IfaceConfig->GpioChipSelect;
	HAL_GPIO_WritePin(CSpin->Port, CSpin->Pin, GPIO_PIN_SET);

	LTC2983RuntimeState_t * const state = handle->State;

	switch (state->TaskState){
		case TASK_STATE_STARTUP_TRANSFER: {
			uint8_t * const rxBuffer = state->RxBuffer;

			uint8_t response = rxBuffer[3];

			if ((response & LTC2983_COMMAND_STATUS_DONE) == LTC2983_COMMAND_STATUS_DONE){
				state->Status = LTC2983_DRIVER_STATUS_NONE;
				state->StartupDone = true;
				state->Reset = false;
			}
			else {
				state->Status = LTC2983_DRIVER_STATUS_SLEEP;
			}

			state->TaskState = TASK_STATE_IDLE;

			_LTC2983_FireCallback(handle);
			break;
		}
		case TASK_STATE_SLEEP_TRANSFER: {
			state->TaskState = TASK_STATE_IDLE;
			state->Status = LTC2983_DRIVER_STATUS_SLEEP;
			state->StartupDone = false;

			_LTC2983_FireCallback(handle);
			break;
		}
		case TASK_STATE_WRITE_BYTE_TRANSFER: {
			state->TaskState = TASK_STATE_IDLE;
			state->Status = LTC2983_DRIVER_STATUS_NONE;

			_LTC2983_FireCallback(handle);
			break;
		}
		case TASK_STATE_READ_GLOBAL_CONFIG_TRANSFER: {
			uint8_t * const rxBuffer = state->RxBuffer;

			LTC2983GlobalConfigReg_t rxGlobalConfig = rxBuffer[3];

			handle->GlobalConfigurationRegister = rxGlobalConfig;

			state->TaskState = TASK_STATE_IDLE;
			state->Status = LTC2983_DRIVER_STATUS_NONE;

			_LTC2983_FireCallback(handle);
			break;
		}
		case TASK_STATE_READ_MUX_CONFIG_TRANSFER: {
			uint8_t * const rxBuffer = state->RxBuffer;

			LTC2983MuxConfigDelay_t rxMux = rxBuffer[3];

			handle->MuxConfigDelay = rxMux;

			state->TaskState = TASK_STATE_IDLE;
			state->Status = LTC2983_DRIVER_STATUS_NONE;

			_LTC2983_FireCallback(handle);
			break;
		}
		case TASK_STATE_WRITE_4BYTES_TRANSFER: {
			state->TaskState = TASK_STATE_IDLE;
			state->Status = LTC2983_DRIVER_STATUS_NONE;

			_LTC2983_FireCallback(handle);
			break;
		}
		case TASK_STATE_READ_MULTIMASK_TRANSFER: {
			uint8_t * const rxBuffer = state->RxBuffer;

			LTC2983MeasMultiChannelsMask_t rxMask = 0;
			rxMask |= ((uint32_t)rxBuffer[4] << 16);
			rxMask |= ((uint32_t)rxBuffer[5] << 8);
			rxMask |= ((uint32_t)rxBuffer[6]);

			handle->BitMask = rxMask;

			state->TaskState = TASK_STATE_IDLE;
			state->Status = LTC2983_DRIVER_STATUS_NONE;

			_LTC2983_FireCallback(handle);
			break;
		}
		case TASK_STATE_WRITE_CHANNELS_ASSIGN_TRANSFER: {
			volatile uint8_t * const pIdx = &state->WriteChannelsAssignmentDataIndex;

			*pIdx = *pIdx + 1;

			const uint8_t channelsCount = handle->ChannelConfigs->Count;

			if (*pIdx < channelsCount){
				const LTC2983ChannelConfig_t * const config = &handle->ChannelConfigs->Configs[*pIdx];
				HAL_StatusTypeDef status = _LTC2983_WriteSingleChannelAssignmentData(handle, config);

				if (status != HAL_OK){
					handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
					switch (status){
						case HAL_ERROR:{
							handle->State->Error = LTC2983_DRIVER_ERROR_SPI_ERROR;
						} break;
						case HAL_BUSY:{
							handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
						} break;
						case HAL_TIMEOUT:{
							handle->State->Error = LTC2983_DRIVER_ERROR_SPI_TIMEOUT;
						} break;
					}

					handle->State->TaskState = TASK_STATE_IDLE;
					_LTC2983_FireCallback(handle);
				}
			}
			else{
				*pIdx = 0;

				state->ChannelsConfigured = true;
				state->TaskState = TASK_STATE_IDLE;
				state->Status = LTC2983_DRIVER_STATUS_COMPLETE;
				state->Error = LTC2983_DRIVER_ERROR_NONE;

				_LTC2983_FireCallback(handle);
			}
			break;
		}
		case TASK_STATE_READ_CHANNELS_ASSIGN_TRANSFER:{
			uint8_t * const rxBuffer = state->RxBuffer;

			if (state->Status == LTC2983_DRIVER_STATUS_ERROR){
				break;
			}

			uint32_t config = 0;
			config |= ((uint32_t)rxBuffer[3]) << 24;
			config |= ((uint32_t)rxBuffer[4]) << 16;
			config |= ((uint32_t)rxBuffer[5]) << 8;
			config |= ((uint32_t)rxBuffer[6]);


			volatile uint8_t * const pIdx = &state->ReadChannelsAssignmentDataIndex;
			handle->ChannelConfigs->Configs[*pIdx].Data = config;

			*pIdx = *pIdx + 1;

			const uint8_t channelsCount = handle->ChannelConfigs->Count;
			if (*pIdx < channelsCount){
				LTC2983ChannelConfig_t * config = &handle->ChannelConfigs->Configs[*pIdx];
				HAL_StatusTypeDef status = _LTC2983_ReadSingleChannelAssignmentData(handle, config);

				if (status != HAL_OK){
					handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
					switch (status){
						case HAL_ERROR:{
							handle->State->Error = LTC2983_DRIVER_ERROR_SPI_ERROR;
						} break;
						case HAL_BUSY:{
							handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
						} break;
						case HAL_TIMEOUT:{
							handle->State->Error = LTC2983_DRIVER_ERROR_SPI_TIMEOUT;
						} break;
					}
					*pIdx = 0;

					state->TaskState = TASK_STATE_IDLE;
					_LTC2983_FireCallback(handle);
				}
			}
			else{
				*pIdx = 0;

				state->TaskState = TASK_STATE_IDLE;
				state->Status = LTC2983_DRIVER_STATUS_NONE;
				state->Error = LTC2983_DRIVER_ERROR_NONE;

				_LTC2983_FireCallback(handle);
			}
			break;
		}
		case TASK_STATE_CONVERT_TRANSFER: {
			state->TaskState = TASK_STATE_CONVERT_WAIT_HW;

			//_LTC2983_FireCallback(handle);
			break;
		}
		case TASK_STATE_TEMP_READ_RESULTS_TRANSFER:{
			uint8_t lastChannel = state->LastChannelRead;

			if (state->Status == LTC2983_DRIVER_STATUS_ERROR){
				state->TaskState = TASK_STATE_IDLE;
				break;
			}

			LTC2983ConvResult_t * const result = _LTC2983_FindConvResult(handle->Results, lastChannel);

			if (!result){
				state->TaskState = TASK_STATE_IDLE;
				state->Status = LTC2983_DRIVER_STATUS_ERROR;
				state->Error = LTC2983_DRIVER_ERROR_INVALID_CONFIGS;
			}

			_LTC2983_ProcessTempRead(handle, state, result);

			state->TaskState = TASK_STATE_IDLE;
			state->Status = LTC2983_DRIVER_STATUS_COMPLETE;

			_LTC2983_FireCallback(handle);
			break;
		}
		case TASK_STATE_TEMP_READ_ALL_RESULTS_TRANSFER:{
			uint8_t lastChannel = state->LastChannelRead;

			if (state->Status == LTC2983_DRIVER_STATUS_ERROR){
				state->TaskState = TASK_STATE_IDLE;
				break;
			}

			LTC2983ConvResult_t * const result = _LTC2983_FindConvResult(handle->Results, lastChannel);

			if (!result){
				state->TaskState = TASK_STATE_IDLE;
				state->Status = LTC2983_DRIVER_STATUS_ERROR;
				state->Error = LTC2983_DRIVER_ERROR_INVALID_CONFIGS;
				break;
			}

			_LTC2983_ProcessTempRead(handle, state, result);


			LTC2983MeasMultiChannelsMask_t multimask = handle->BitMask;
			bool sent = false;
			for (uint8_t ch = lastChannel; ch < LTC2983_CHANNEL_MAX; ch++){
				if (multimask & (1 << ch)){
					uint8_t targetChannel = ch + 1;

					state->LastChannelRead = targetChannel;

					HAL_StatusTypeDef status = _LTC2983_ReadTemperatureResults(handle, targetChannel);

					if (status != HAL_OK){
						handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
						switch (status){
							case HAL_ERROR:{
								handle->State->Error = LTC2983_DRIVER_ERROR_SPI_ERROR;
							} break;
							case HAL_BUSY:{
								handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
							} break;
							case HAL_TIMEOUT:{
								handle->State->Error = LTC2983_DRIVER_ERROR_SPI_TIMEOUT;
							} break;
						}

						state->TaskState = TASK_STATE_IDLE;
						sent = true;
						_LTC2983_FireCallback(handle);
						break;
					}

					sent = true;

					break;
				}
			}

			if (!sent){ // Sets to done if all of the channels were read from LTC
				state->TaskState = TASK_STATE_IDLE;
				state->Status = LTC2983_DRIVER_STATUS_COMPLETE;

				_LTC2983_FireCallback(handle);
			}

			break;
		}
		case TASK_STATE_READ_RAW_VOLTAGE_TRANSFER:{
			uint8_t lastChannel = state->LastChannelRead;

			if (state->Status == LTC2983_DRIVER_STATUS_ERROR){
				state->TaskState = TASK_STATE_IDLE;
				break;
			}

			uint8_t* rxBuffer = handle->State->RxBuffer;

			uint32_t voltage = ((uint32_t)rxBuffer[3]) << 24 \
					 	 	 | ((uint32_t)rxBuffer[4]) << 16 \
							 | ((uint32_t)rxBuffer[5]) << 8 \
							 | ((uint32_t)rxBuffer[6]);

			float raw = (float)voltage / 1024;

			LTC2983ConvResult_t * const result = &handle->Results->Results[lastChannel-1];//_LTC2983_FindConvResult(handle->Results, lastChannel);

			result->Raw = raw;

			state->TaskState = TASK_STATE_IDLE;
			state->Status = LTC2983_DRIVER_STATUS_COMPLETE;

			_LTC2983_FireCallback(handle);
			break;
		}
		default:
			break;
	}
}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin) {
	assert(globalHandleRegistry != NULL);

	LTC2983Handle_t * const handle = _LTC2983_GetHandleByExtiPin(GPIO_Pin);

	if (handle == NULL){
		return;
	}

	LTC2983RuntimeState_t * const state = handle->State;

	switch (state->TaskState){
		case TASK_STATE_CONVERT_TRANSFER:
		case TASK_STATE_CONVERT_WAIT_HW:{
			state->TaskState = TASK_STATE_IDLE;
			state->Status = LTC2983_DRIVER_STATUS_COMPLETE;

			_LTC2983_FireCallback(handle);
			break;
		}
	}
}



