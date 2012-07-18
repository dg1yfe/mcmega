/*
 * firmware.h
 *
 *  Created on: 27.05.2012
 *      Author: eligs
 */

#ifndef FIRMWARE_H_
#define FIRMWARE_H_
#include "FreeRTOS.h"
#include "semphr.h"
extern xSemaphoreHandle SerialBusMutex;
extern xQueueHandle xRxQ, xRxKeyQ;
extern xTaskHandle xUiTaskHandle, xControlTaskHandle;

#endif /* FIRMWARE_H_ */
