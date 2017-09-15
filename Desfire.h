
#ifndef DESFIRE_H
#define DESFIRE_H

#include "PN532.h"
#include "Buffer.h"
//#include "DesFireKey.h"

// Just an invalid key number
#define NOT_AUTHENTICATED      255

#define MAX_FRAME_SIZE         60 // The maximum total length of a packet that is transfered to / from the card

// ------- Desfire legacy instructions --------

#define DF_INS_AUTHENTICATE_LEGACY        0x0A
#define DF_INS_CHANGE_KEY_SETTINGS        0x54
#define DF_INS_GET_KEY_SETTINGS           0x45
#define DF_INS_CHANGE_KEY                 0xC4
#define DF_INS_GET_KEY_VERSION            0x64

#define DF_INS_CREATE_APPLICATION         0xCA
#define DF_INS_DELETE_APPLICATION         0xDA
#define DF_INS_GET_APPLICATION_IDS        0x6A
#define DF_INS_SELECT_APPLICATION         0x5A

#define DF_INS_FORMAT_PICC                0xFC
#define DF_INS_GET_VERSION                0x60

#define DF_INS_GET_FILE_IDS               0x6F
#define DF_INS_GET_FILE_SETTINGS          0xF5
#define DF_INS_CHANGE_FILE_SETTINGS       0x5F
#define DF_INS_CREATE_STD_DATA_FILE       0xCD
#define DF_INS_CREATE_BACKUP_DATA_FILE    0xCB
#define DF_INS_CREATE_VALUE_FILE          0xCC
#define DF_INS_CREATE_LINEAR_RECORD_FILE  0xC1
#define DF_INS_CREATE_CYCLIC_RECORD_FILE  0xC0
#define DF_INS_DELETE_FILE                0xDF

#define DF_INS_READ_DATA                  0xBD
#define DF_INS_WRITE_DATA                 0x3D
#define DF_INS_GET_VALUE                  0x6C
#define DF_INS_CREDIT                     0x0C
#define DF_INS_DEBIT                      0xDC
#define DF_INS_LIMITED_CREDIT             0x1C
#define DF_INS_WRITE_RECORD               0x3B
#define DF_INS_READ_RECORDS               0xBB
#define DF_INS_CLEAR_RECORD_FILE          0xEB
#define DF_COMMIT_TRANSACTION             0xC7
#define DF_INS_ABORT_TRANSACTION          0xA7

#define DF_INS_ADDITIONAL_FRAME           0xAF // data did not fit into a frame, another frame will follow

// -------- Desfire EV1 instructions ----------

#define DFEV1_INS_AUTHENTICATE_ISO        0x1A
#define DFEV1_INS_AUTHENTICATE_AES        0xAA
#define DFEV1_INS_FREE_MEM                0x6E
#define DFEV1_INS_GET_DF_NAMES            0x6D
#define DFEV1_INS_GET_CARD_UID            0x51
#define DFEV1_INS_GET_ISO_FILE_IDS        0x61
#define DFEV1_INS_SET_CONFIGURATION       0x5C

// ---------- ISO7816 instructions ------------

#define ISO7816_INS_EXTERNAL_AUTHENTICATE 0x82
#define ISO7816_INS_INTERNAL_AUTHENTICATE 0x88
#define ISO7816_INS_APPEND_RECORD         0xE2
#define ISO7816_INS_GET_CHALLENGE         0x84
#define ISO7816_INS_READ_RECORDS          0xB2
#define ISO7816_INS_SELECT_FILE           0xA4
#define ISO7816_INS_READ_BINARY           0xB0
#define ISO7816_INS_UPDATE_BINARY         0xD6


// Status codes (errors) returned from Desfire card
enum DESFireStatus
{
    ST_Success               = 0x00,
    ST_NoChanges             = 0x0C,
    ST_OutOfMemory           = 0x0E,
    ST_IllegalCommand        = 0x1C,
    ST_IntegrityError        = 0x1E,
    ST_KeyDoesNotExist       = 0x40,
    ST_WrongCommandLen       = 0x7E,
    ST_PermissionDenied      = 0x9D,
    ST_IncorrectParam        = 0x9E,
    ST_AppNotFound           = 0xA0,
    ST_AppIntegrityError     = 0xA1,
    ST_AuthentError          = 0xAE,
    ST_MoreFrames            = 0xAF, // data did not fit into a frame, another frame will follow
    ST_LimitExceeded         = 0xBE,
    ST_CardIntegrityError    = 0xC1,
    ST_CommandAborted        = 0xCA,
    ST_CardDisabled          = 0xCD,
    ST_InvalidApp            = 0xCE,
    ST_DuplicateAidFiles     = 0xDE,
    ST_EepromError           = 0xEE,
    ST_FileNotFound          = 0xF0,
    ST_FileIntegrityError    = 0xF1,
};

// Card information about software and hardware version.
struct DESFireCardVersion
{
    byte hardwareVendorId;    // The hardware vendor
    byte hardwareType;        // The hardware type
    byte hardwareSubType;     // The hardware subtype
    byte hardwareMajVersion;  // The hardware major version
    byte hardwareMinVersion;  // The hardware minor version
    byte hardwareStorageSize; // The hardware storage size
    byte hardwareProtocol;    // The hardware protocol

    byte softwareVendorId;    // The software vendor
    byte softwareType;        // The software type
    byte softwareSubType;     // The software subtype
    byte softwareMajVersion;  // The software major version
    byte softwareMinVersion;  // The software minor version
    byte softwareStorageSize; // The software storage size
    byte softwareProtocol;    // The software protocol

    byte uid[7];              // The serial card number
    byte batchNo[5];          // The batch number
    byte cwProd;              // The production week (BCD)
    byte yearProd;            // The production year (BCD)
};

// MK = Application Master Key or PICC Master Key
enum DESFireKeySettings
{
    // ------------ BITS 0-3 ---------------
    KS_ALLOW_CHANGE_MK                = 0x01, // If this bit is set, the MK can be changed, otherwise it is frozen.
    KS_LISTING_WITHOUT_MK             = 0x02, // Picc key: If this bit is set, GetApplicationIDs, GetKeySettings do not require MK authentication.
                                              // App  key: If this bit is set, GetFileIDs, GetFileSettings, GetKeySettings do not require MK authentication.
    KS_CREATE_DELETE_WITHOUT_MK       = 0x04, // Picc key: If this bit is set, CreateApplication does not require MK authentication.
                                              // App  key: If this bit is set, CreateFile, DeleteFile do not require MK authentication.
    KS_CONFIGURATION_CHANGEABLE       = 0x08, // If this bit is set, the configuration settings of the MK can be changed, otherwise they are frozen.
    
    // ------------ BITS 4-7 (not used for the PICC master key) -------------
    KS_CHANGE_KEY_WITH_MK             = 0x00, // A key change requires MK authentication
    KS_CHANGE_KEY_WITH_KEY_1          = 0x10, // A key change requires authentication with key 1
    KS_CHANGE_KEY_WITH_KEY_2          = 0x20, // A key change requires authentication with key 2
    KS_CHANGE_KEY_WITH_KEY_3          = 0x30, // A key change requires authentication with key 3
    KS_CHANGE_KEY_WITH_KEY_4          = 0x40, // A key change requires authentication with key 4 
    KS_CHANGE_KEY_WITH_KEY_5          = 0x50, // A key change requires authentication with key 5
    KS_CHANGE_KEY_WITH_KEY_6          = 0x60, // A key change requires authentication with key 6
    KS_CHANGE_KEY_WITH_KEY_7          = 0x70, // A key change requires authentication with key 7
    KS_CHANGE_KEY_WITH_KEY_8          = 0x80, // A key change requires authentication with key 8
    KS_CHANGE_KEY_WITH_KEY_9          = 0x90, // A key change requires authentication with key 9
    KS_CHANGE_KEY_WITH_KEY_A          = 0xA0, // A key change requires authentication with key 10
    KS_CHANGE_KEY_WITH_KEY_B          = 0xB0, // A key change requires authentication with key 11
    KS_CHANGE_KEY_WITH_KEY_C          = 0xC0, // A key change requires authentication with key 12
    KS_CHANGE_KEY_WITH_KEY_D          = 0xD0, // A key change requires authentication with key 13
    KS_CHANGE_KEY_WITH_TARGETED_KEY   = 0xE0, // A key change requires authentication with the same key that is to be changed
    KS_CHANGE_KEY_FROZEN              = 0xF0, // All keys are frozen
    
    // -------------------------------------
    KS_FACTORY_DEFAULT                = 0x0F,
};

enum DESFireAccessRights
{
    AR_KEY0  = 0x00, // Authentication with application key 0 required (master key)
    AR_KEY1  = 0x01, // Authentication with application key 1 required
    AR_KEY2  = 0x02, // ...
    AR_KEY3  = 0x03,
    AR_KEY4  = 0x04,
    AR_KEY5  = 0x05,
    AR_KEY6  = 0x06,
    AR_KEY7  = 0x07,
    AR_KEY8  = 0x08,
    AR_KEY9  = 0x09,
    AR_KEY10 = 0x0A,
    AR_KEY11 = 0x0B,
    AR_KEY12 = 0x0C,
    AR_KEY13 = 0x0D,
    AR_FREE  = 0x0E, // Always allowed even without authentication
    AR_NEVER = 0x0F  // Always forbidden even with authentication
};

struct DESFireFilePermissions
{
    DESFireAccessRights  e_ReadAccess;         
    DESFireAccessRights  e_WriteAccess;        
    DESFireAccessRights  e_ReadAndWriteAccess; 
    DESFireAccessRights  e_ChangeAccess;       

    uint16_t Pack()
    {
        return (e_ReadAccess << 12) | (e_WriteAccess <<  8) | (e_ReadAndWriteAccess <<  4) | e_ChangeAccess;
    }
    void Unpack(uint16_t u16_Data)
    {
        e_ReadAccess         = (DESFireAccessRights)((u16_Data >> 12) & 0x0F);
        e_WriteAccess        = (DESFireAccessRights)((u16_Data >>  8) & 0x0F);
        e_ReadAndWriteAccess = (DESFireAccessRights)((u16_Data >>  4) & 0x0F);
        e_ChangeAccess       = (DESFireAccessRights)((u16_Data      ) & 0x0F);
    }
};

// Defines if data transmitted to files is encrypted (with the session key) or secured with a MAC
enum DESFireFileEncryption
{
    CM_PLAIN   = 0x00,
    CM_MAC     = 0x01,   // not implemented (Plain data transfer with additional MAC)
    CM_ENCRYPT = 0x03,   // not implemented (Does not make data stored on the card more secure. Only encrypts the transfer between Teensy and the card)
};

enum DESFireFileType
{
    MDFT_STANDARD_DATA_FILE             = 0x00,
    MDFT_BACKUP_DATA_FILE               = 0x01, // not implemented
    MDFT_VALUE_FILE_WITH_BACKUP         = 0x02, // not implemented
    MDFT_LINEAR_RECORD_FILE_WITH_BACKUP = 0x03, // not implemented
    MDFT_CYCLIC_RECORD_FILE_WITH_BACKUP = 0x04, // not implemented
};

struct DESFireFileSettings
{
    DESFireFileType        e_FileType;
    DESFireFileEncryption  e_Encrypt;
    DESFireFilePermissions k_Permis;
    // -----------------------------
    // used only for MDFT_STANDARD_DATA_FILE and MDFT_BACKUP_DATA_FILE
    uint32_t u32_FileSize;
    // -----------------------------
    // used only for MDFT_VALUE_FILE_WITH_BACKUP
    uint32_t  u32_LowerLimit;
    uint32_t  u32_UpperLimit;
    uint32_t  u32_LimitedCreditValue;
    bool      b_LimitedCreditEnabled;
    // -----------------------------
    // used only for MDFT_LINEAR_RECORD_FILE_WITH_BACKUP and MDFT_CYCLIC_RECORD_FILE_WITH_BACKUP
    uint32_t  u32_RecordSize;
    uint32_t  u32_MaxNumberRecords;
    uint32_t  u32_CurrentNumberRecords;
};

enum DESFireCmac
{
    MAC_None   = 0,
    // Transmit data:
    MAC_Tmac   = 1, // The CMAC must be calculated for the TX data sent to the card although this Tx CMAC is not transmitted
    MAC_Tcrypt = 2, // To the parameters sent to the card a CRC32 must be appended and then they must be encrypted with the session key    
    // Receive data:
    MAC_Rmac   = 4, // The CMAC must be calculated for the RX data received from the card. If status == ST_Success -> verify the CMAC in the response
    MAC_Rcrypt = 8, // The data received from the card must be decrypted with the session key
    // Combined:
    MAC_TmacRmac   = MAC_Tmac   | MAC_Rmac,
    MAC_TmacRcrypt = MAC_Tmac   | MAC_Rcrypt,
    MAC_TcryptRmac = MAC_Tcrypt | MAC_Rmac,
};

class Desfire : public PN532
{
 public:
    Desfire();
    bool GetCardVersion(DESFireCardVersion* pk_Version);
    bool FormatCard();
    bool EnableRandomIDForever();
    bool GetRealCardID(byte u8_UID[7]);
    bool GetFreeMemory(uint32_t* pu32_Memory);
    
    // ---------------------
    bool SwitchOffRfField();  // overrides PN532::SwitchOffRfField()
    bool Selftest();
    byte GetLastPN532Error(); // See comment for this function in CPP file

    int  DataExchange(byte      u8_Command, TxBuffer* pi_Params, byte* u8_RecvBuf, int s32_RecvSize, DESFireStatus* pe_Status, DESFireCmac e_Mac);
    int  DataExchange(TxBuffer* pi_Command, TxBuffer* pi_Params, byte* u8_RecvBuf, int s32_RecvSize, DESFireStatus* pe_Status, DESFireCmac e_Mac);  
    
 private:
 
    bool CheckCardStatus(DESFireStatus e_Status);

    byte          mu8_LastAuthKeyNo; // The last key which did a successful authetication (0xFF if not yet authenticated)
    uint32_t      mu32_LastApplication;
    //DESFireKey*   mpi_SessionKey;
    byte          mu8_LastPN532Error;

    // Must have enough space to hold the entire response from DF_INS_GET_APPLICATION_IDS (84 byte) + CMAC padding
    byte          mu8_CmacBuffer_Data[120]; 
    TxBuffer      mi_CmacBuffer;
};

#endif
