/************************************************************************************
 * 
 * @author   Elmü
 * 
 * This library has been tested with Desfire EV1 cards. 
 * It will surely not work with older Desfire cards (deprecated) because legacy authentication is not implemented.
 * I have older code with lagacy authentication. If you are interested contact me on Codeproject.
 * 
 * This library is based on code from the following open source libraries:
 * https://github.com/nceruchalu/easypay
 * https://github.com/leg0/libfreefare
 * http://liblogicalaccess.islog.com
 * 
 * The open source code has been completely rewritten for the Arduino compiler by Elmü.
 * Check for a new version on:
 * http://www.codeproject.com/Articles/1096861/DIY-electronic-RFID-Door-Lock-with-Battery-Backup
 * 
*************************************************************************************
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "Desfire.h"

Desfire::Desfire() 
    : mi_CmacBuffer(mu8_CmacBuffer_Data, sizeof(mu8_CmacBuffer_Data))
{
    mu8_LastAuthKeyNo    = NOT_AUTHENTICATED;
    mu8_LastPN532Error   = 0;    
    mu32_LastApplication = 0x000000; // No application selected

    // The PICC master key on an empty card is a simple DES key filled with 8 zeros
    const byte ZERO_KEY[24] = {0};

}

// Whenever the RF field is switched off, these variables must be reset
bool Desfire::SwitchOffRfField()
{
    mu8_LastAuthKeyNo    = NOT_AUTHENTICATED;
    mu32_LastApplication = 0x000000; // No application selected

    return PN532::SwitchOffRfField();
}


/**************************************************************************
    Enables random ID mode in which the card sends another UID each time.
    In Random UID mode the card sends a 4 byte UID that always starts with 0x80.
    To get the real UID of the card call GetRealCardID()
    ---------------------------------------------------------------------
    ATTENTION ATTENTION ATTENTION ATTENTION ATTENTION ATTENTION ATTENTION
    ---------------------------------------------------------------------
    NXP does not provide any way to turn off Random ID mode.
    If you once call this funtion the card will send random ID FOREVER!
    ---------------------------------------------------------------------
    ATTENTION ATTENTION ATTENTION ATTENTION ATTENTION ATTENTION ATTENTION
    ---------------------------------------------------------------------
**************************************************************************/
bool Desfire::EnableRandomIDForever()
{
    //if (mu8_DebugLevel > 0) Utils::Print("\r\n*** EnableRandomIDForever()\r\n");

    TX_BUFFER(i_Command, 2);
    i_Command.AppendUint8(DFEV1_INS_SET_CONFIGURATION);
    i_Command.AppendUint8(0x00); // subcommand 00
    
    TX_BUFFER(i_Params, 16);
    i_Params.AppendUint8(0x02); // 0x02 = enable random ID, 0x01 = disable format

    // The TX CMAC must not be calculated here because a CBC encryption operation has already been executed
    return (0 == DataExchange(&i_Command, &i_Params, NULL, 0, NULL, MAC_TcryptRmac));
}

/**************************************************************************
    This command makes only sense if the card is in Random UID mode.
    It allows to obtain the real UID of the card.
    If Random ID mode is not active use ReadPassiveTargetID() or GetCardVersion()
    instead to get the UID.
    A previous authentication is required.
**************************************************************************/
bool Desfire::GetRealCardID(byte u8_UID[7])
{
    //if (mu8_DebugLevel > 0) Utils::Print("\r\n*** GetRealCardID()\r\n");

    if (mu8_LastAuthKeyNo == NOT_AUTHENTICATED)
    {
        //Utils::Print("Not authenticated\r\n");
        return false;
    }

    RX_BUFFER(i_Data, 16);
    if (16 != DataExchange(DFEV1_INS_GET_CARD_UID, NULL, i_Data, 16, NULL, MAC_TmacRcrypt))
        return false;

    // The card returns UID[7] + CRC32[4] encrypted with the session key
    // Copy the 7 bytes of the UID to the output buffer
    i_Data.ReadBuf(u8_UID, 7);

    // Get the CRC sent by the card
    uint32_t u32_Crc1 = i_Data.ReadUint32();

    // The CRC must be calculated over the UID + the status byte appended
    byte u8_Status = ST_Success;
    uint32_t u32_Crc2 = Utils::CalcCrc32(u8_UID, 7, &u8_Status, 1);

    if (mu8_DebugLevel > 1)
    {
        Utils::Print("* CRC:       0x");
        Utils::PrintHex32(u32_Crc2, LF);
    }

    if (u32_Crc1 != u32_Crc2)
    {
        //Utils::Print("Invalid CRC\r\n");
        return false;
    }

    if (mu8_DebugLevel > 0)
    {
        //Utils::Print("Real UID: ");
        Utils::PrintHexBuf(u8_UID, 7, LF);
    }
    return true;
}


// ########################################################################
// ####                      LOW LEVEL FUNCTIONS                      #####
// ########################################################################

// If this value is != 0, the PN532 has returned an error code while executing the latest command.
// Typically a Timeout error (Value = 0x01) means that the card is too far away from the reader.
// Interestingly a timeout occurres typically when authenticating. 
// The commands that are executed first (GetKeyVersion and SelectApplication) execute without problems.
// But it when it comes to Authenticate() the card suddenly does not respond anymore -> Timeout from PN532.
// Conclusion: It seems that a Desfire card increases its power consumption in the moment when encrypting data,
// so when it is too far away from the antenna -> the connection dies.
byte Desfire::GetLastPN532Error()
{
    return mu8_LastPN532Error;
}

/**************************************************************************
    Sends data to the card and receives the response.
    u8_Command    = Desfire command without additional paramaters
    pi_Command    = Desfire command + possible additional paramaters that will not be encrypted
    pi_Params     = Desfire command parameters that may be encrypted (MAC_Tcrypt). This paramater may also be null.
    u8_RecvBuf    = buffer that receives the received data (should be the size of the expected recv data)
   s32_RecvSize   = buffer size of u8_RecvBuf
    pe_Status     = if (!= NULL) -> receives the status byte
    e_Mac         = defines CMAC calculation
    returns the byte count that has been read into u8_RecvBuf or -1 on error
**************************************************************************/
int Desfire::DataExchange(byte u8_Command, TxBuffer* pi_Params, byte* u8_RecvBuf, int s32_RecvSize, DESFireStatus* pe_Status, DESFireCmac e_Mac)
{
    TX_BUFFER(i_Command, 1);
    i_Command.AppendUint8(u8_Command);
  
    return DataExchange(&i_Command, pi_Params, u8_RecvBuf, s32_RecvSize, pe_Status, e_Mac);
}
int Desfire::DataExchange(TxBuffer* pi_Command,               // in (command + params that are not encrypted)
                          TxBuffer* pi_Params,                // in (parameters that may be encrypted)
                          byte* u8_RecvBuf, int s32_RecvSize, // out
                          DESFireStatus* pe_Status,
                          DESFireCmac    e_Mac)               // in
{
    if (pe_Status) *pe_Status = ST_Success;
    mu8_LastPN532Error = 0;

    TX_BUFFER(i_Empty, 1);
    if (pi_Params == NULL)
        pi_Params = &i_Empty;

    // The response for INDATAEXCHANGE is always: 
    // - 0xD5
    // - 0x41
    // - Status byte from PN532        (0 if no error)
    // - Status byte from Desfire card (0 if no error)
    // - data bytes ...
    int s32_Overhead = 11; // Overhead added to payload = 11 bytes = 7 bytes for PN532 frame + 3 bytes for INDATAEXCHANGE response + 1 card status byte
    if (e_Mac & MAC_Rmac) s32_Overhead += 8; // + 8 bytes for CMAC
  
    // mu8_PacketBuffer is used for input and output
    if (2 + pi_Command->GetCount() + pi_Params->GetCount() > PN532_PACKBUFFSIZE || s32_Overhead + s32_RecvSize > PN532_PACKBUFFSIZE)    
    {
        //Utils::Print("DataExchange(): Invalid parameters\r\n");
        return -1;
    }

    if (e_Mac & (MAC_Tcrypt | MAC_Rcrypt))
    {
        if (mu8_LastAuthKeyNo == NOT_AUTHENTICATED)
        {
            //Utils::Print("Not authenticated\r\n");
            return -1;
        }
    }

    byte u8_Command = pi_Command->GetData()[0];

    int P=0;
    mu8_PacketBuffer[P++] = PN532_COMMAND_INDATAEXCHANGE;
    mu8_PacketBuffer[P++] = 1; // Card number (Logical target number)

    memcpy(mu8_PacketBuffer + P, pi_Command->GetData(), pi_Command->GetCount());
    P += pi_Command->GetCount();

    memcpy(mu8_PacketBuffer + P, pi_Params->GetData(),  pi_Params->GetCount());
    P += pi_Params->GetCount();

    if (!SendCommandCheckAck(mu8_PacketBuffer, P))
        return -1;

    byte s32_Len = ReadData(mu8_PacketBuffer, s32_RecvSize + s32_Overhead);

    // ReadData() returns 3 byte if status error from the PN532
    // ReadData() returns 4 byte if status error from the Desfire card
    if (s32_Len < 3 || mu8_PacketBuffer[1] != PN532_COMMAND_INDATAEXCHANGE + 1)
    {
        //Utils::Print("DataExchange() failed\r\n");
        return -1;
    }

    // Here we get two status bytes that must be checked
    byte u8_PN532Status = mu8_PacketBuffer[2]; // contains errors from the PN532
    byte u8_CardStatus  = mu8_PacketBuffer[3]; // contains errors from the Desfire card

    mu8_LastPN532Error = u8_PN532Status;

    if (!CheckPN532Status(u8_PN532Status) ||s32_Len < 4)
        return -1;

    // After any error that the card has returned the authentication is invalidated.
    // The card does not send any CMAC anymore until authenticated anew.
    if (u8_CardStatus != ST_Success && u8_CardStatus != ST_MoreFrames)
    {
        mu8_LastAuthKeyNo = NOT_AUTHENTICATED; // A new authentication is required now
    }

    if (!CheckCardStatus((DESFireStatus)u8_CardStatus))
        return -1;
    if (pe_Status)
       *pe_Status = (DESFireStatus)u8_CardStatus;

    s32_Len -= 4; // 3 bytes for INDATAEXCHANGE response + 1 byte card status

    // A CMAC may be appended to the end of the frame.
    // The CMAC calculation is important because it maintains the IV of the session key up to date.
    // If the IV is out of sync with the IV in the card, the next encryption with the session key will result in an Integrity Error.
    if ((e_Mac & MAC_Rmac) &&                                              // Calculate RX CMAC only if the caller requests it
        (u8_CardStatus == ST_Success || u8_CardStatus == ST_MoreFrames) && // In case of an error there is no CMAC in the response
        (mu8_LastAuthKeyNo != NOT_AUTHENTICATED))                          // No session key -> no CMAC calculation possible
    {
        // For example GetCardVersion() calls DataExchange() 3 times:
        // 1. u8_Command = DF_INS_GET_VERSION      -> clear CMAC buffer + append received data
        // 2. u8_Command = DF_INS_ADDITIONAL_FRAME -> append received data
        // 3. u8_Command = DF_INS_ADDITIONAL_FRAME -> append received data
        if (u8_Command != DF_INS_ADDITIONAL_FRAME)
        {
            mi_CmacBuffer.Clear();
        }

        // This is an intermediate frame. More frames will follow. There is no CMAC in the response yet.
        if (u8_CardStatus == ST_MoreFrames)
        {
            if (!mi_CmacBuffer.AppendBuf(mu8_PacketBuffer + 4, s32_Len))
                return -1;
        }
        
        if ((s32_Len >= 8) &&             // If the response is shorter than 8 bytes it surely does not contain a CMAC
           (u8_CardStatus == ST_Success)) // Response contains CMAC only in case of success
        {
            s32_Len -= 8; // Do not return the received CMAC to the caller and do not include it into the CMAC calculation
          
            byte* u8_RxMac = mu8_PacketBuffer + 4 + s32_Len;

        }
    }

    if (s32_Len > s32_RecvSize)
    {
        //Utils::Print("DataExchange() Buffer overflow\r\n");
        return -1;
    } 

    if (u8_RecvBuf && s32_Len)
    {
        memcpy(u8_RecvBuf, mu8_PacketBuffer + 4, s32_Len);

        if (e_Mac & MAC_Rcrypt) // decrypt received data with session key
        {

            if (mu8_DebugLevel > 1)
            {
                Utils::Print("Decrypt:  ");
                Utils::PrintHexBuf(u8_RecvBuf, s32_Len, LF);
            }        
        }    
    }
    return s32_Len;
}

// Checks the status byte that is returned from the card
bool Desfire::CheckCardStatus(DESFireStatus e_Status)
{
/*
    char s8_Buf[24];
    sprintf(s8_Buf, "Desfire status 0x%02X: ", e_Status);
    Serial.println(s8_Buf);
  */
    switch (e_Status)
    {
        case ST_Success:    // Success
        case ST_NoChanges:  // No changes made
        case ST_MoreFrames: // Another frame will follow
            return true;

        default: break; // This is just to avoid stupid gcc compiler warnings
    }

    switch (e_Status)
    {
            return false;
    }
}


