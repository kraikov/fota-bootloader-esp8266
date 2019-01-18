//----------------------------------------------------------------------------------------------------------------------
// Included files to resolve specific definitions in this file
//----------------------------------------------------------------------------------------------------------------------
#include "LoaderConfiguration.h"
#include <Loader.h>


//----------------------------------------------------------------------------------------------------------------------
// Local macros
//----------------------------------------------------------------------------------------------------------------------
#define WriteLine ets_printf

//----------------------------------------------------------------------------------------------------------------------
// Local function prototypes
//----------------------------------------------------------------------------------------------------------------------
static void SetDefaultConfig(BootConfiguration * const BootCfg, uint32 flashSize);
static uint32 GetImageAddress(uint32 readPosition);

//======================================================================================================================
// LOCAL FUNCTIONS
//======================================================================================================================

//======================================================================================================================
// DESCRIPTION:         Populate the user fields of the default config. Created on first boot or in case of corruption
//
// PARAMETERS:          BootConfiguration * const BootCfg - a reference to the specific Boot configuration instance
//                      uint32 flashSize - current flash size
//
// RETURN VALUE:        void
//
//======================================================================================================================
static void SetDefaultConfig(BootConfiguration * const BootCfg, uint32 flashSize)
{
    BootCfg->Count = 2;
    BootCfg->ROMS[0] = SECTOR_SIZE * (BOOT_CONFIG_SECTOR + 1);
    BootCfg->ROMS[1] = (flashSize / 2) + (SECTOR_SIZE * (BOOT_CONFIG_SECTOR + 1));
}

//======================================================================================================================
// DESCRIPTION:
//
// PARAMETERS:
//
// RETURN VALUE:
//
//======================================================================================================================
static uint32 GetImageAddress(uint32 readPosition)
{
    uint8 data[BUFFER_SIZE];
    uint8 sectorCount;
    uint8 currentSector;
    uint8 checkSum = DEFAULT_CHECKSUM;
    uint32 loopCounter;
    uint32 remaining;
    uint32 romAddress;

    ExtraROMHeader *header = (ExtraROMHeader*) data;
    SectionHeader *section = (SectionHeader*) data;

    if ((0 == readPosition) || (0xFFFFFFFF == readPosition))
    {
        return 0;
    }

    // Read ROM Header
    if (SPIRead(readPosition, header, sizeof(ExtraROMHeader)) != 0)
    {
        return 0;
    }

    if ((ROM_MAGIC == header->MagicNumber) && (ROM_MAGIC_COUNT == header->Count))
    {
        romAddress = readPosition + header->Length + sizeof(ExtraROMHeader);
        readPosition = romAddress;
        if (SPIRead(readPosition, header, sizeof(ROMHeader)) != 0)
        {
            return 0;
        }
        sectorCount = header->Count;
        readPosition += sizeof(ROMHeader);
    }
    else
    {
        return 0;
    }

    // Test each section
    for (currentSector = 0; currentSector < sectorCount; currentSector++)
    {
        // Read section header
        if (SPIRead(readPosition, section, sizeof(SectionHeader)) != 0)
        {
            return 0;
        }
        readPosition += sizeof(SectionHeader);

        // Get section Address and Length
        remaining = section->Length;

        while (remaining > 0)
        {
            // Work out how much to read, up to BUFFER_SIZE
            uint32 readLength = (remaining < BUFFER_SIZE) ? remaining : BUFFER_SIZE;
            // Read the block
            if (0 != SPIRead(readPosition, data, readLength))
            {
                return 0;
            }
            // Increment next read position
            readPosition += readLength;
            // Decrement remaining count
            remaining -= readLength;
            // Add to checkSum
            for (loopCounter = 0; loopCounter < readLength; loopCounter++)
            {
                checkSum ^= data[loopCounter];
            }
        }
    }

    // Round up to next 16 and get checksum
    readPosition = readPosition | 0x0F;
    if (0 != SPIRead(readPosition, data, 1))
    {
        return 0;
    }

    // Compare calculated and stored checksums
    if (checkSum != data[0])
    {
        return 0;
    }

    return romAddress;
}

//======================================================================================================================
// EXPORTED FUNCTIONS
//======================================================================================================================

//======================================================================================================================
// DESCRIPTION:         Prevent this function being placed inline with main to keep main's stack size as small as possible
//                      Don't mark as static or it'll be optimised out when using the assembler stub
//
// PARAMETERS:          void
//
// RETURN VALUE:
//
//======================================================================================================================
uint32 __attribute__ ((noinline)) FindImageAddress(void)
{
    uint8 updateConfigFlag = FALSE;
    uint8 config;
    uint8 buffer[SECTOR_SIZE];
    uint32 runAddress;
    uint32 flashSize;
    int32 romToBoot;

    BootConfiguration *romconf = (BootConfiguration*) buffer;
    ROMHeader *header = (ROMHeader*) buffer;

    // read rom header
    SPIRead(0, header, sizeof(ROMHeader));

    // print and get flash size
    WriteLine("Device flash size:   ");
    config = header->FlashSize >> 4;
    switch (config)
    {
        case 0:
        {
            WriteLine("4 MBit\r\n");
            flashSize = 0x80000;
            break;
        }
        case 1:
        {
            WriteLine("2 MBit\r\n");
            flashSize = 0x40000;
            break;
        }
        case 2:
        {
            WriteLine("8 MBit\r\n");
            flashSize = 0x100000;
            break;
        }
        case 3:
        {
            WriteLine("16 MBit\r\n");
            flashSize = 0x100000;
            break;
        }
        case 4:
        {
            WriteLine("32 MBit\r\n");
            flashSize = 0x100000;
            break;
        }
        default:
        {
            WriteLine("Unknown flash memory size\r\n");
            flashSize = 0x80000;
            break;
        }
    }

    // print spi speed
    WriteLine("Device flash speed:  ");
    config = header->FlashSize & 0x0F;
    switch (config)
    {
        case 0:
        {
            WriteLine("40 MHz\r\n");
            break;
        }
        case 1:
        {
            WriteLine("26.7 MHz\r\n");
            break;
        }
        case 2:
        {
            WriteLine("20 MHz\r\n");
            break;
        }
        case 0x0F:
        {
            WriteLine("80 MHz\r\n");
            break;
        }
        default:
        {
            WriteLine("unknown\r\n");
            break;
        }
    }
    WriteLine("\r\n");

    // read boot config
    SPIRead(BOOT_CONFIG_SECTOR * SECTOR_SIZE, buffer, SECTOR_SIZE);
    // fresh install or old version?
    if (romconf->MagicNumber != BOOT_CONFIG_MAGIC || romconf->Version != BOOT_CONFIG_VERSION)
    {
        // create a default config for a standard 2 rom setup
        WriteLine("Writing default bootloader configuration.\r\n");
        ets_memset(romconf, 0x00, sizeof(BootConfiguration));
        romconf->MagicNumber = BOOT_CONFIG_MAGIC;
        romconf->Version = BOOT_CONFIG_VERSION;
        SetDefaultConfig(romconf, flashSize);
        // write new config sector
        SPIEraseSector(BOOT_CONFIG_SECTOR);
        SPIWrite(BOOT_CONFIG_SECTOR * SECTOR_SIZE, buffer, SECTOR_SIZE);
    }

    // Try rom selected in the config
    romToBoot = romconf->CurrentROM;

    // Check valid rom number
    if (romconf->CurrentROM >= romconf->Count)
    {
        // If invalid rom selected try rom 0
        WriteLine("Invalid rom selected. Set to default ROM 0.\r\n");
        romToBoot = 0;
        romconf->CurrentROM = 0;
        updateConfigFlag = TRUE;
    }

    // Check rom is valid
    runAddress = GetImageAddress(romconf->ROMS[romToBoot]);

    // Check we have a good rom
    while (runAddress == 0)
    {
        WriteLine("Rom %d is invalid.\r\n", romToBoot);
        // For normal mode try each previous rom
        // Until we find a good one or run out
        updateConfigFlag = TRUE;
        romToBoot--;
        if (romToBoot < 0)
        {
            romToBoot = romconf->Count - 1;
        }

        if (romToBoot == romconf->CurrentROM)
        {
            // Tried them all and all are bad!
            WriteLine("No good rom available.\r\n");
            return 0;
        }
        runAddress = GetImageAddress(romconf->ROMS[romToBoot]);
    }

    // Re-write config, if required
    if (updateConfigFlag)
    {
        romconf->CurrentROM = romToBoot;
        SPIEraseSector(BOOT_CONFIG_SECTOR);
        SPIWrite(BOOT_CONFIG_SECTOR * SECTOR_SIZE, buffer, SECTOR_SIZE);
    }

    WriteLine("Trying to boot ROM %d.\r\n", romToBoot);

    // Copy the loader to top of iram
    ets_memcpy((void*) _text_addr, _text_data, _text_len);

    // Return address to load from
    return runAddress;
}

//======================================================================================================================
// DESCRIPTION:
//
// PARAMETERS:          void
//
// RETURN VALUE:        void
//
//======================================================================================================================
void call_user_start(void)
{
    uint32 address;
    Loader *loader;

    address = FindImageAddress();
    if (address != 0)
    {
        loader = (Loader*) entry_addr;
        loader(address);
    }
}
