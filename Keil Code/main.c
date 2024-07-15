/*
 * COMPENG 2DX3 Final Project Code
 * Written by Umar Javaid
 * April 1st, 2024
 */

// clang-format off
#include <stdint.h>
#include "PLL.h"
#include "SysTick.h"
#include "uart.h"
#include "onboardLEDs.h"
#include "tm4c1294ncpdt.h"
#include "VL53L1X_api.h"
#include "init.h"
// clang-format on

// Project Constants and Variables
#define ROTATE_CLOCKWISE 1
#define ROTATE_COUNTERCLOCKWISE 0
#define NUMBER_OF_SAMPLES 64
#define NUMBER_OF_MEASUREMENTS 3

int measuredDistances[NUMBER_OF_SAMPLES];  // Array to store distance measurements
char stepperMotorSteps[] = {0b00000011, 0b00000110, 0b00001100, 0b00001001}; // Motor phase steps
uint16_t tofSensorAddress = 0x29; // I2C address of the Time-of-Flight sensor
int operationStatus = 0; // Variable to store the status of various operations
int motorActivationFlag = 0; // Flag to control motor operation
int stepDelay = 26666; // Delay between motor steps to control speed

/// Spins the motor for a specified number of steps in a given direction
void spinMotor(int stepCount, int direction)
{
    for (int stepIndex = 0; stepIndex < stepCount; stepIndex++)
    {
        if (direction == ROTATE_CLOCKWISE)
        {
            for (int motorIndex = 0; motorIndex < 4; motorIndex++)
            {
                GPIO_PORTH_DATA_R = stepperMotorSteps[motorIndex]; // Set motor phase
                SysTick_Wait(stepDelay); // Wait for a period
            }
        }
        else if (direction == ROTATE_COUNTERCLOCKWISE)
        {
            for (int motorIndex = 3; motorIndex >= 0; motorIndex--)
            {
                GPIO_PORTH_DATA_R = stepperMotorSteps[motorIndex]; // Set motor phase in reverse
                SysTick_Wait(stepDelay * 3); // Longer wait for reverse direction
            }
        }
    }
}

void GPIOJ_IRQHandler(void)
{
    motorActivationFlag = ~motorActivationFlag; // Toggle motor activation flag
    GPIO_PORTJ_ICR_R = 0x02; // Clear the interrupt flag
}

void CheckBusSpeed(void)
{
    while (1)
    {
        GPIO_PORTM_DATA_R ^= 0x01; // Toggle port data for speed check
        SysTick_Wait(160000); // Wait period
    }
}
	
int main(void)
{
    uint16_t sensorDistance;
    uint8_t sensorRangeStatus;
    uint8_t sensorDataReady;
    uint8_t sensorInitializationState = 0; // Sensor state for boot completion
    char uartSignal;

    // Initialize system peripherals
    PLL_Init(); // Initialize Phase Locked Loop
    PortH_Init(); // Initialize port H
    SysTick_Init(); // Initialize SysTick Timer
    onboardLEDs_Init(); // Initialize onboard LEDs
    I2C_Init(); // Initialize I2C communication
    UART_Init(); // Initialize UART communication
    PortJ_Init(); // Initialize port J
    PortJ_Interrupt_Init(); // Initialize port J interrupts
	
		//Uncomment to Check BusSpeed
			//PortM_Init();
			//CheckBusSpeed();

    // Initialize the Time-of-Flight sensor
    while (sensorInitializationState == 0)
    {
        operationStatus = VL53L1X_BootState(tofSensorAddress, &sensorInitializationState);
        SysTick_Wait10ms(100); // Wait for sensor boot-up
    }

    FlashAllLEDs(); // Signal initialization with LEDs

    operationStatus = VL53L1X_ClearInterrupt(tofSensorAddress); // Clear any interrupts
    operationStatus = VL53L1X_SensorInit(tofSensorAddress); // Initialize the sensor
    Status_Check("SensorInit", operationStatus); // Check sensor initialization status

    operationStatus = VL53L1X_StartRanging(tofSensorAddress); // Start ranging
    Status_Check("StartRanging", operationStatus); // Check ranging status

    while (1) // Main loop
    {
        if (motorActivationFlag) // Check if motor should be activated
        {
            for (int measurementCycle = 0; measurementCycle < NUMBER_OF_MEASUREMENTS; measurementCycle++)
            {
                for (int sampleIndex = 0; sampleIndex < NUMBER_OF_SAMPLES; sampleIndex++)
                {
                    while (sensorDataReady == 0) // Wait until data is ready
                    {
                        operationStatus = VL53L1X_CheckForDataReady(tofSensorAddress, &sensorDataReady);
                        FlashLED4(1); // Flash an LED to signal data checking
                        VL53L1_WaitMs(tofSensorAddress, 2); // Brief wait
                    }

                    sensorDataReady = 0; // Reset data ready flag
                    operationStatus = VL53L1X_GetRangeStatus(tofSensorAddress, &sensorRangeStatus); // Get range status
                    operationStatus = VL53L1X_GetDistance(tofSensorAddress, &sensorDistance); // Get measured distance
                    FlashLED4(1); // Flash an LED to signal distance measurement

                    measuredDistances[sampleIndex] = sensorDistance; // Store measured distance
                    operationStatus = VL53L1X_ClearInterrupt(tofSensorAddress); // Clear sensor interrupt

                    spinMotor(512 / NUMBER_OF_SAMPLES, ROTATE_CLOCKWISE); // Spin motor a fractional rotation
                }

                for (int dataIndex = 0; dataIndex < NUMBER_OF_SAMPLES; dataIndex++)
                {
                    sprintf(printf_buffer, "%u,", measuredDistances[dataIndex]); // Format data for UART
                    uartSignal = UART_InChar(); // Wait for a signal from UART
                    if (uartSignal == 0x75) // Check if signal is 'u'
                    {
                        UART_printf(printf_buffer); // Send formatted data over UART
                    }
                }

                FlashLED3(1); // Flash an LED to signal end of measurement cycle
                spinMotor(512, ROTATE_COUNTERCLOCKWISE); // Return motor to starting position

                motorActivationFlag = 0; // Reset motor activation flag
            }
        }
    }
}
