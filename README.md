# LTC2983 High-Accuracy Temperature Sensor Driver

![C](https://img.shields.io/badge/Language-C-blue.svg)
![Platform](https://img.shields.io/badge/Platform-STM32-lightgrey.svg)
![Architecture](https://img.shields.io/badge/Architecture-Non--Blocking-success.svg)

A robust, non-blocking C driver for the **LTC2983** Multi-Sensor High Accuracy Digital Temperature System, designed primarily for STM32 microcontrollers. 

This driver was built with real-time embedded systems in mind. Instead of blocking the CPU during SPI communication or hardware sensor conversion, it utilizes a **state machine**, **DMA-driven SPI transfers**, and **EXTI hardware interrupts** to ensure highly efficient, asynchronous execution.

## 🚀 Key Features

* **Non-Blocking Architecture:** Uses `HAL_SPI_TransmitReceive_DMA` to prevent CPU stalling during data transfers.
* **Interrupt-Driven:** Hardware conversion completion is caught via external interrupts, progressing without polling delays.
* **Multi-Instance Support:** Designed around a handle registry (`LTC2983HandleRegistry_t`), allowing multiple LTC2983 chips to run concurrently on the same MCU safely.

## 📂 Code Structure

* `LTC2983.h` / `LTC2983.c` - Core driver logic, state machine definition, and hardware abstraction layer.
* `LTC2983_config.h` / `LTC2983_config.c` - Implementation-specific configuration, defining channel assignments, SPI handles (e.g., `hspi2`), and GPIO pins for Chip Select and Reset.

## 🛠️ Usage Example

The driver is designed to be easily integrated into RTOS tasks or bare-metal main loops using an event-driven approach:

```c
#include "LTC2983.h"

// 1. Initialize the driver with the hardware handle
LTC2983_Init(&ltc1Handle); //[cite: 2, 3]

// 2. Start up the device
LTC2983_StartUp(&ltc1Handle); //[cite: 2]

// ... Wait for the driver status to become LTC2983_DRIVER_STATUS_NONE ...

// 3. Write channel assignments to the sensor
LTC2983_WriteChannelsAssignmentData(&ltc1Handle); //[cite: 2]

// 4. Request a temperature conversion (e.g., Channel 4 configured as PT1000)
LTC2983_Convert(&ltc1Handle, 4); //[cite: 2]

// 5. Once EXTI triggers the callback, read the results asynchronously:
LTC2983_ReadTemperatureResults(&ltc1Handle, 4); //[cite: 2]

// 6. Access the safely stored result:
// float temp = ltc1Handle.Results->Results[3].Temperature; //[cite: 2]
