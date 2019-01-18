#ifndef __LOADER_CONFIGURATION__
#define __LOADER_CONFIGURATION__

//----------------------------------------------------------------------------------------------------------------------
// Included files to resolve specific definitions in this file
//----------------------------------------------------------------------------------------------------------------------

#include "BootloaderDriver.h"

//----------------------------------------------------------------------------------------------------------------------
// Constant data
//----------------------------------------------------------------------------------------------------------------------
#define ROM_MAGIC           0xEA

#define ROM_MAGIC_COUNT     0x04

// buffer size, must be at least 0x10 (size of ExtraROMHeader structure)
#define BUFFER_SIZE 0x0100

// read maximum size (limit for SPIRead)
#define READ_SIZE 0x1000

//----------------------------------------------------------------------------------------------------------------------
// Exported functions
//----------------------------------------------------------------------------------------------------------------------

extern uint32 SPIRead(uint32 addr, void *outptr, uint32 len);

extern uint32 SPIEraseSector(int);

extern uint32 SPIWrite(uint32 addr, void *inptr, uint32 len);

extern void ets_printf(char*, ...);

extern void ets_delay_us(int);

extern void ets_memset(void*, uint8, uint32);

extern void ets_memcpy(void*, const void*, uint32);

//----------------------------------------------------------------------------------------------------------------------
// Exported type
//----------------------------------------------------------------------------------------------------------------------

// Function that we'll call by address
typedef void Loader(uint32);

typedef void UserCode(void);

typedef struct
{
    // general rom header
    uint8 MagicNumber;
    uint8 Count;
    uint8 FlashMode;
    uint8 FlashSize;
    UserCode* Entry;
} ROMHeader;

typedef struct
{
    uint8* Address;
    uint32 Length;
} SectionHeader;

typedef struct
{
    // general rom header
    uint8 MagicNumber;
    uint8 Count;
    uint8 FlashMode;
    uint8 FlashSize;
    uint32 Entry;
    uint32 Address;
    uint32 Length;
} ExtraROMHeader;

#endif
