/**************************************************************************
    
    @author   Elmü
    This class is a safe buffer class which prevents buffer overflows.
    The design of this class avoids the need for the 'new' operator which would lead to memory fragmentation.

    Check for a new version on:
    http://www.codeproject.com/Articles/1096861/DIY-electronic-RFID-Door-Lock-with-Battery-Backup
  
**************************************************************************/


#ifndef BUFFER_H
#define BUFFER_H

#include "Utils.h"

// These macros create a new buffer on the stack avoiding the use of the 'new' operator.
// ATTENTION: 
// These macros will not work if you define the TxBuffer/RxBuffer as member of a class (see mi_CmacBuffer). 
// They compile only inside the code of a function.
//
// TX_BUFFER(i_SessKey, 16)  
// Creates an instance i_SessKey with the size of 16 bytes and is equivalent to:
// byte i_SessKey_Buffer[16];
// TxBuffer i_SessKey(i_SessKey_Buffer, 16);
//
// The byte buffer is created outside the class to avoid the need of a template class which would have several disadvantages.
// The operator "##" tells the preprocessor to concatenate two strings.
#define TX_BUFFER(buffer_name, size) \
    byte buffer_name##_Buffer[size]; \
    TxBuffer buffer_name(buffer_name##_Buffer, size);

#define RX_BUFFER(buffer_name, size) \
    byte buffer_name##_Buffer[size]; \
    RxBuffer buffer_name(buffer_name##_Buffer, size);


// This class makes a buffer overflow impossible
class RxBuffer
{
public:
    inline RxBuffer(byte* u8_Buffer, int s32_Size) 
    { 
        mu8_Buf   = u8_Buffer;
        ms32_Size = s32_Size;
        ms32_Pos  = 0;        
        memset(mu8_Buf, 0, s32_Size); 
    }

	// This allows to shrink the available maximum buffer size, but it does not allow to make the buffer larger.
    bool SetSize(int s32_NewSize)
    {
        ms32_Pos = 0;
        if (!CheckPos(s32_NewSize))
            return false;
      
        ms32_Size = s32_NewSize;
        return true;
    }
    
    inline int GetSize()
    {
        return ms32_Size;
    }
    
    inline byte* GetData()
    {
        return mu8_Buf;
    }
       
    inline operator byte*() 
    { 
        return mu8_Buf; 
    }

    // reads 1 byte from the buffer and increments the pointer
    uint8_t ReadUint8()
    {
        if (!CheckPos(1)) return 0;
      
        uint8_t u8_Data = mu8_Buf[ms32_Pos];
        ms32_Pos += 1;
        return u8_Data;
    }
    
    // reads 2 bytes from the buffer and increments the pointer
    uint16_t ReadUint16()
    {
        if (!CheckPos(2)) return 0;
      
        uint16_t u16_Data;
        memcpy(&u16_Data, mu8_Buf + ms32_Pos, 2);
        ms32_Pos += 2;
        return u16_Data;
    }
    
    // reads 3 bytes from the buffer and increments the pointer
    uint32_t ReadUint24()
    {
        if (!CheckPos(3)) return 0;
      
        uint32_t u32_Data = 0;
        memcpy(&u32_Data, mu8_Buf + ms32_Pos, 3); // Copy only the lower 3 bytes
        ms32_Pos += 3;
        return u32_Data;
    }
    
    // reads 4 bytes from the buffer and increments the pointer
    uint32_t ReadUint32()
    {
        if (!CheckPos(4)) return 0;
      
        uint32_t u32_Data;
        memcpy(&u32_Data, mu8_Buf + ms32_Pos, 4);
        ms32_Pos += 4;
        return u32_Data;
    }

    bool ReadBuf(byte* u8_Buffer, int s32_Count)
    {
        if (!CheckPos(s32_Count)) return false;
      
        memcpy(u8_Buffer, mu8_Buf + ms32_Pos, s32_Count);
        ms32_Pos += s32_Count;
        return true;
    }

private:
    bool CheckPos(int s32_Count)
    {
        if (ms32_Pos + s32_Count <= ms32_Size)
            return true;
        
        //Utils::Print("### RxBuffer Overflow ###\r\n");
        return false;
    }
   
    byte* mu8_Buf;
    int   ms32_Size;
    int   ms32_Pos;
};

// ====================================================================================

// This class makes a buffer overflow impossible
class TxBuffer
{
public:
    inline TxBuffer(byte* u8_Buffer, int s32_Size) 
    { 
        mu8_Buf   = u8_Buffer;
        ms32_Size = s32_Size;
        ms32_Pos  = 0;        
        memset(mu8_Buf, 0, s32_Size); 
    }

    // Resets the byte counter
    inline void Clear()
    {
        ms32_Pos = 0;
    }

    inline int GetSize()
    {
        return ms32_Size;
    }

    // returns the count of bytes that are currently free in the buffer
    inline int GetFree()
    {
        return ms32_Size - ms32_Pos;
    }

    // returns the count of bytes that are currently occupied in the buffer
    inline int GetCount()
    {
        return ms32_Pos;
    }

    // ATTENTION: Use this only after encrypting the buffer.
    bool SetCount(int s32_Count)
    {
        ms32_Pos = 0;
        if (!CheckPos(s32_Count))
            return false;
      
        ms32_Pos = s32_Count;
        return true;
    }   

    inline byte* GetData()
    {
        return mu8_Buf;
    }
    
    inline operator byte*() 
    { 
        return mu8_Buf; 
    }

    // writes 1 byte to the buffer and increments the pointer
    bool AppendUint8(uint8_t u8_Data)
    {
        if (!CheckPos(1)) return false;
      
        mu8_Buf[ms32_Pos] = u8_Data;
        ms32_Pos += 1;
        return true;
    }
    
    // writes n bytes to the buffer and increments the pointer
    bool AppendBuf(const uint8_t* u8_Data, int s32_Count)
    {
        // u8_Data may be NULL if s32_Count == 0!
        if (s32_Count == 0)
            return true;
      
        if (!CheckPos(s32_Count)) return false;
      
        memcpy(mu8_Buf + ms32_Pos, u8_Data, s32_Count);
        ms32_Pos += s32_Count;
        return true;
    }
    
    // writes 2 bytes to the buffer and increments the pointer
    bool AppendUint16(uint16_t u16_Data)
    {
        if (!CheckPos(2)) return false;
      
        memcpy(mu8_Buf + ms32_Pos, &u16_Data, 2);
        ms32_Pos += 2;
        return true;
    }

    // writes 3 bytes to the buffer and increments the pointer
    bool AppendUint24(uint32_t u32_Data)
    {
        if (!CheckPos(3)) return false;
      
        memcpy(mu8_Buf + ms32_Pos, &u32_Data, 3);
        ms32_Pos += 3;
        return true;
    }

    // writes 4 bytes to the buffer and increments the pointer
    bool AppendUint32(uint32_t u32_Data)
    {
        if (!CheckPos(4)) return false;
      
        memcpy(mu8_Buf + ms32_Pos, &u32_Data, 4);
        ms32_Pos += 4;
        return true;
    }

private:
    bool CheckPos(int s32_Count)
    {
        if (ms32_Pos + s32_Count <= ms32_Size)
            return true;
        
        //Utils::Print("### TxBuffer Overflow ###\r\n");
        return false;
    }

    byte* mu8_Buf;
    int   ms32_Size;
    int   ms32_Pos;
};

#endif // BUFFER_H
