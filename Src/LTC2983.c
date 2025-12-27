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
#define LTC2983_4BYTE_MESSAGE_LENGHT    ((uint8_t) 0x07)
#define LTC2983_BYTE_MESSAGE_LENGHT     ((uint8_t) 0x04)


//**********************************************************************************************************
// -- GLOBAL VARIABLES --
//**********************************************************************************************************
static LTC2983HandleRegistry_t *globalHandleRegistry = NULL;

//**********************************************************************************************************
// -- PRIVATE FUNCTIONS --
//**********************************************************************************************************


/** SPI FUNCTIONS */

static void _LTC2983_WriteByte(LTC2983Handle_t * const handle, const LTC2983MemoryAddress_t address, uint8_t data){
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
	(void)HAL_SPI_TransmitReceive_DMA(hspi, txBuffer, rxBuffer, LTC2983_BYTE_MESSAGE_LENGHT);
}

static void _LTC2983_ReadByte(LTC2983Handle_t * const handle, LTC2983MemoryAddress_t address){
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
	(void)HAL_SPI_TransmitReceive_DMA(hspi, txBuffer, rxBuffer, LTC2983_BYTE_MESSAGE_LENGHT);
}

static void _LTC2983_Write4Bytes(LTC2983Handle_t * const handle, const LTC2983MemoryAddress_t address, uint32_t data){
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
	(void)HAL_SPI_TransmitReceive_DMA(hspi, txBuffer, rxBuffer, LTC2983_4BYTE_MESSAGE_LENGHT);
}

static void _LTC2983_Read4Bytes(LTC2983Handle_t * const handle, const LTC2983MemoryAddress_t address){
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
	(void)HAL_SPI_TransmitReceive_DMA(hspi, txBuffer, rxBuffer, LTC2983_4BYTE_MESSAGE_LENGHT);
}

/** LTC FUNCTIONS */

/** Get LTC2983 Channel Memory Start Address */
static LTC2983MemoryAddress_t _LTC2983_GetChannelStartAddress(const LTC2983MemoryAddress_t baseAddress, const LTC2983Channel_t channel)
{
    assert((LTC2983_CHANNEL_MIN <= channel) && (channel <= LTC2983_CHANNEL_MAX));

    return baseAddress + 4 * (channel - 1);
}

static void _LTC2983_FireCallback(const LTC2983Handle_t * const handle)
{
    if (handle->TaskDoneCallback)
    {
        handle->TaskDoneCallback();
    }
}


static void _LTC2983_WriteSingleChannelAssignmentData(LTC2983Handle_t * const handle, const LTC2983ChannelConfig_t * const config){ // TODO check if the const work
	assert(handle != NULL);
	assert(config != NULL);

	LTC2983Channel_t targetChannel = config->Channel;
	const LTC2983MemoryAddress_t channelAddress = LTC2983_GetChannelStartAddress(LTC2983_CH_ADDRESS_BASE, targetChannel);

	const uint8_t * const transferData = config->Data;
	_LTC2983_Write4Bytes(handle, channelAddress, transferData);
}

static void _LTC2983_ReadSingleChannelAssignmentData(LTC2983Handle_t * const handle, const LTC2983ChannelConfig_t * const config){
	assert(handle != NULL);
	assert(config != NULL);

	const LTC2983Channel_t targetChannel = config->Channel;
	const LTC2983MemoryAddress_t channelAddress = LTC2983_GetChannelStartAddress(LTC2983_CH_ADDRESS_BASE, targetChannel);

	_LTC2983_Read4Bytes(handle, channelAddress);
}

static void _LTC2983_Convert(LTC2983Handle_t * const handle, const LTC2983Channel_t channel){
	assert(handle != NULL);

	uint8_t data;
	data = LTC2983_COMMAND_STATUS_START;
	data |= channel;

	_LTC2983_WriteByte(handle, LTC2983_COMMAND_STATUS_REGISTER, data);
}

static void _LTC2983_ReadTemperatureResults(LTC2983Handle_t * const handle, const LTC2983Channel_t const channel){
	assert(handle != NULL);
	//assert(); channel >= 0 && < max

	const LTC2983MemoryAddress_t convChannelAddress = LTC2983_GetChannelStartAddress(LTC2938_CONVERSION_RESULT_MEMORY_BASE, channel);

	_LTC2983_Read4Bytes(handle, convChannelAddress);
}

static bool _LTC2983_IsChannelInTempResults(const LTC2983ConvResult_t * const results, const LTC2983Channel_t targetChannel){
	const LTC2983ChannelConfig_t * const results = results->Results;
	const uint8_t range = results->Count;
	for (int i = 0; i < range; i++){
		const LTC2983ConvResult_t * const result = &results[i]; // TODO double check pointers
		if (result->Channel == targetChannel){
			return true;
		}
	}

	return false;
}

static LTC2983ConvResult_t * _LTC2983_FindConvResult(const LTC2983ConvResult_t * const results, const LTC2983Channel_t targetChannel){
	const LTC2983ChannelConfig_t * const results = results->Results;
	const uint8_t range = results->Count;
	for (int i = 0; i < range; i++){
		const LTC2983ConvResult_t * const result = &results[i]; // TODO double check pointers
		if (result->Channel == targetChannel){
			return &result;
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

static bool _LTC2983_IsDriverOnSpiBusy(SPI_HandleTypeDef * const targetSpi){
	assert(targetSpi != NULL);
	assert(globalHandleRegistry != NULL);

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

//**********************************************************************************************************
// -- PUBLIC FUNCTIONS --
//**********************************************************************************************************

LTC2983RegistryStatus_t LTC2983_UnRegisterLTC2983HandleRegistry(void){
	assert(globalConfigRegistry != NULL);

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


			state->Status = LTC2983_GetDriverStatus(handle);
			state->Error = LTC2983_DRIVER_ERROR_NONE;
			state->WriteChannelsAssignmentDataIndex = 0;
			state->ReadChannelsAssignmentDataIndex = 0;
			state->ReadAllIndex = 0;
			state->ConvertAllIndex = 0;
			state->LastChannelRead = 0;


			state->Initialized = true;

			return LTC2983_DRIVER_STATUS_OKAY;
		}
	}

	return LTC2983_DRIVER_STATUS_ERROR;
}

void LTC2983_DeInit(void){

}

LTC2983DriverStatus_t LTC2983_GetDriverStatus(const LTC2983Handle_t * const handle){
	const LTC2983DriverStatus_t status = handle->Status;
	return status;
}

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

LTC2983DriverError_t LTC2983_GetDriverError(const LTC2983Handle_t * const handle){
	const LTC2983DriverError_t error = handle->Error;
	return error;
}

void LTC2983_RegisterTaskDoneCallback(const LTC2983Handle_t * const handle, const LTC2983TaskDoneCallback_t * const callback){
	assert(handle != NULL);

	LTC2983TaskDoneCallback_t * const tdcb = handle->TaskDoneCallback;
	tdcb = callback;
}

void LTC2983_UnRegisterTaskDoneCallback(const LTC2983Handle_t * const handle){ //TODO double check
	assert(handle != NULL);

	LTC2983TaskDoneCallback_t * const TaskDoneCallback = handle->TaskDoneCallback;
	TaskDoneCallback = NULL;
}

void LTC2983_StartUp(void){

}

void LTC2983_Sleep(void){

}

LTC2983DriverStatus_t LTC2983_WriteChannelsAssignmentData(LTC2983Handle_t * const handle){
	assert(handle != NULL);
	assert(handle->ChannelConfigs != NULL);
	assert(handle->ChannelConfigs->Count > 0);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_BUSY);
	assert(handle->State->Initialized != false);
	assert(handle->IfaceConfig != NULL);
	assert(_LTC2983_IsDriverOnSpiBusy(handle->IfaceConfig->hspi) == false);

	if (_LTC2983_IsDriverOnSpiBusy(handle->IfaceConfig->hspi) == true){
		handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_BUSY){
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_BUSY;
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Initialized == false){
		handle->State->Error = LTC2983_DRIVER_ERROR_NOT_INITIALIZED;
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	LTC2983RuntimeState_t * const pState = handle->State;

	pState->Status = LTC2983_DRIVER_STATUS_BUSY;
	pState->TaskState = TASK_STATE_WRITE_CHANNELS_ASSIGN_TRANSFER;

	const LTC2983ChannelConfig_t * const configs = handle->ChannelConfigs->Configs;
	const uint8_t channelsIndex = handle->State->WriteChannelsAssignmentDataIndex;

	const LTC2983ChannelConfig_t * const config = &configs[channelsIndex];
	_LTC2983_WriteSingleChannelAssignmentData(handle, config);

	return LTC2983_DRIVER_STATUS_OKAY;
}

LTC2983DriverStatus_t LTC2983_ReadChannelsAssignmentData(LTC2983Handle_t * const handle){
	assert(handle != NULL);
	assert(handle->ChannelConfigs != NULL);
	assert(handle->ChannelConfigs->Count > 0);
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_BUSY);
	assert(handle->State->Initialized != false);
	assert(handle->IfaceConfig != NULL);
	assert(_LTC2983_IsDriverOnSpiBusy(handle->IfaceConfig->hspi) == false);

	if (_LTC2983_IsDriverOnSpiBusy(handle->IfaceConfig->hspi) == true){
		handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		return LTC2983_DRIVER_STATUS_ERROR;
	}


	if (handle->State->Status == LTC2983_DRIVER_STATUS_BUSY){
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Initialized == false){
		handle->State->Error = LTC2983_DRIVER_ERROR_NOT_INITIALIZED;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	LTC2983RuntimeState_t * const pState = handle->State;

	pState->Status = LTC2983_DRIVER_STATUS_BUSY;
	pState->TaskState = TASK_STATE_READ_CHANNELS_ASSIGN_TRANSFER;

	const LTC2983ChannelConfig_t * const configs = handle->ChannelConfigs->Configs;
	const uint8_t channelsIndex = handle->State->ReadChannelsAssignmentDataIndex;

	const LTC2983ChannelConfig_t * const config = &configs[channelsIndex];
	_LTC2983_ReadSingleChannelAssignmentData(handle, config);

	return LTC2983_DRIVER_STATUS_OKAY;
}

void LTC2983_WriteGlobalConfigReg(const LTC2983GlobalConfigReg_t reg){

}

void LTC2983_ReadGlobalConfigReg(const LTC2983GlobalConfigReg_t * const reg){

}

void LTC2983_WriteMuxConfigDelay(const LTC2983MuxConfigDelay_t reg){

}

void LTC2983_ReadMuxConfigDelay(const LTC2983MuxConfigDelay_t * const reg){

}

void LTC2983_WriteMeasMultiChannelsMask(const LTC2983MeasMultiChannelsMask_t reg){

}

void LTC2983_ReadMeasMultiChannelsMask(const LTC2983MeasMultiChannelsMask_t * const reg){

}

LTC2983DriverStatus_t LTC2983_Convert(LTC2983Handle_t * const handle, const LTC2983Channel_t channel){
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_BUSY);
	assert(handle->State->Initialized != false);
	assert(_LTC2983_IsChannelInTempResults(handle->Results, channel) != false);
	assert(handle->IfaceConfig != NULL);
	assert(_LTC2983_IsDriverOnSpiBusy(handle->IfaceConfig->hspi) == false);

	if (_LTC2983_IsDriverOnSpiBusy(handle->IfaceConfig->hspi) == true){
		handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_BUSY){
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Initialized == false){
		handle->State->Error = LTC2983_DRIVER_ERROR_NOT_INITIALIZED;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (_LTC2983_IsChannelInTempResults(handle->Results, channel) == false && chanel != 0){
		handle->State->Error = LTC2983_DRIVER_ERROR_INVALID_CHANNEL;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	handle->State->Status = LTC2983_DRIVER_STATUS_BUSY;

	if (channel == 0){
		handle->State->TaskState = TASK_STATE_CONVERT_ALL_TRANSFER;
		handle->State->ConvertAllIndex = 0;
		LTC2983Channel_t * const resultChannel = handle->Results->Results[0].Channel;

		_LTC2983_Convert(handle, resultChannel);
	}
	else{
		handle->State->TaskState = TASK_STATE_CONVERT_TRANSFER;
		_LTC2983_Convert(handle, channel);
	}

	return LTC2983_DRIVER_STATUS_OKAY;
}

LTC2983DriverStatus_t LTC2983_ReadTemperatureResults(LTC2983Handle_t * const handle, const LTC2983Channel_t channel){
	assert(handle->State->Status != LTC2983_DRIVER_STATUS_BUSY);
	assert(handle->State->Initialized != false);
	assert(_LTC2983_IsChannelInTempResults(handle->Results, channel) != false);
	assert(handle->IfaceConfig != NULL);
	assert(_LTC2983_IsDriverOnSpiBusy(handle->IfaceConfig->hspi) == false);

	if (_LTC2983_IsDriverOnSpiBusy(handle->IfaceConfig->hspi) == true){
		handle->State->Error = LTC2983_DRIVER_ERROR_SPI_BUSY;
		handle->State->Status = LTC2983_DRIVER_STATUS_ERROR;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Status == LTC2983_DRIVER_STATUS_BUSY){
		handle->State->Error = LTC2983_DRIVER_ERROR_DEVICE_BUSY;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (handle->State->Initialized == false){
		handle->State->Error = LTC2983_DRIVER_ERROR_NOT_INITIALIZED;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	if (_LTC2983_IsChannelInTempResults(handle->Results, channel) == false && channel != 0){
		handle->State->Error = LTC2983_DRIVER_ERROR_INVALID_CHANNEL;
		return LTC2983_DRIVER_STATUS_ERROR;
	}

	handle->State->Status = LTC2983_DRIVER_STATUS_BUSY;

	if (channel == 0){
		handle->State->TaskState = TASK_STATE_TEMP_READ_ALL_RESULTS_TRANSFER;
		handle->State->ReadAllIndex = 0;
		LTC2983Channel_t * const resultChannel = handle->Results->Results[0].Channel;

		_LTC2983_ReadTemperatureResults(handle, resultChannel);
	}
	else{
		handle->State->TaskState = TASK_STATE_TEMP_READ_RESULTS_TRANSFER;
		handle->State->LastChannelRead = channel;

		_LTC2983_ReadTemperatureResults(handle, channel);
	}

	return LTC2983_DRIVER_STATUS_OKAY;
}

void LTC2983_ConvertAndReadTemperatureResults(const LTC2983Channel_t channel){
	assert(activeHandle != NULL);
	assert(activeHandle->Status != LTC2983_DRIVER_STATUS_BUSY);

	if (!LTC2983_IsChannelInConvResults(channel)){
		activeHandle->Error = LTC2983_DRIVER_ERROR_INVALID_CHANNEL;
		return;
	}


	activeHandle->Status = LTC2983_DRIVER_STATUS_BUSY;
	taskState = TASK_STATE_CONVERT_TEMP_READ_RESULTS_START;

	activeChannel = channel;

	_LTC2983_Convert(channel);
}


//**********************************************************************************************************
// -- TASK STATE/CALLBACK FUNCTIONS --
//**********************************************************************************************************


void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
	assert(globalHandleRegistry != NULL);

	LTC2983Handle_t * const handle = _LTC2983_GetHandleByHspi(hspi);

	if (handle == NULL){
		return;
	}

	const GpioChannel_t * const CSpin = &handle->IfaceConfig->GpioChipSelect;
	HAL_GPIO_WritePin(CSpin->Port, CSpin->Pin, GPIO_PIN_SET);

	LTC2983RuntimeState_t * const state = handle->State;

	switch (state->TaskState){
		case TASK_STATE_WRITE_CHANNELS_ASSIGN_TRANSFER: {
			uint8_t * const pIdx = &state->WriteChannelsAssignmentDataIndex;

			*pIdx = *pIdx + 1;

			const uint8_t channelsCount = handle->ChannelConfigs->Count;

			if (*pIdx < channelsCount){
				const LTC2983ChannelConfig_t * const config = &handle->ChannelConfigs->Configs[*pIdx];
				_LTC2983_WriteSingleChannelAssignmentData(handle, config);
			}
			else{
				*pIdx = 0;

				state->TaskState = TASK_STATE_IDLE;
				state->Status = LTC2983_DRIVER_STATUS_COMPLETE;
				state->Error = LTC2983_DRIVER_ERROR_NONE;
			}
			break;
		}
		case TASK_STATE_READ_CHANNELS_ASSIGN_TRANSFER:{
			uint8_t * const rxBuffer = state->RxBuffer;

			uint32_t config = 0;
			config |= ((uint32_t)rxBuffer[3]) << 24;
			config |= ((uint32_t)rxBuffer[4]) << 16;
			config |= ((uint32_t)rxBuffer[5]) << 8;
			config |= ((uint32_t)rxBuffer[6]);


			uint8_t * const pIdx = &state->WriteChannelsAssignmentDataIndex;
			handle->ChannelConfigs->Configs[*pIdx].Data = config;

			*pIdx = *pIdx + 1;

			const uint8_t channelsCount = handle->ChannelConfigs->Count;
			if (*pIdx < channelsCount){
				LTC2983ChannelConfig_t * const config = handle->ChannelConfigs->Configs[*pIdx];
				_LTC2983_ReadSingleChannelAssignmentData(handle, config);
			}
			else{
				*pIdx = 0;

				state->TaskState = TASK_STATE_IDLE;
				state->Status = LTC2983_DRIVER_STATUS_NONE;
			}
			break;
		}
		case TASK_STATE_CONVERT_TRANSFER: {
			state->TaskState = TASK_STATE_CONVERT_WAIT_HW;
			break;
		}
		case TASK_STATE_CONVERT_ALL_TRANSFER: {
			state->TaskState = TASK_STATE_CONVERT_ALL_WAIT_HW;
			break;
		}
		case TASK_STATE_TEMP_READ_RESULTS_TRANSFER:{
			uint8_t readIndex = state->LastChannelRead;
			LTC2983ConvResults_t * const result = handle->Results->Results[readIndex];

			LTC2983_ProcessRead(handle, state, result);

			state->TaskState = TASK_STATE_IDLE;
			state->Status = LTC2983_DRIVER_STATUS_COMPLETE;

			_LTC2983_FireCallback(handle);
		}
		case TASK_STATE_TEMP_READ_ALL_RESULTS_TRANSFER:{
			uint8_t * readIndex = &state->ReadAllIndex;
			LTC2983ConvResults_t * const results = handle->Results;

			LTC2983ConvResult_t * const result = &results->Results[*readIndex];
			_LTC2983_ProcessTempRead(handle, state, result);

			(*readIndex)++;

			if (*readIndex < results->Count){
				LTC2983ConvResult_t * const channel = &results->Results[*readIndex];

				_LTC2983_ReadTemperatureResults(handle, channel->Channel);
			}else{
				*readIndex = 0;

				state->TaskState = TASK_STATE_IDLE;
				state->Status = LTC2983_DRIVER_STATUS_COMPLETE;

				_LTC2983_FireCallback(handle);
			}
			break;
		}
		case TASK_STATE_CONVERT_TEMP_READ_RESULTS_START: {
			break;
		}
		case TASK_STATE_CONVERT_TEMP_READ_RESULTS_TRANSFER: {
			break;
		}
		default:
			break;
	}
}

static void LTC2983ExtiStateMachine();
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	assert(globalHandleRegistry != NULL);

	LTC2983Handle_t * const handle = _LTC2983_GetHandleByExtiPin(GPIO_PIN);

	if (handle == NULL){
		return;
	}

	const GpioChannel_t * const CSpin = &handle->IfaceConfig->GpioChipSelect;
	HAL_GPIO_WritePin(CSpin->Port, CSpin->Pin, GPIO_PIN_SET);

	LTC2983ExtiStateMachine(handle);
}

static void LTC2983ExtiStateMachine(LTC2983Handle_t * const handle){
	LTC2983RuntimeState_t * const state = handle->State;

	switch (state->TaskState){
		case TASK_STATE_CONVERT_WAIT_HW:{
			state->TaskState = TASK_STATE_IDLE;
			state->Status = LTC2983_DRIVER_STATUS_COMPLETE;

			_LTC2983_FireCallback(handle);
			break;
		}
		case TASK_STATE_CONVERT_ALL_WAIT_HW:{
			uint8_t * convIndex = &state->ConvertAllIndex;
			LTC2983ConvResults_t * const results = handle->Results;

			(*convIndex)++;

			if (*convIndex < results->Count){
				LTC2983ConvResult_t * const channel = &results->Results[*convIndex];

				state->TaskState = TASK_STATE_CONVERT_ALL_TRANSFER;

				_LTC2983Convert(handle, channel->Channel);
			}else{
				*convIndex = 0;

				state->TaskState = TASK_STATE_IDLE;
				state->Status = LTC2983_DRIVER_STATUS_COMPLETE;

				_LTC2983_FireCallback(handle);
			}

			break;
		}
		case TASK_STATE_CONVERT_TEMP_READ_RESULTS_WAIT_HW:{

			//TODO

			break;
		}
	}
}

void _LTC2983_ProcessTempRead(LTC2983Handle_t * const handle, LTC2983RuntimeState_t * const state, LTC2983ConvResult_t * const result){
	uint8_t * const rxBuffer = state->RxBuffer;

	uint8_t status = 0;
	status |= ((uint32_t)rxBuffer[3]) << 24;

	if (status & LTC2983_CONV_STATUS_VALID == LTC2983_CONV_STATUS_VALID){
		uint32_t temperature = 0;
		temperature |= ((uint32_t)rxBuffer[4]) << 16;
		temperature |= ((uint32_t)rxBuffer[5]) << 8;
		temperature |= ((uint32_t)rxBuffer[6]);

		result->Temperature = (float)temperature / 1024;
		result->Status = LTC2983_CONV_STATUS_VALID;
	}else{
		if ((status & LTC2983_CONV_STATUS_SENSOR_HARD_FAILURE) == LTC2983_CONV_STATUS_SENSOR_HARD_FAILURE){
			result->Status = LTC2983_CONV_STATUS_SENSOR_HARD_FAILURE;
		} else if ((status & LTC2983_CONV_STATUS_ADC_HARD_FAILURE) == LTC2983_CONV_STATUS_ADC_HARD_FAILURE){
			result->Status = LTC2983_CONV_STATUS_ADC_HARD_FAILURE;
		} else if ((status & LTC2983_CONV_STATUS_CJ_HARD_FAILURE) == LTC2983_CONV_STATUS_CJ_HARD_FAILURE){
			result->Status = LTC2983_CONV_STATUS_CJ_HARD_FAILURE;
		} else if ((status & LTC2983_CONV_STATUS_CJ_SOFT_FAILURE) == LTC2983_CONV_STATUS_CJ_SOFT_FAILURE){
			result->Status = LTC2983_CONV_STATUS_CJ_SOFT_FAILURE;
		} else if ((status & LTC2983_CONV_STATUS_SENSOR_ABOVE) == LTC2983_CONV_STATUS_SENSOR_ABOVE){
			result->Status = LTC2983_CONV_STATUS_SENSOR_ABOVE;
		} else if ((status & LTC2983_CONV_STATUS_SENSOR_BELOW) == LTC2983_CONV_STATUS_SENSOR_BELOW){
			result->Status = LTC2983_CONV_STATUS_SENSOR_BELOW;
		} else if ((status & LTC2983_CONV_STATUS_ADC_RANGE_ERROR) == LTC2983_CONV_STATUS_ADC_RANGE_ERROR){
			result->Status = LTC2983_CONV_STATUS_ADC_RANGE_ERROR;
		}
	}

	return;
}



