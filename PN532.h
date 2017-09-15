
#ifndef ADAFRUIT_PN532_H
#define ADAFRUIT_PN532_H

#include "Utils.h"

// ----------------------------------------------------------------------

// This parameter may be used to slow down the software SPI bus speed.
// This is required when there is a long cable between the PN532 and the Teensy.
// This delay in microseconds (not milliseconds!) is made between toggeling the CLK line.
// Use an oscilloscope to check the resulting speed!
// A value of 50 microseconds results in a clock signal of 10 kHz
// A value of 0 results in maximum speed (depends on CPU speed).
// This parameter is not used for hardware SPI mode.
#define PN532_SOFT_SPI_DELAY  50

// The clock (in Hertz) when using Hardware SPI mode
// This parameter is not used for software SPI mode.
#define PN532_HARD_SPI_CLOCK  1000000

// The maximum time to wait for an answer from the PN532
// Do NOT use infinite timeouts like in Adafruit code!
#define PN532_TIMEOUT  1000

// The packet buffer is used for sending commands and for receiving responses from the PN532
#define PN532_PACKBUFFSIZE   80

// ----------------------------------------------------------------------

#define PN532_PREAMBLE                      (0x00)
#define PN532_STARTCODE1                    (0x00)
#define PN532_STARTCODE2                    (0xFF)
#define PN532_POSTAMBLE                     (0x00)

#define PN532_HOSTTOPN532                   (0xD4)
#define PN532_PN532TOHOST                   (0xD5)

// PN532 Commands
#define PN532_COMMAND_DIAGNOSE              (0x00)
#define PN532_COMMAND_GETFIRMWAREVERSION    (0x02)
#define PN532_COMMAND_GETGENERALSTATUS      (0x04)
#define PN532_COMMAND_READREGISTER          (0x06)
#define PN532_COMMAND_WRITEREGISTER         (0x08)
#define PN532_COMMAND_READGPIO              (0x0C)
#define PN532_COMMAND_WRITEGPIO             (0x0E)
#define PN532_COMMAND_SETSERIALBAUDRATE     (0x10)
#define PN532_COMMAND_SETPARAMETERS         (0x12)
#define PN532_COMMAND_SAMCONFIGURATION      (0x14)
#define PN532_COMMAND_POWERDOWN             (0x16)
#define PN532_COMMAND_RFCONFIGURATION       (0x32)
#define PN532_COMMAND_RFREGULATIONTEST      (0x58)
#define PN532_COMMAND_INJUMPFORDEP          (0x56)
#define PN532_COMMAND_INJUMPFORPSL          (0x46)
#define PN532_COMMAND_INLISTPASSIVETARGET   (0x4A)
#define PN532_COMMAND_INATR                 (0x50)
#define PN532_COMMAND_INPSL                 (0x4E)
#define PN532_COMMAND_INDATAEXCHANGE        (0x40)
#define PN532_COMMAND_INCOMMUNICATETHRU     (0x42)
#define PN532_COMMAND_INDESELECT            (0x44)
#define PN532_COMMAND_INRELEASE             (0x52)
#define PN532_COMMAND_INSELECT              (0x54)
#define PN532_COMMAND_INAUTOPOLL            (0x60)
#define PN532_COMMAND_TGINITASTARGET        (0x8C)
#define PN532_COMMAND_TGSETGENERALBYTES     (0x92)
#define PN532_COMMAND_TGGETDATA             (0x86)
#define PN532_COMMAND_TGSETDATA             (0x8E)
#define PN532_COMMAND_TGSETMETADATA         (0x94)
#define PN532_COMMAND_TGGETINITIATORCOMMAND (0x88)
#define PN532_COMMAND_TGRESPONSETOINITIATOR (0x90)
#define PN532_COMMAND_TGGETTARGETSTATUS     (0x8A)

#define PN532_WAKEUP                        (0x55)

#define PN532_SPI_STATUSREAD                (0x02)
#define PN532_SPI_DATAWRITE                 (0x01)
#define PN532_SPI_DATAREAD                  (0x03)
#define PN532_SPI_READY                     (0x01)

#define PN532_I2C_ADDRESS                   (0x48 >> 1)
#define PN532_I2C_READY                     (0x01)

#define PN532_GPIO_P30                      (0x01)
#define PN532_GPIO_P31                      (0x02)
#define PN532_GPIO_P32                      (0x04)
#define PN532_GPIO_P33                      (0x08)
#define PN532_GPIO_P34                      (0x10)
#define PN532_GPIO_P35                      (0x20)
#define PN532_GPIO_VALIDATIONBIT            (0x80)

#define CARD_TYPE_106KB_ISO14443A           (0x00) // card baudrate 106 kB
#define CARD_TYPE_212KB_FELICA              (0x01) // card baudrate 212 kB
#define CARD_TYPE_424KB_FELICA              (0x02) // card baudrate 424 kB
#define CARD_TYPE_106KB_ISO14443B           (0x03) // card baudrate 106 kB
#define CARD_TYPE_106KB_JEWEL               (0x04) // card baudrate 106 kB

enum eCardType
{
    CARD_Unknown   = 0, // Mifare Classic or other card
    CARD_Desfire   = 1, // A Desfire card with normal 7 byte UID  (bit 0)
    CARD_DesRandom = 3, // A Desfire card with 4 byte random UID  (bit 0 + 1)
};

class PN532
{
 public:
    PN532();
    
    #if USE_SOFTWARE_SPI
        void InitSoftwareSPI(byte u8_Clk, byte u8_Miso, byte u8_Mosi, byte u8_Sel, byte u8_Reset);
    #endif
   
    // Generic PN532 functions
    void begin();  
    void SetDebugLevel(byte level);
    bool SamConfig();
    bool GetFirmwareVersion(byte* pIcType, byte* pVersionHi, byte* pVersionLo, byte* pFlags);
    bool WriteGPIO(bool P30, bool P31, bool P33, bool P35);
    bool SetPassiveActivationRetries();
    bool DeselectCard();
    bool ReleaseCard();
    bool SelectCard();

    // This function is overridden in Desfire.cpp
    virtual bool SwitchOffRfField();
            
    // ISO14443A functions
    bool ReadPassiveTargetID(byte* uidBuffer, byte* uidLength, eCardType* pe_CardType);

    // Low Level functions
    bool CheckPN532Status(byte u8_Status);
    bool SendCommandCheckAck(byte *cmd, byte cmdlen);    
    byte ReadData    (byte* buff, byte len);
    bool ReadPacket  (byte* buff, byte len);
    void WriteCommand(byte* cmd,  byte cmdlen);
    void SendPacket  (byte* buff, byte len);
    bool IsReady();
    bool WaitReady();
    bool ReadAck();
    void SpiWrite(byte c);
    byte SpiRead(void);

    byte mu8_DebugLevel;   // 0, 1, or 2
    byte mu8_PacketBuffer[PN532_PACKBUFFSIZE];

 private:
    byte mu8_ClkPin;
    byte mu8_MisoPin;  
    byte mu8_MosiPin;  
    byte mu8_SselPin;  
    byte mu8_ResetPin;
};

#endif
