/*
 * LTC2983.h
 *
 *  Created on: Dec 20, 2025
 *      Author: Vrnak Matyas
 */

#ifndef INC_LTC2983_H_
#define INC_LTC2983_H_

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/** Driver task state */
typedef enum{
	TASK_STATE_IDLE,
	// Start-Up && sleep
	TASK_STATE_STARTUP_TRANSFER,//
	TASK_STATE_SLEEP_TRANSFER,//
	// Write Segment byte
	TASK_STATE_WRITE_BYTE_TRANSFER,
	// Read Segment byte of global config register
	TASK_STATE_READ_GLOBAL_CONFIG_TRANSFER,
	// Read Segment byte of mux config
	TASK_STATE_READ_MUX_CONFIG_TRANSFER,
	// Write Segment 4 bytes
	TASK_STATE_WRITE_4BYTES_TRANSFER,
	// Read Segment 4 bytes of multi bit mask
	TASK_STATE_READ_MULTIMASK_TRANSFER,
	// Write Channel Assignment Data
	TASK_STATE_WRITE_CHANNELS_ASSIGN_TRANSFER,
	// Read Channel Assignment Data
	TASK_STATE_READ_CHANNELS_ASSIGN_TRANSFER,
	// Convert
	TASK_STATE_CONVERT_TRANSFER,
	TASK_STATE_CONVERT_WAIT_HW,
	// Read Temperature Results
	TASK_STATE_TEMP_READ_RESULTS_TRANSFER,
	TASK_STATE_TEMP_READ_ALL_RESULTS_TRANSFER,

	// Debug
	TASK_STATE_READ_RAW_VOLTAGE_TRANSFER,
} LTC2983TaskState_t;


/** Driver status */
typedef enum {
	LTC2983_DRIVER_STATUS_NONE,
	LTC2983_DRIVER_STATUS_SLEEP,
	LTC2983_DRIVER_STATUS_BUSY,
	LTC2983_DRIVER_STATUS_COMPLETE,
	LTC2983_DRIVER_STATUS_ERROR,
} LTC2983DriverStatus_t;

/** Driver error */
typedef enum {
	LTC2983_DRIVER_ERROR_NONE,
    LTC2983_DRIVER_ERROR_DEVICE_SLEEPING,
    LTC2983_DRIVER_ERROR_SPI_BUSY,
    LTC2983_DRIVER_ERROR_SPI_TIMEOUT,
    LTC2983_DRIVER_ERROR_SPI_ERROR,
    LTC2983_DRIVER_ERROR_DEVICE_BUSY,
    LTC2983_DRIVER_ERROR_WAIT_HW,
	LTC2983_DRIVER_ERROR_INVALID_CHANNEL,
	LTC2983_DRIVER_ERROR_NOT_INITIALIZED,
	LTC2983_DRIVER_ERROR_INVALID_CONFIGS,
} LTC2983DriverError_t;

/**  */
typedef enum {
	LTC2983_REGISTRY_STATUS_OKAY,
	LTC2983_REGISTRY_STATUS_NO_POINTER,
	LTC2983_REGISTRY_STATUS_ALREADY_INITIALIZED,
} LTC2983RegistryStatus_t;

/**  */
typedef enum {
	LTC2983_ENUM_CONV_STATUS_VALID,
	LTC2983_ENUM_CONV_STATUS_INVALID,
	LTC2983_ENUM_CONV_STATUS_ADC_RANGE_ERROR,
	LTC2983_ENUM_CONV_STATUS_SENSOR_BELOW,
	LTC2983_ENUM_CONV_STATUS_SENSOR_ABOVE,
	LTC2983_ENUM_CONV_STATUS_CJ_SOFT_FAILURE,
	LTC2983_ENUM_CONV_STATUS_CJ_HARD_FAILURE,
	LTC2983_ENUM_CONV_STATUS_ADC_HARD_FAILURE,
	LTC2983_ENUM_CONV_STATUS_SENSOR_HARD_FAILURE
} LTC2983EnumConvStatus_t;

/** LTC2983 channel number */
typedef uint8_t LTC2983Channel_t;
#define LTC2983_MULTIPLE_CHANNELS 0
#define LTC2983_CHANNEL_MIN 1
#define LTC2983_CHANNEL_MAX 20

/** LTC2983 Channel Assignment Data */
typedef uint32_t LTC2983ChannelAssignmentData_t;

/** LTC2983 Global Configuration Register */
typedef uint8_t LTC2983GlobalConfigReg_t;

/** LTC2983 Mux Configuration Delay (value is multiplied by 100us) */
typedef uint8_t LTC2983MuxConfigDelay_t;

/** LTC2983 Measure Multiple Channels Bit Mask */
typedef uint32_t LTC2983MeasMultiChannelsMask_t;

/** LTC2983 Temperature */
typedef float LTC2983Temperature_t;

/** LTC2983 Conversion Status */
typedef uint8_t LTC2983ConvStatus_t;

/** LTC2983 interface configuration */
typedef struct {
	  uint16_t Pin;        /*!< Specifies the GPIO pins to be configured.
	                           This parameter can be a value of @ref GPIO_pins */
	  GPIO_TypeDef *Port;	/*!< Specifies the GPIO port. This parameter can be a value of @ref GPIO_ports */
} GpioChannel_t;

/** LTC2983 GPIO port and spi handle container */
typedef struct {
    SPI_HandleTypeDef *hspi;
    GpioChannel_t GpioChipSelect;
    GpioChannel_t GpioReset;
    GpioChannel_t GpioInterrupt; // external interrupt
} LTC2983IfaceConfig_t;

/** LTC2983 internal state */
typedef struct{
	volatile bool Initialized;
	volatile bool StartupDone;
	volatile bool ChannelsConfigured;

	volatile LTC2983DriverStatus_t Status;
	volatile LTC2983DriverError_t Error;
	volatile LTC2983TaskState_t TaskState;

	// 1B instructions + 2B address + 4B data
	uint8_t TxBuffer[7] __attribute__((aligned(32)));
	uint8_t RxBuffer[7] __attribute__((aligned(32)));

	// Write and read channels assignment helper variables
	volatile uint8_t WriteChannelsAssignmentDataIndex;
	volatile uint8_t ReadChannelsAssignmentDataIndex;

	// Temp read temperature result of channel variable
	volatile uint8_t ReadAllIndex;
	volatile uint8_t LastChannelRead;

	volatile bool Reset;
} LTC2983RuntimeState_t;

/** LTC2983 channel configuration */
typedef struct {
    LTC2983Channel_t Channel;
    LTC2983ChannelAssignmentData_t Data;
} LTC2983ChannelConfig_t;

/** LTC2983 channel configurations */
typedef struct {
    LTC2983ChannelConfig_t * Configs;
    uint8_t Count;
} LTC2983ChannelConfigs_t;

/** LTC2983 conversion result */
typedef struct {
    LTC2983Channel_t Channel; // input param
    //volatile LTC2983EnumConvStatus_t Status; // output param
    volatile uint8_t Status;
    volatile LTC2983Temperature_t Temperature; // output param
    volatile uint32_t Raw;
} LTC2983ConvResult_t;

/** LTC2983 conversion results */
typedef struct {
    LTC2983ConvResult_t * const Results;
    uint8_t Count;
} LTC2983ConvResults_t;

struct LTC2983Handle_s;
/** LTC2983 task done callback type */
typedef void(*LTC2983TaskDoneCallback_t)(struct LTC2983Handle_s * const handle);

/** A general handle for each LTC2983 */
typedef struct LTC2983Handle_s{
	LTC2983IfaceConfig_t * IfaceConfig;

	LTC2983ChannelConfigs_t * ChannelConfigs;

	LTC2983ConvResults_t * Results;

	LTC2983RuntimeState_t * State;

	LTC2983MeasMultiChannelsMask_t BitMask;

	LTC2983GlobalConfigReg_t GlobalConfigurationRegister;

	LTC2983MuxConfigDelay_t MuxConfigDelay;

	LTC2983TaskDoneCallback_t TaskDoneCallback;
} LTC2983Handle_t;

/** A registry of all the LTC2983 handles available */
typedef struct{
	LTC2983Handle_t ** const Handles;
	uint8_t Count;
} LTC2983HandleRegistry_t;


//**********************************************************************************************************
// -- SENSOR TYPES --
//**********************************************************************************************************
#define LTC2983_SENSOR_TYPE_LSB 27
// RTD
#define LTC2983_SENSOR_TYPE__RTD_PT_10 ((LTC2983ChannelAssignmentData_t) 0xA << LTC2983_SENSOR_TYPE_LSB)
#define LTC2983_SENSOR_TYPE__RTD_PT_50 ((LTC2983ChannelAssignmentData_t) 0xB << LTC2983_SENSOR_TYPE_LSB)
#define LTC2983_SENSOR_TYPE__RTD_PT_100 ((LTC2983ChannelAssignmentData_t) 0xC << LTC2983_SENSOR_TYPE_LSB)
#define LTC2983_SENSOR_TYPE__RTD_PT_200 ((LTC2983ChannelAssignmentData_t) 0xD << LTC2983_SENSOR_TYPE_LSB)
#define LTC2983_SENSOR_TYPE__RTD_PT_500 ((LTC2983ChannelAssignmentData_t) 0xE << LTC2983_SENSOR_TYPE_LSB)
#define LTC2983_SENSOR_TYPE__RTD_PT_1000 ((LTC2983ChannelAssignmentData_t) 0xF << LTC2983_SENSOR_TYPE_LSB)
#define LTC2983_SENSOR_TYPE__RTD_PT_1000_375 ((LTC2983ChannelAssignmentData_t) 0x10 << LTC2983_SENSOR_TYPE_LSB)
#define LTC2983_SENSOR_TYPE__RTD_NI_120 ((LTC2983ChannelAssignmentData_t) 0x11 << LTC2983_SENSOR_TYPE_LSB)
#define LTC2983_SENSOR_TYPE__RTD_CUSTOM ((LTC2983ChannelAssignmentData_t) 0x12 << LTC2983_SENSOR_TYPE_LSB)
// Sense Resistor
#define LTC2983_SENSOR_TYPE__SENSE_RESISTOR ((LTC2983ChannelAssignmentData_t) 0x1D << LTC2983_SENSOR_TYPE_LSB)
// -
#define LTC2983_SENSOR_TYPE__NONE ((LTC2983ChannelAssignmentData_t) 0x0 << LTC2983_SENSOR_TYPE_LSB)
// Direct ADC
#define LTC2983_SENSOR_TYPE__DIRECT_ADC ((LTC2983ChannelAssignmentData_t) 0x1E << LTC2983_SENSOR_TYPE_LSB)
// Thermistor
#define LTC2983_SENSOR_TYPE__THERMISTOR_44004_2P252K_25C ((LTC2983ChannelAssignmentData_t) 0x13 << LTC2983_SENSOR_TYPE_LSB)
#define LTC2983_SENSOR_TYPE__THERMISTOR_44005_3K_25C ((LTC2983ChannelAssignmentData_t) 0x14 << LTC2983_SENSOR_TYPE_LSB)
#define LTC2983_SENSOR_TYPE__THERMISTOR_44007_5K_25C ((LTC2983ChannelAssignmentData_t) 0x15 << LTC2983_SENSOR_TYPE_LSB)
#define LTC2983_SENSOR_TYPE__THERMISTOR_44006_10K_25C ((LTC2983ChannelAssignmentData_t) 0x16 << LTC2983_SENSOR_TYPE_LSB)
#define LTC2983_SENSOR_TYPE__THERMISTOR_44008_30K_25C ((LTC2983ChannelAssignmentData_t) 0x17 << LTC2983_SENSOR_TYPE_LSB)
#define LTC2983_SENSOR_TYPE__THERMISTOR_YSI_400_2P252K_25C ((LTC2983ChannelAssignmentData_t) 0x18 << LTC2983_SENSOR_TYPE_LSB)
#define LTC2983_SENSOR_TYPE__THERMISTOR_1003K_1K_25C ((LTC2983ChannelAssignmentData_t) 0x19 << LTC2983_SENSOR_TYPE_LSB)
#define LTC2983_SENSOR_TYPE__THERMISTOR_CUSTOM_STEINHART_HART ((LTC2983ChannelAssignmentData_t) 0x1A << LTC2983_SENSOR_TYPE_LSB)
#define LTC2983_SENSOR_TYPE__THERMISTOR_CUSTOM_TABLE ((LTC2983ChannelAssignmentData_t) 0x1B << LTC2983_SENSOR_TYPE_LSB)
// Thermocouple
#define LTC2983_SENSOR_TYPE__TYPE_J_THERMOCOUPLE ((LTC2983ChannelAssignmentData_t) 0x1 << LTC2983_SENSOR_TYPE_LSB)
#define LTC2983_SENSOR_TYPE__TYPE_K_THERMOCOUPLE ((LTC2983ChannelAssignmentData_t) 0x2 << LTC2983_SENSOR_TYPE_LSB)
#define LTC2983_SENSOR_TYPE__TYPE_E_THERMOCOUPLE ((LTC2983ChannelAssignmentData_t) 0x3 << LTC2983_SENSOR_TYPE_LSB)
#define LTC2983_SENSOR_TYPE__TYPE_N_THERMOCOUPLE ((LTC2983ChannelAssignmentData_t) 0x4 << LTC2983_SENSOR_TYPE_LSB)
#define LTC2983_SENSOR_TYPE__TYPE_R_THERMOCOUPLE ((LTC2983ChannelAssignmentData_t) 0x5 << LTC2983_SENSOR_TYPE_LSB)
#define LTC2983_SENSOR_TYPE__TYPE_S_THERMOCOUPLE ((LTC2983ChannelAssignmentData_t) 0x6 << LTC2983_SENSOR_TYPE_LSB)
#define LTC2983_SENSOR_TYPE__TYPE_T_THERMOCOUPLE ((LTC2983ChannelAssignmentData_t) 0x7 << LTC2983_SENSOR_TYPE_LSB)
#define LTC2983_SENSOR_TYPE__TYPE_B_THERMOCOUPLE ((LTC2983ChannelAssignmentData_t) 0x8 << LTC2983_SENSOR_TYPE_LSB)
#define LTC2983_SENSOR_TYPE__CUSTOM_THERMOCOUPLE ((LTC2983ChannelAssignmentData_t) 0x9 << LTC2983_SENSOR_TYPE_LSB)
// Off-Chip Diode
#define LTC2983_SENSOR_TYPE__OFF_CHIP_DIODE ((LTC2983ChannelAssignmentData_t) 0x1C << LTC2983_SENSOR_TYPE_LSB)

//**********************************************************************************************************
// -- RTD --
//**********************************************************************************************************
// rtd - rsense channel
#define LTC2983_RTD_RSENSE_CHANNEL_LSB 22
#define LTC2983_RTD_RSENSE_CHANNEL__NONE ((LTC2983ChannelAssignmentData_t) 0x0 << LTC2983_RTD_RSENSE_CHANNEL_LSB)
#define LTC2983_RTD_RSENSE_CHANNEL__1 ((LTC2983ChannelAssignmentData_t) 0x1 << LTC2983_RTD_RSENSE_CHANNEL_LSB)
#define LTC2983_RTD_RSENSE_CHANNEL__2 ((LTC2983ChannelAssignmentData_t) 0x2 << LTC2983_RTD_RSENSE_CHANNEL_LSB)
#define LTC2983_RTD_RSENSE_CHANNEL__3 ((LTC2983ChannelAssignmentData_t) 0x3 << LTC2983_RTD_RSENSE_CHANNEL_LSB)
#define LTC2983_RTD_RSENSE_CHANNEL__4 ((LTC2983ChannelAssignmentData_t) 0x4 << LTC2983_RTD_RSENSE_CHANNEL_LSB)
#define LTC2983_RTD_RSENSE_CHANNEL__5 ((LTC2983ChannelAssignmentData_t) 0x5 << LTC2983_RTD_RSENSE_CHANNEL_LSB)
#define LTC2983_RTD_RSENSE_CHANNEL__6 ((LTC2983ChannelAssignmentData_t) 0x6 << LTC2983_RTD_RSENSE_CHANNEL_LSB)
#define LTC2983_RTD_RSENSE_CHANNEL__7 ((LTC2983ChannelAssignmentData_t) 0x7 << LTC2983_RTD_RSENSE_CHANNEL_LSB)
#define LTC2983_RTD_RSENSE_CHANNEL__8 ((LTC2983ChannelAssignmentData_t) 0x8 << LTC2983_RTD_RSENSE_CHANNEL_LSB)
#define LTC2983_RTD_RSENSE_CHANNEL__9 ((LTC2983ChannelAssignmentData_t) 0x9 << LTC2983_RTD_RSENSE_CHANNEL_LSB)
#define LTC2983_RTD_RSENSE_CHANNEL__10 ((LTC2983ChannelAssignmentData_t) 0xA << LTC2983_RTD_RSENSE_CHANNEL_LSB)
#define LTC2983_RTD_RSENSE_CHANNEL__11 ((LTC2983ChannelAssignmentData_t) 0xB << LTC2983_RTD_RSENSE_CHANNEL_LSB)
#define LTC2983_RTD_RSENSE_CHANNEL__12 ((LTC2983ChannelAssignmentData_t) 0xC << LTC2983_RTD_RSENSE_CHANNEL_LSB)
#define LTC2983_RTD_RSENSE_CHANNEL__13 ((LTC2983ChannelAssignmentData_t) 0xD << LTC2983_RTD_RSENSE_CHANNEL_LSB)
#define LTC2983_RTD_RSENSE_CHANNEL__14 ((LTC2983ChannelAssignmentData_t) 0xE << LTC2983_RTD_RSENSE_CHANNEL_LSB)
#define LTC2983_RTD_RSENSE_CHANNEL__15 ((LTC2983ChannelAssignmentData_t) 0xF << LTC2983_RTD_RSENSE_CHANNEL_LSB)
#define LTC2983_RTD_RSENSE_CHANNEL__16 ((LTC2983ChannelAssignmentData_t) 0x10 << LTC2983_RTD_RSENSE_CHANNEL_LSB)
#define LTC2983_RTD_RSENSE_CHANNEL__17 ((LTC2983ChannelAssignmentData_t) 0x11 << LTC2983_RTD_RSENSE_CHANNEL_LSB)
#define LTC2983_RTD_RSENSE_CHANNEL__18 ((LTC2983ChannelAssignmentData_t) 0x12 << LTC2983_RTD_RSENSE_CHANNEL_LSB)
#define LTC2983_RTD_RSENSE_CHANNEL__19 ((LTC2983ChannelAssignmentData_t) 0x13 << LTC2983_RTD_RSENSE_CHANNEL_LSB)
#define LTC2983_RTD_RSENSE_CHANNEL__20 ((LTC2983ChannelAssignmentData_t) 0x14 << LTC2983_RTD_RSENSE_CHANNEL_LSB)
// rtd - num wires
#define LTC2983_RTD_NUM_WIRES_LSB 20
#define LTC2983_RTD_NUM_WIRES__2_WIRE ((LTC2983ChannelAssignmentData_t) 0x0 << LTC2983_RTD_NUM_WIRES_LSB)
#define LTC2983_RTD_NUM_WIRES__3_WIRE ((LTC2983ChannelAssignmentData_t) 0x1 << LTC2983_RTD_NUM_WIRES_LSB)
#define LTC2983_RTD_NUM_WIRES__4_WIRE ((LTC2983ChannelAssignmentData_t) 0x2 << LTC2983_RTD_NUM_WIRES_LSB)
#define LTC2983_RTD_NUM_WIRES__4_WIRE_KELVIN_RSENSE ((LTC2983ChannelAssignmentData_t) 0x3 << LTC2983_RTD_NUM_WIRES_LSB)
// rtd - excitation mode
#define LTC2983_RTD_EXCITATION_MODE_LSB 18
#define LTC2983_RTD_EXCITATION_MODE__NO_ROTATION_NO_SHARING ((LTC2983ChannelAssignmentData_t) 0x0 << LTC2983_RTD_EXCITATION_MODE_LSB)
#define LTC2983_RTD_EXCITATION_MODE__NO_ROTATION_SHARING ((LTC2983ChannelAssignmentData_t) 0x1 << LTC2983_RTD_EXCITATION_MODE_LSB)
#define LTC2983_RTD_EXCITATION_MODE__ROTATION_SHARING ((LTC2983ChannelAssignmentData_t) 0x2 << LTC2983_RTD_EXCITATION_MODE_LSB)
// rtd - excitation current
#define LTC2983_RTD_EXCITATION_CURRENT_LSB 14
#define LTC2983_RTD_EXCITATION_CURRENT__INVALID ((LTC2983ChannelAssignmentData_t) 0x0 << LTC2983_RTD_EXCITATION_CURRENT_LSB)
#define LTC2983_RTD_EXCITATION_CURRENT__5UA ((LTC2983ChannelAssignmentData_t) 0x1 << LTC2983_RTD_EXCITATION_CURRENT_LSB)
#define LTC2983_RTD_EXCITATION_CURRENT__10UA ((LTC2983ChannelAssignmentData_t) 0x2 << LTC2983_RTD_EXCITATION_CURRENT_LSB)
#define LTC2983_RTD_EXCITATION_CURRENT__25UA ((LTC2983ChannelAssignmentData_t) 0x3 << LTC2983_RTD_EXCITATION_CURRENT_LSB)
#define LTC2983_RTD_EXCITATION_CURRENT__50UA ((LTC2983ChannelAssignmentData_t) 0x4 << LTC2983_RTD_EXCITATION_CURRENT_LSB)
#define LTC2983_RTD_EXCITATION_CURRENT__100UA ((LTC2983ChannelAssignmentData_t) 0x5 << LTC2983_RTD_EXCITATION_CURRENT_LSB)
#define LTC2983_RTD_EXCITATION_CURRENT__250UA ((LTC2983ChannelAssignmentData_t) 0x6 << LTC2983_RTD_EXCITATION_CURRENT_LSB)
#define LTC2983_RTD_EXCITATION_CURRENT__500UA ((LTC2983ChannelAssignmentData_t) 0x7 << LTC2983_RTD_EXCITATION_CURRENT_LSB)
#define LTC2983_RTD_EXCITATION_CURRENT__1MA ((LTC2983ChannelAssignmentData_t) 0x8 << LTC2983_RTD_EXCITATION_CURRENT_LSB)
// rtd - standard
#define LTC2983_RTD_STANDARD_LSB 12
#define LTC2983_RTD_STANDARD__EUROPEAN ((LTC2983ChannelAssignmentData_t) 0x0 << LTC2983_RTD_STANDARD_LSB)
#define LTC2983_RTD_STANDARD__AMERICAN ((LTC2983ChannelAssignmentData_t) 0x1 << LTC2983_RTD_STANDARD_LSB)
#define LTC2983_RTD_STANDARD__JAPANESE ((LTC2983ChannelAssignmentData_t) 0x2 << LTC2983_RTD_STANDARD_LSB)
#define LTC2983_RTD_STANDARD__ITS_90 ((LTC2983ChannelAssignmentData_t) 0x3 << LTC2983_RTD_STANDARD_LSB)
// rtd - custom address
#define LTC2983_RTD_CUSTOM_ADDRESS_LSB 6
// rtd - custom length-1
#define LTC2983_RTD_CUSTOM_LENGTH_1_LSB 0
// rtd - custom values
#define LTC2983_RTD_CUSTOM_VALUES_LSB 31

//**********************************************************************************************************
// -- Sense Resistor --
//**********************************************************************************************************
// sense resistor - value
#define LTC2983_SENSE_RESISTOR_VALUE_LSB 0

//**********************************************************************************************************
// -- Direct ADC --
//**********************************************************************************************************
// Direct ADC - differential?
#define LTC2983_DIRECT_ADC_DIFFERENTIAL_LSB 26
#define LTC2983_DIRECT_ADC_DIFFERENTIAL ((LTC2983ChannelAssignmentData_t) 0x0 << LTC2983_DIRECT_ADC_DIFFERENTIAL_LSB)
#define LTC2983_DIRECT_ADC_SINGLE_ENDED ((LTC2983ChannelAssignmentData_t) 0x1 << LTC2983_DIRECT_ADC_DIFFERENTIAL_LSB)

//**********************************************************************************************************
// -- Thermistor --
//**********************************************************************************************************
// thermistor - rsense channel
#define LTC2983_THERMISTOR_RSENSE_CHANNEL_LSB 22
#define LTC2983_THERMISTOR_RSENSE_CHANNEL__NONE ((LTC2983ChannelAssignmentData_t) 0x0 << LTC2983_THERMISTOR_RSENSE_CHANNEL_LSB)
#define LTC2983_THERMISTOR_RSENSE_CHANNEL__1 ((LTC2983ChannelAssignmentData_t) 0x1 << LTC2983_THERMISTOR_RSENSE_CHANNEL_LSB)
#define LTC2983_THERMISTOR_RSENSE_CHANNEL__2 ((LTC2983ChannelAssignmentData_t) 0x2 << LTC2983_THERMISTOR_RSENSE_CHANNEL_LSB)
#define LTC2983_THERMISTOR_RSENSE_CHANNEL__3 ((LTC2983ChannelAssignmentData_t) 0x3 << LTC2983_THERMISTOR_RSENSE_CHANNEL_LSB)
#define LTC2983_THERMISTOR_RSENSE_CHANNEL__4 ((LTC2983ChannelAssignmentData_t) 0x4 << LTC2983_THERMISTOR_RSENSE_CHANNEL_LSB)
#define LTC2983_THERMISTOR_RSENSE_CHANNEL__5 ((LTC2983ChannelAssignmentData_t) 0x5 << LTC2983_THERMISTOR_RSENSE_CHANNEL_LSB)
#define LTC2983_THERMISTOR_RSENSE_CHANNEL__6 ((LTC2983ChannelAssignmentData_t) 0x6 << LTC2983_THERMISTOR_RSENSE_CHANNEL_LSB)
#define LTC2983_THERMISTOR_RSENSE_CHANNEL__7 ((LTC2983ChannelAssignmentData_t) 0x7 << LTC2983_THERMISTOR_RSENSE_CHANNEL_LSB)
#define LTC2983_THERMISTOR_RSENSE_CHANNEL__8 ((LTC2983ChannelAssignmentData_t) 0x8 << LTC2983_THERMISTOR_RSENSE_CHANNEL_LSB)
#define LTC2983_THERMISTOR_RSENSE_CHANNEL__9 ((LTC2983ChannelAssignmentData_t) 0x9 << LTC2983_THERMISTOR_RSENSE_CHANNEL_LSB)
#define LTC2983_THERMISTOR_RSENSE_CHANNEL__10 ((LTC2983ChannelAssignmentData_t) 0xA << LTC2983_THERMISTOR_RSENSE_CHANNEL_LSB)
#define LTC2983_THERMISTOR_RSENSE_CHANNEL__11 ((LTC2983ChannelAssignmentData_t) 0xB << LTC2983_THERMISTOR_RSENSE_CHANNEL_LSB)
#define LTC2983_THERMISTOR_RSENSE_CHANNEL__12 ((LTC2983ChannelAssignmentData_t) 0xC << LTC2983_THERMISTOR_RSENSE_CHANNEL_LSB)
#define LTC2983_THERMISTOR_RSENSE_CHANNEL__13 ((LTC2983ChannelAssignmentData_t) 0xD << LTC2983_THERMISTOR_RSENSE_CHANNEL_LSB)
#define LTC2983_THERMISTOR_RSENSE_CHANNEL__14 ((LTC2983ChannelAssignmentData_t) 0xE << LTC2983_THERMISTOR_RSENSE_CHANNEL_LSB)
#define LTC2983_THERMISTOR_RSENSE_CHANNEL__15 ((LTC2983ChannelAssignmentData_t) 0xF << LTC2983_THERMISTOR_RSENSE_CHANNEL_LSB)
#define LTC2983_THERMISTOR_RSENSE_CHANNEL__16 ((LTC2983ChannelAssignmentData_t) 0x10 << LTC2983_THERMISTOR_RSENSE_CHANNEL_LSB)
#define LTC2983_THERMISTOR_RSENSE_CHANNEL__17 ((LTC2983ChannelAssignmentData_t) 0x11 << LTC2983_THERMISTOR_RSENSE_CHANNEL_LSB)
#define LTC2983_THERMISTOR_RSENSE_CHANNEL__18 ((LTC2983ChannelAssignmentData_t) 0x12 << LTC2983_THERMISTOR_RSENSE_CHANNEL_LSB)
#define LTC2983_THERMISTOR_RSENSE_CHANNEL__19 ((LTC2983ChannelAssignmentData_t) 0x13 << LTC2983_THERMISTOR_RSENSE_CHANNEL_LSB)
#define LTC2983_THERMISTOR_RSENSE_CHANNEL__20 ((LTC2983ChannelAssignmentData_t) 0x14 << LTC2983_THERMISTOR_RSENSE_CHANNEL_LSB)
// thermistor - differential?
#define LTC2983_THERMISTOR_DIFFERENTIAL_LSB 21
#define LTC2983_THERMISTOR_DIFFERENTIAL ((LTC2983ChannelAssignmentData_t) 0x0 << LTC2983_THERMISTOR_DIFFERENTIAL_LSB)
#define LTC2983_THERMISTOR_SINGLE_ENDED ((LTC2983ChannelAssignmentData_t) 0x1 << LTC2983_THERMISTOR_DIFFERENTIAL_LSB)
// thermistor - excitation mode
#define LTC2983_THERMISTOR_EXCITATION_MODE_LSB 19
#define LTC2983_THERMISTOR_EXCITATION_MODE__NO_SHARING_NO_ROTATION ((LTC2983ChannelAssignmentData_t) 0x0 << LTC2983_THERMISTOR_EXCITATION_MODE_LSB)
#define LTC2983_THERMISTOR_EXCITATION_MODE__SHARING_ROTATION ((LTC2983ChannelAssignmentData_t) 0x1 << LTC2983_THERMISTOR_EXCITATION_MODE_LSB)
#define LTC2983_THERMISTOR_EXCITATION_MODE__SHARING_NO_ROTATION ((LTC2983ChannelAssignmentData_t) 0x2 << LTC2983_THERMISTOR_EXCITATION_MODE_LSB)
// thermistor - excitation current
#define LTC2983_THERMISTOR_EXCITATION_CURRENT_LSB 15
#define LTC2983_THERMISTOR_EXCITATION_CURRENT__INVALID ((LTC2983ChannelAssignmentData_t) 0x0 << LTC2983_THERMISTOR_EXCITATION_CURRENT_LSB)
#define LTC2983_THERMISTOR_EXCITATION_CURRENT__250NA ((LTC2983ChannelAssignmentData_t) 0x1 << LTC2983_THERMISTOR_EXCITATION_CURRENT_LSB)
#define LTC2983_THERMISTOR_EXCITATION_CURRENT__500NA ((LTC2983ChannelAssignmentData_t) 0x2 << LTC2983_THERMISTOR_EXCITATION_CURRENT_LSB)
#define LTC2983_THERMISTOR_EXCITATION_CURRENT__1UA ((LTC2983ChannelAssignmentData_t) 0x3 << LTC2983_THERMISTOR_EXCITATION_CURRENT_LSB)
#define LTC2983_THERMISTOR_EXCITATION_CURRENT__5UA ((LTC2983ChannelAssignmentData_t) 0x4 << LTC2983_THERMISTOR_EXCITATION_CURRENT_LSB)
#define LTC2983_THERMISTOR_EXCITATION_CURRENT__10UA ((LTC2983ChannelAssignmentData_t) 0x5 << LTC2983_THERMISTOR_EXCITATION_CURRENT_LSB)
#define LTC2983_THERMISTOR_EXCITATION_CURRENT__25UA ((LTC2983ChannelAssignmentData_t) 0x6 << LTC2983_THERMISTOR_EXCITATION_CURRENT_LSB)
#define LTC2983_THERMISTOR_EXCITATION_CURRENT__50UA ((LTC2983ChannelAssignmentData_t) 0x7 << LTC2983_THERMISTOR_EXCITATION_CURRENT_LSB)
#define LTC2983_THERMISTOR_EXCITATION_CURRENT__100UA ((LTC2983ChannelAssignmentData_t) 0x8 << LTC2983_THERMISTOR_EXCITATION_CURRENT_LSB)
#define LTC2983_THERMISTOR_EXCITATION_CURRENT__250UA ((LTC2983ChannelAssignmentData_t) 0x9 << LTC2983_THERMISTOR_EXCITATION_CURRENT_LSB)
#define LTC2983_THERMISTOR_EXCITATION_CURRENT__500UA ((LTC2983ChannelAssignmentData_t) 0xA << LTC2983_THERMISTOR_EXCITATION_CURRENT_LSB)
#define LTC2983_THERMISTOR_EXCITATION_CURRENT__1MA ((LTC2983ChannelAssignmentData_t) 0xB << LTC2983_THERMISTOR_EXCITATION_CURRENT_LSB)
#define LTC2983_THERMISTOR_EXCITATION_CURRENT__AUTORANGE ((LTC2983ChannelAssignmentData_t) 0xC << LTC2983_THERMISTOR_EXCITATION_CURRENT_LSB)
// thermistor - custom address
#define LTC2983_THERMISTOR_CUSTOM_ADDRESS_LSB 6
// thermistor - custom length-1
#define LTC2983_THERMISTOR_CUSTOM_LENGTH_1_LSB 0
// thermistor - custom values
#define LTC2983_THERMISTOR_CUSTOM_VALUES_LSB 31

//**********************************************************************************************************
// -- Thermocouple --
//**********************************************************************************************************
// tc - cold junction ch
#define LTC2983_TC_COLD_JUNCTION_CH_LSB 22
#define LTC2983_TC_COLD_JUNCTION_CH__NONE ((LTC2983ChannelAssignmentData_t) 0x0 << LTC2983_TC_COLD_JUNCTION_CH_LSB)
#define LTC2983_TC_COLD_JUNCTION_CH__1 ((LTC2983ChannelAssignmentData_t) 0x1 << LTC2983_TC_COLD_JUNCTION_CH_LSB)
#define LTC2983_TC_COLD_JUNCTION_CH__2 ((LTC2983ChannelAssignmentData_t) 0x2 << LTC2983_TC_COLD_JUNCTION_CH_LSB)
#define LTC2983_TC_COLD_JUNCTION_CH__3 ((LTC2983ChannelAssignmentData_t) 0x3 << LTC2983_TC_COLD_JUNCTION_CH_LSB)
#define LTC2983_TC_COLD_JUNCTION_CH__4 ((LTC2983ChannelAssignmentData_t) 0x4 << LTC2983_TC_COLD_JUNCTION_CH_LSB)
#define LTC2983_TC_COLD_JUNCTION_CH__5 ((LTC2983ChannelAssignmentData_t) 0x5 << LTC2983_TC_COLD_JUNCTION_CH_LSB)
#define LTC2983_TC_COLD_JUNCTION_CH__6 ((LTC2983ChannelAssignmentData_t) 0x6 << LTC2983_TC_COLD_JUNCTION_CH_LSB)
#define LTC2983_TC_COLD_JUNCTION_CH__7 ((LTC2983ChannelAssignmentData_t) 0x7 << LTC2983_TC_COLD_JUNCTION_CH_LSB)
#define LTC2983_TC_COLD_JUNCTION_CH__8 ((LTC2983ChannelAssignmentData_t) 0x8 << LTC2983_TC_COLD_JUNCTION_CH_LSB)
#define LTC2983_TC_COLD_JUNCTION_CH__9 ((LTC2983ChannelAssignmentData_t) 0x9 << LTC2983_TC_COLD_JUNCTION_CH_LSB)
#define LTC2983_TC_COLD_JUNCTION_CH__10 ((LTC2983ChannelAssignmentData_t) 0xA << LTC2983_TC_COLD_JUNCTION_CH_LSB)
#define LTC2983_TC_COLD_JUNCTION_CH__11 ((LTC2983ChannelAssignmentData_t) 0xB << LTC2983_TC_COLD_JUNCTION_CH_LSB)
#define LTC2983_TC_COLD_JUNCTION_CH__12 ((LTC2983ChannelAssignmentData_t) 0xC << LTC2983_TC_COLD_JUNCTION_CH_LSB)
#define LTC2983_TC_COLD_JUNCTION_CH__13 ((LTC2983ChannelAssignmentData_t) 0xD << LTC2983_TC_COLD_JUNCTION_CH_LSB)
#define LTC2983_TC_COLD_JUNCTION_CH__14 ((LTC2983ChannelAssignmentData_t) 0xE << LTC2983_TC_COLD_JUNCTION_CH_LSB)
#define LTC2983_TC_COLD_JUNCTION_CH__15 ((LTC2983ChannelAssignmentData_t) 0xF << LTC2983_TC_COLD_JUNCTION_CH_LSB)
#define LTC2983_TC_COLD_JUNCTION_CH__16 ((LTC2983ChannelAssignmentData_t) 0x10 << LTC2983_TC_COLD_JUNCTION_CH_LSB)
#define LTC2983_TC_COLD_JUNCTION_CH__17 ((LTC2983ChannelAssignmentData_t) 0x11 << LTC2983_TC_COLD_JUNCTION_CH_LSB)
#define LTC2983_TC_COLD_JUNCTION_CH__18 ((LTC2983ChannelAssignmentData_t) 0x12 << LTC2983_TC_COLD_JUNCTION_CH_LSB)
#define LTC2983_TC_COLD_JUNCTION_CH__19 ((LTC2983ChannelAssignmentData_t) 0x13 << LTC2983_TC_COLD_JUNCTION_CH_LSB)
#define LTC2983_TC_COLD_JUNCTION_CH__20 ((LTC2983ChannelAssignmentData_t) 0x14 << LTC2983_TC_COLD_JUNCTION_CH_LSB)
// tc - differential?
#define LTC2983_TC_DIFFERENTIAL_LSB 21
#define LTC2983_TC_DIFFERENTIAL ((LTC2983ChannelAssignmentData_t) 0x0 << LTC2983_TC_DIFFERENTIAL_LSB)
#define LTC2983_TC_SINGLE_ENDED ((LTC2983ChannelAssignmentData_t) 0x1 << LTC2983_TC_DIFFERENTIAL_LSB)
// tc - open ckt detect?
#define LTC2983_TC_OPEN_CKT_DETECT_LSB 20
#define LTC2983_TC_OPEN_CKT_DETECT__NO ((LTC2983ChannelAssignmentData_t) 0x0 << LTC2983_TC_OPEN_CKT_DETECT_LSB)
#define LTC2983_TC_OPEN_CKT_DETECT__YES ((LTC2983ChannelAssignmentData_t) 0x1 << LTC2983_TC_OPEN_CKT_DETECT_LSB)
// tc - open ckt detect current
#define LTC2983_TC_OPEN_CKT_DETECT_CURRENT_LSB 18
#define LTC2983_TC_OPEN_CKT_DETECT_CURRENT__10UA ((LTC2983ChannelAssignmentData_t) 0x0 << LTC2983_TC_OPEN_CKT_DETECT_CURRENT_LSB)
#define LTC2983_TC_OPEN_CKT_DETECT_CURRENT__100UA ((LTC2983ChannelAssignmentData_t) 0x1 << LTC2983_TC_OPEN_CKT_DETECT_CURRENT_LSB)
#define LTC2983_TC_OPEN_CKT_DETECT_CURRENT__500UA ((LTC2983ChannelAssignmentData_t) 0x2 << LTC2983_TC_OPEN_CKT_DETECT_CURRENT_LSB)
#define LTC2983_TC_OPEN_CKT_DETECT_CURRENT__1MA ((LTC2983ChannelAssignmentData_t) 0x3 << LTC2983_TC_OPEN_CKT_DETECT_CURRENT_LSB)
// tc - custom address
#define LTC2983_TC_CUSTOM_ADDRESS_LSB 6
// tc - custom length-1
#define LTC2983_TC_CUSTOM_LENGTH_1_LSB 0
// tc - custom values
#define LTC2983_TC_CUSTOM_VALUES_LSB 31

//**********************************************************************************************************
// -- Off-Chip Diode --
//**********************************************************************************************************
// diode - differential?
#define LTC2983_DIODE_DIFFERENTIAL_LSB 26
#define LTC2983_DIODE_DIFFERENTIAL ((LTC2983ChannelAssignmentData_t) 0x0 << LTC2983_DIODE_DIFFERENTIAL_LSB)
#define LTC2983_DIODE_SINGLE_ENDED ((LTC2983ChannelAssignmentData_t) 0x1 << LTC2983_DIODE_DIFFERENTIAL_LSB)
// diode - num readings
#define LTC2983_DIODE_NUM_READINGS_LSB 25
#define LTC2983_DIODE_NUM_READINGS__2 ((LTC2983ChannelAssignmentData_t) 0x0 << LTC2983_DIODE_NUM_READINGS_LSB)
#define LTC2983_DIODE_NUM_READINGS__3 ((LTC2983ChannelAssignmentData_t) 0x1 << LTC2983_DIODE_NUM_READINGS_LSB)
// diode - averaging on?
#define LTC2983_DIODE_AVERAGING_ON_LSB 24
#define LTC2983_DIODE_AVERAGING_OFF ((LTC2983ChannelAssignmentData_t) 0x0 << LTC2983_DIODE_AVERAGING_ON_LSB)
#define LTC2983_DIODE_AVERAGING_ON ((LTC2983ChannelAssignmentData_t) 0x1 << LTC2983_DIODE_AVERAGING_ON_LSB)
// diode - current
#define LTC2983_DIODE_CURRENT_LSB 22
#define LTC2983_DIODE_CURRENT__10UA_40UA_80UA ((LTC2983ChannelAssignmentData_t) 0x0 << LTC2983_DIODE_CURRENT_LSB)
#define LTC2983_DIODE_CURRENT__20UA_80UA_160UA ((LTC2983ChannelAssignmentData_t) 0x1 << LTC2983_DIODE_CURRENT_LSB)
#define LTC2983_DIODE_CURRENT__40UA_160UA_320UA ((LTC2983ChannelAssignmentData_t) 0x2 << LTC2983_DIODE_CURRENT_LSB)
#define LTC2983_DIODE_CURRENT__80UA_320UA_640UA ((LTC2983ChannelAssignmentData_t) 0x3 << LTC2983_DIODE_CURRENT_LSB)
// diode - ideality factor(eta)
#define LTC2983_DIODE_IDEALITY_FACTOR_LSB 0


//**********************************************************************************************************
// -- GLOBAL CONFIGURATION CONSTANTS --
//**********************************************************************************************************
#define LTC2983_REJECTION__50_60_HZ ((LTC2983GlobalConfigReg_t) 0x0)
#define LTC2983_REJECTION__60_HZ    ((LTC2983GlobalConfigReg_t) 0x1)
#define LTC2983_REJECTION__50_HZ    ((LTC2983GlobalConfigReg_t) 0x2)
#define LTC2983_TEMP_UNIT__C        ((LTC2983GlobalConfigReg_t) 0x0)
#define LTC2983_TEMP_UNIT__F        ((LTC2983GlobalConfigReg_t) 0x4)


//**********************************************************************************************************
// -- CONVERSION RESULT STATUS BYTE CONSTANTS --
//**********************************************************************************************************
#define LTC2983_CONV_STATUS_SENSOR_HARD_FAILURE ((LTC2983ConvStatus_t) 0x80)
#define LTC2983_CONV_STATUS_ADC_HARD_FAILURE    ((LTC2983ConvStatus_t) 0x40)
#define LTC2983_CONV_STATUS_CJ_HARD_FAILURE     ((LTC2983ConvStatus_t) 0x20)
#define LTC2983_CONV_STATUS_CJ_SOFT_FAILURE     ((LTC2983ConvStatus_t) 0x10)
#define LTC2983_CONV_STATUS_SENSOR_ABOVE        ((LTC2983ConvStatus_t) 0x08)
#define LTC2983_CONV_STATUS_SENSOR_BELOW        ((LTC2983ConvStatus_t) 0x04)
#define LTC2983_CONV_STATUS_ADC_RANGE_ERROR     ((LTC2983ConvStatus_t) 0x02)
#define LTC2983_CONV_STATUS_VALID               ((LTC2983ConvStatus_t) 0x01)
#define LTC2983_CONV_STATUS_INVALID             ((LTC2983ConvStatus_t) 0x00)


#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */

/** Assignment of LTC2983HandleRegistry for the driver. */
LTC2983RegistryStatus_t LTC2983_RegisterLTC2983HandleRegistry(LTC2983HandleRegistry_t * const handleRegistry);
/** Unassignment of LTC2983HandleRegistry for the driver. */
LTC2983RegistryStatus_t LTC2983_UnRegisterLTC2983HandleRegistry(void);
/** Allocation, initialization and configuration of LTC2983 interface */
LTC2983DriverStatus_t LTC2983_Init(LTC2983Handle_t * const handle);
/** Get driver status */
LTC2983DriverStatus_t LTC2983_GetDriverStatus(const LTC2983Handle_t * const handle);
/** Get driver error */
LTC2983DriverError_t LTC2983_GetDriverError(const LTC2983Handle_t * const handle);
/** Set task done callback */
void LTC2983_RegisterTaskDoneCallback(LTC2983Handle_t * const handle, LTC2983TaskDoneCallback_t callback);
/** Clear task done callback */
void LTC2983_UnRegisterTaskDoneCallback(LTC2983Handle_t * const handle);
/** Enter LTC2983 start-up or checks if LTC2983 is awake. Driver must not be busy. */
LTC2983DriverStatus_t LTC2983_StartUp(LTC2983Handle_t * const handle);
/** Place LTC2983 into sleep mode. Driver must not be busy. */
LTC2983DriverStatus_t LTC2983_Sleep(LTC2983Handle_t * const handle);
/** Write LTC2983 Channel Assignment Data. Driver must not be busy. */
LTC2983DriverStatus_t LTC2983_WriteChannelsAssignmentData(LTC2983Handle_t * const handle);
/** Read LTC2983 Channel Assignment Data. Driver must not be busy. Careful, overwrites channels in LTC struct */
LTC2983DriverStatus_t LTC2983_ReadChannelsAssignmentData(LTC2983Handle_t * const handle);
/** Write LTC2983 Global Configuration Register. Driver must not be busy. */
LTC2983DriverStatus_t LTC2983_WriteGlobalConfigReg(LTC2983Handle_t * const handle);
/** Read LTC2983 Global Configuration Register. Driver must not be busy. */
LTC2983DriverStatus_t LTC2983_ReadGlobalConfigReg(LTC2983Handle_t * const handle);
/** Write LTC2983 Mux Configuration Delay. Driver must not be busy. */
LTC2983DriverStatus_t LTC2983_WriteMuxConfigDelay(LTC2983Handle_t * const handle);
/** Read LTC2983 Mux Configuration Delay. Driver must not be busy. */
LTC2983DriverStatus_t LTC2983_ReadMuxConfigDelay(LTC2983Handle_t * const handle);
/** Write LTC2983 Measure Multiple Channels Bit Mask. Driver must not be busy. */
LTC2983DriverStatus_t LTC2983_WriteMeasMultiChannelsMask(LTC2983Handle_t * const handle);
/** Read LTC2983 Measure Multiple Channels Bit Mask. Overwrites old bit mask in handle. Driver must not be busy. */
LTC2983DriverStatus_t LTC2983_ReadMeasMultiChannelsMask(LTC2983Handle_t * const handle);
/** Convert LTC2983 Channel. Driver must not be busy. The temp result will be in the LTC->result 0 for bit mask. */
LTC2983DriverStatus_t LTC2983_Convert(LTC2983Handle_t * const handle, const LTC2983Channel_t channel);
/** Read LTC2983 Temperature Result. Driver must not be busy. 0 for multimask channels. The temp results will be in the handle->results. */
LTC2983DriverStatus_t LTC2983_ReadTemperatureResults(LTC2983Handle_t * const handle, const LTC2983Channel_t channel);

LTC2983DriverStatus_t LTC2983_ReadRawVoltage(LTC2983Handle_t * const handle, const LTC2983Channel_t channel);

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif /* INC_LTC2983_H_ */
