#include "init.h"

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

void I2C_Init(void)
{
    SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0;   // activate I2C0
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1; // activate port B
    while ((SYSCTL_PRGPIO_R & 0x0002) == 0)
    {
    }; // ready?

    GPIO_PORTB_AFSEL_R |= 0x0C; // 3) enable alt funct on PB2,3       0b00001100
    GPIO_PORTB_ODR_R |= 0x08;   // 4) enable open drain on PB3 only

    GPIO_PORTB_DEN_R |= 0x0C; // 5) enable digital I/O on PB2,3
                              //    GPIO_PORTB_AMSEL_R &= ~0x0C; // 7) disable analog functionality on PB2,3

    // 6) configure PB2,3 as I2C
    //  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00003300;
    GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R & 0xFFFF00FF) + 0x00002200; // TED
    I2C0_MCR_R = I2C_MCR_MFE;                                          // 9) master function enable
    I2C0_MTPR_R = 0b0000000000000101000000000111011; // 8) configure for 100 kbps clock (added 8 clocks of glitch
                                                     // suppression ~50ns)
    //    I2C0_MTPR_R = 0x3B;                                        						// 8) configure for 100 kbps
    //    clock
}

// The VL53L1X needs to be reset using XSHUT.  We will use PG0
void PortG_Init(void)
{
    // Use PortG0
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R6; // activate clock for Port N
    while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R6) == 0)
    {
    };                           // allow time for clock to stabilize
    GPIO_PORTG_DIR_R &= 0x00;    // make PG0 in (HiZ)
    GPIO_PORTG_AFSEL_R &= ~0x01; // disable alt funct on PG0
    GPIO_PORTG_DEN_R |= 0x01;    // enable digital I/O on PG0
                                 // configure PG0 as GPIO
    // GPIO_PORTN_PCTL_R = (GPIO_PORTN_PCTL_R&0xFFFFFF00)+0x00000000;
    GPIO_PORTG_AMSEL_R &= ~0x01; // disable analog functionality on PN0

    return;
}

// XSHUT     This pin is an active-low shutdown input;
//					the board pulls it up to VDD to enable the sensor by default.
//					Driving this pin low puts the sensor into hardware standby. This input is not level-shifted.
void VL53L1X_XSHUT(void)
{
    GPIO_PORTG_DIR_R |= 0x01;        // make PG0 out
    GPIO_PORTG_DATA_R &= 0b11111110; // PG0 = 0
    FlashAllLEDs();
    SysTick_Wait10ms(10);
    GPIO_PORTG_DIR_R &= ~0x01; // make PG0 input (HiZ)
}

void PortH_Init(void)
{
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R7; // Activate the clock for Port H
    while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R7) == 0)
    {
    }; // Allow time for clock to stabilize

    GPIO_PORTH_DIR_R |= 0x0F;    // Enable PH0 and PH1 as inputs
    GPIO_PORTH_AFSEL_R &= ~0x0F; // disable alt funct on Port M pins (PM0-PM3)
    GPIO_PORTH_DEN_R |= 0x0F;    // Enable PH0, PH1, PH2, and PH3 as digital pins
    GPIO_PORTH_AMSEL_R &= ~0x0F; // disable analog functionality on Port M	pins (PM0-PM3)

    return;
}

// Enable interrupts
void EnableInt(void)
{
    __asm("    cpsie   i\n");
}

// Disable interrupts
void DisableInt(void)
{
    __asm("    cpsid   i\n");
}

// Low power wait
void WaitForInt(void)
{
    __asm("    wfi\n");
}

// Global variable visible in Watch window of debugger
// Increments at least once per button press
volatile unsigned long FallingEdges = 0;

// Give clock to Port J and initalize PJ1 as Digital Input GPIO
void PortJ_Init(void)
{
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R8; // Activate clock for Port J
    while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R8) == 0)
    {
    };                         // Allow time for clock to stabilize
    GPIO_PORTJ_DIR_R &= ~0x02; // Make PJ1 input
    GPIO_PORTJ_DEN_R |= 0x02;  // Enable digital I/O on PJ1

    GPIO_PORTJ_PCTL_R &= ~0x000000F0; //  Configure PJ1 as GPIO
    GPIO_PORTJ_AMSEL_R &= ~0x02;      //  Disable analog functionality on PJ1
    GPIO_PORTJ_PUR_R |= 0x02;         //	Enable weak pull up resistor on PJ1
}

// Interrupt initialization for GPIO Port J IRQ# 51
void PortJ_Interrupt_Init(void)
{

    FallingEdges = 0; // Initialize counter

    GPIO_PORTJ_IS_R = 0;     // (Step 1) PJ1 is Edge-sensitive
    GPIO_PORTJ_IBE_R = 0;    //     			PJ1 is not triggered by both edges
    GPIO_PORTJ_IEV_R = 0;    //     			PJ1 is falling edge event
    GPIO_PORTJ_ICR_R = 0x02; // 					Clear interrupt flag by setting proper bit in ICR register
    GPIO_PORTJ_IM_R = 0x02;  // 					Arm interrupt on PJ1 by setting proper bit in IM register

    NVIC_EN1_R = 0x00080000; // (Step 2) Enable interrupt 51 in NVIC (which is in Register EN1)

    NVIC_PRI12_R = 0xA0000000; // (Step 4) Set interrupt priority to 5

    EnableInt(); // (Step 3) Enable Global Interrupt. lets go!
}

void PortM_Init(void)
{
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R11; // Activate the clock for Port L
    while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R11) == 0)
    {
    }; // Allow time for clock to stabilize

    GPIO_PORTM_DIR_R |= 0x01;    // Enable PM0 and PM1 as inputs
    GPIO_PORTM_AFSEL_R &= ~0x01; // disable alt funct on Port M pins (PM0-PM3)
    GPIO_PORTM_DEN_R |= 0x01;    // Enable PM0, PM1, PM2, and PM3 as digital pins
    GPIO_PORTM_AMSEL_R &= ~0x01; // disable analog functionality on Port M	pins (PM0-PM3)

    return;
}
