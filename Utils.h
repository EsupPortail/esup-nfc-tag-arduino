
#ifndef UTILS_H
#define UTILS_H

// *********************************************************************************
// The following switches define how the Teensy communicates with the PN532 board.
// For the DoorOpener sketch the only valid option is Software SPI.
// For other projects with the PN532 you may change this.
// ATTENTION: Only one of the following defines must be set to true!
// NOTE: In Software SPI mode there is no external libraray required. Only 4 regular digital pins are used.
// If you want to transfer the code to another processor the easiest way will be to use Software SPI mode.
#define USE_SOFTWARE_SPI   true
#define USE_HARDWARE_SPI   false
#define USE_HARDWARE_I2C   false
// ********************************************************************************/

#include <Arduino.h>

#if USE_HARDWARE_SPI
    #include <SPI.h>  // Hardware SPI bus
#elif USE_HARDWARE_I2C
    #include <Wire.h> // Hardware I2C bus
#elif USE_SOFTWARE_SPI
    // no #include required
#else
    #error "You must specify the PN532 communication mode."
#endif

#define LF  "\r\n" // LineFeed 

// Teensy definitions for digital pins:
#ifndef INPUT
    #define OUTPUT   0x1
    #define INPUT    0x0
    #define HIGH     0x1
    #define LOW      0x0
#endif

// If you compile this project on Visual Studio...
#ifdef _MSC_VER 
  typedef unsigned __int64 uint64_t;
  typedef __int64           int64_t;
  typedef unsigned int     uint32_t;
  typedef int               int32_t;
  typedef unsigned short   uint16_t;
  typedef short             int16_t;
  typedef unsigned char     uint8_t;
  typedef char               int8_t;
    // disable "sprintf deprecated" warning
    #pragma warning(disable: 4996) 
#endif

// -------------------------------------------------------------------------------------------------------------------

// USB connection to Terminal program (Teraterm) on PC via COM port
// When you compile the code for Linux, Windows or any other platform you must modify this class.
// You can leave all functions empty and only redirect Print() to printf().
class SerialClass
{  
public:
    // Create a COM connection via USB.
    // Teensy ignores the baudrate parameter (only for older Arduino boards)
    static inline void Begin(uint32_t u32_Baud) 
    {
        Serial.begin(u32_Baud);
    }
    // returns how many characters the user has typed in the Terminal program on the PC which have not yet been read with Read()
    static inline int Available()
    {
        return Serial.available();
    }
    // Get the next character from the Terminal program on the PC
  // returns -1 if no character available
    static inline int Read()
    {
        return Serial.read();
    }
    // Print text to the Terminal program on the PC
  // On Windows/Linux use printf() here to write debug output an errors to the Console.
    static inline void Print(const char* s8_Text)
    {
        Serial.print(s8_Text);
    }
};

// -------------------------------------------------------------------------------------------------------------------

#if USE_HARDWARE_SPI
    // This class implements Hardware SPI (4 wire bus). It is not used for the DoorOpener sketch.
    // When you compile the code for Linux, Windows or any other platform you must modify this class.
    // NOTE: This class is not used when you switched to I2C mode with PN532::InitI2C() or Software SPI mode with PN532::InitSoftwareSPI().
    class SpiClass
    {  
    public:
        static inline void Begin(uint32_t u32_Clock) 
        {
            SPI.begin();
            SPI.beginTransaction(SPISettings(u32_Clock, LSBFIRST, SPI_MODE0));
        }
        // Write one byte to the MOSI pin and at the same time receive one byte on the MISO pin.
        static inline byte Transfer(byte u8_Data) 
        {
            return SPI.transfer(u8_Data);
        }
    };
#endif

// -------------------------------------------------------------------------------------------------------------------

#if USE_HARDWARE_I2C
    // This class implements Hardware I2C (2 wire bus with pull-up resistors). It is not used for the DoorOpener sketch.
    // When you compile the code for Linux, Windows or any other platform you must modify this class.
    // NOTE: This class is not used when you switched to SPI mode with PN532::InitSoftwareSPI() or PN532::InitHardwareSPI().
    class I2cClass
    {  
    public:
        // Initialize the I2C pins
        static inline void Begin() 
        {
            Wire.begin();
        }
        // --------------------- READ -------------------------
        // Read the requested amount of bytes at once from the I2C bus into an internal buffer.
        // ATTENTION: The Arduino library is extremely primitive. A timeout has not been implemented.
        // When the CLK line is permanently low this function hangs forever!
        static inline byte RequestFrom(byte u8_Address, byte u8_Quantity)
        {
            return Wire.requestFrom(u8_Address, u8_Quantity);
        }
        // Read one byte from the buffer that has been read when calling RequestFrom()
        static inline int Read()
        {
            return Wire.read();
        }
        // --------------------- WRITE -------------------------
        // Initiates a Send transmission
        static inline void BeginTransmission(byte u8_Address)
        {
            Wire.beginTransmission(u8_Address);
        }
        // Write one byte to the I2C bus
        static inline void Write(byte u8_Data)
        {
            Wire.write(u8_Data);
        }
        // Ends a Send transmission
        static inline void EndTransmission()
        {
            Wire.endTransmission();
        }
    };
#endif

// -------------------------------------------------------------------------------------------------------------------

class Utils
{
public:
    // returns the current tick counter
  // When you compile the code for Linux, Windows or any other platform you must change this function.
  // On Windows use GetTickCount() here
    static inline uint32_t GetMillis()
    {
        return millis();
    }

  // When you compile the code for Linux, Windows or any other platform you must change this function.
  // Use Sleep() here.
    static inline void DelayMilli(int s32_MilliSeconds)
    {
        delay(s32_MilliSeconds);
    }

  // This function is only required for Software SPI mode.
    // When you compile the code for Linux, Windows or any other platform you must change this function.
  // There is no API in Windows that supports delays shorter than approx 20 milli seconds. (Sleep(1) will sleep approx 20 ms)
  // To implement delays in micro seconds you can use a loop that runs until a performance counter has reached the expected value.
  // On Windows use: while(...) { .. QueryPerformanceCounter() .. if (Counter > X) break; .. }
    static inline void DelayMicro(int s32_MicroSeconds)
    {
        delayMicroseconds(s32_MicroSeconds);
    }
    
  // Defines if a digital processor pin is used as input or output
    // u8_Mode = INPUT or OUTPUT
    // When you compile the code for Linux, Windows or any other platform you must change this function.  
    static inline void SetPinMode(byte u8_Pin, byte u8_Mode)
    {
        pinMode(u8_Pin, u8_Mode);
    }
    
  // Sets a digital processor pin high or low.
    // u8_Status = HIGH or LOW
  // When you compile the code for Linux, Windows or any other platform you must change this function.
    static inline void WritePin(byte u8_Pin, byte u8_Status)
    {
        digitalWrite(u8_Pin, u8_Status);
    }

  // reads the current state of a digital processor pin.
    // returns HIGH or LOW
    // When you compile the code for Linux, Windows or any other platform you must change this function.  
    static inline byte ReadPin(byte u8_Pin)
    {
        return digitalRead(u8_Pin);
    }

    static uint64_t GetMillis64();
    static void     Print(const char*   s8_Text,  const char* s8_LF=NULL);
    static void     PrintDec  (int      s32_Data, const char* s8_LF=NULL);
    static void     PrintHex8 (byte     u8_Data,  const char* s8_LF=NULL);
    static void     PrintHex16(uint16_t u16_Data, const char* s8_LF=NULL);
    static void     PrintHex32(uint32_t u32_Data, const char* s8_LF=NULL);
    static void     PrintHexBuf(const byte* u8_Data, const uint32_t u32_DataLen, const char* s8_LF=NULL, int s32_Brace1=-1, int S32_Brace2=-1);
    static void     PrintInterval(uint64_t u64_Time, const char* s8_LF=NULL);
    static void     GenerateRandom(byte* u8_Random, int s32_Length);
    static void     RotateBlockLeft(byte* u8_Out, const byte* u8_In, int s32_Length);
    static void     BitShiftLeft(uint8_t* u8_Data, int s32_Length);
    static void     XorDataBlock(byte* u8_Out,  const byte* u8_In, const byte* u8_Xor, int s32_Length);    
    static void     XorDataBlock(byte* u8_Data, const byte* u8_Xor, int s32_Length);
    static uint16_t CalcCrc16(const byte* u8_Data,  int s32_Length);
    static uint32_t CalcCrc32(const byte* u8_Data1, int s32_Length1, const byte* u8_Data2=NULL, int s32_Length2=0);

private:
    static uint32_t CalcCrc32(const byte* u8_Data, int s32_Length, uint32_t u32_Crc);
};

#endif // UTILS_H
