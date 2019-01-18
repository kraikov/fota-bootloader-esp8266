//----------------------------------------------------------------------------------------------------------------------
// Included files to resolve specific definitions in this file
//----------------------------------------------------------------------------------------------------------------------
#include "LoaderConfiguration.h"

//======================================================================================================================
// EXPORTED FUNCTIONS
//======================================================================================================================

//======================================================================================================================
// DESCRIPTION:
//
//
// PARAMETERS:
//
// RETURN VALUE:
//
//======================================================================================================================
UserCode* __attribute__ ((noinline)) LoadROMImage(uint32 readpos)
{
    uint8 numberOfSectors;
    uint8* writePosition;
    uint32 remaining;
    UserCode* userCode;

    ROMHeader header;
    SectionHeader section;

    // read rom header
    SPIRead(readpos, &header, sizeof(ROMHeader));
    readpos += sizeof(ROMHeader);

    // create function pointer for entry point
    userCode = header.Entry;

    // copy all the sections
    for (numberOfSectors = header.Count; numberOfSectors > 0; numberOfSectors--)
    {
        // read section header
        SPIRead(readpos, &section, sizeof(SectionHeader));
        readpos += sizeof(SectionHeader);

        // get section address and length
        writePosition = section.Address;
        remaining = section.Length;

        while (remaining > 0)
        {
            // work out how much to read, up to 16 bytes at a time
            uint32 readlen = (remaining < READ_SIZE) ? remaining : READ_SIZE;
            // read the block
            SPIRead(readpos, writePosition, readlen);
            readpos += readlen;
            // increment next write position
            writePosition += readlen;
            // decrement remaining count
            remaining -= readlen;
        }
    }
    return userCode;
}

//======================================================================================================================
// DESCRIPTION:
//
//
// PARAMETERS:
//
// RETURN VALUE:
//
//======================================================================================================================
void call_user_start(uint32 readpos)
{
    UserCode* user;

    user = LoadROMImage(readpos);

    user();
}
