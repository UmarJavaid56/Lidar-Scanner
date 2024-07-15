
#define I2C_MCS_ACK 0x00000008    // Data Acknowledge Enable
#define I2C_MCS_DATACK 0x00000008 // Acknowledge Data
#define I2C_MCS_ADRACK 0x00000004 // Acknowledge Address
#define I2C_MCS_STOP 0x00000004   // Generate STOP
#define I2C_MCS_START 0x00000002  // Generate START
#define I2C_MCS_ERROR 0x00000002  // Error
#define I2C_MCS_RUN 0x00000001    // I2C Master Enable
#define I2C_MCS_BUSY 0x00000001   // I2C Busy
#define I2C_MCR_MFE 0x00000010    // I2C Master Function Enable

#define MAXRETRIES 5 // number of receive attempts before giving up

void I2C_Init(void);

// The VL53L1X needs to be reset using XSHUT.  We will use PG0
void PortG_Init(void);

void PortH_Init(void);

// XSHUT        This pin is an active-low shutdown input;
//              the board pulls it up to VDD to enable the sensor by default.
//              Driving this pin low puts the sensor into hardware standby. This input is not level-shifted.
void VL53L1X_XSHUT(void);

// Enable interrupts
void EnableInt(void);

// Disable interrupts
void DisableInt(void);

// Low power wait
void WaitForInt(void);

// Give clock to Port J and initalize PJ1 as Digital Input GPIO
void PortJ_Init(void);

// Interrupt initialization for GPIO Port J IRQ# 51
void PortJ_Interrupt_Init(void);

void PortM_Init(void);