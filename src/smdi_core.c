/*
 * SMDI core implementation for IRIX 5.3
 * ANSI C90 compliant for MIPS big-endian architecture
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "smdi.h"
#include "aspi_irix.h"
#include "scsi_debug.h"

/* Standard data packet size for SMDI transfers */
#define PACKETSIZE 16384
#define MAXFNLEN   1024  /* Maximum file name length for IRIX */

/* Sample loop control flags */
#define LOOP_NONE         0
#define LOOP_FORWARD      1
#define LOOP_BIDIRECTIONAL 2

/* Sleep function for IRIX */
static void sleep_ms(int ms) {
    /* Convert ms to clock ticks (10ms each), rounding up */
    sginap((ms + 9) / 10);
}

/* Structure for native sample format header */
typedef struct {
    BYTE  signature[4];       /* 'SDMP' */
    DWORD version;            /* Version number (1) */
    BYTE  bitsPerSample;      /* Typically 16 */
    BYTE  channels;           /* 1=mono, 2=stereo */
    BYTE  loopType;           /* 0=none, 1=forward, 2=bidirectional */
    BYTE  reserved;
    DWORD sampleRate;         /* In Hz */
    DWORD sampleCount;        /* Per channel */
    DWORD loopStart;          /* Sample point */
    DWORD loopEnd;            /* Sample point */
    WORD  pitch;              /* MIDI note */
    WORD  pitchFraction;      /* Cents, -50 to +50 */
    DWORD nameLength;         /* Length of name string */
    /* Name string follows... */
    /* Then sample data... */
} NativeSampleHeader;

/*
 * Sample transmission functions
 */

/* Initialize a sample transmission */
DWORD SMDI_InitSampleTransmission(SMDI_TransmissionInfo* lpTransmissionInfo) {
    BOOL unitready;
    SMDI_TransmissionInfo transmissionInfo;
    DWORD messRet;
    
    /* Make a local copy */
    memcpy(&transmissionInfo, lpTransmissionInfo, sizeof(SMDI_TransmissionInfo));
    
    /* Initialize */
    transmissionInfo.dwTransmittedPackets = 0;
    transmissionInfo.dwPacketSize = PACKETSIZE;
    
    /* Send the sample header */
    messRet = SMDI_SendSampleHeader(
        transmissionInfo.HA_ID,
        transmissionInfo.SCSI_ID,
        transmissionInfo.dwSampleNumber,
        transmissionInfo.lpSampleHeader,
        &transmissionInfo.dwPacketSize);
    
    if (messRet == SMDIM_TRANSFERACKNOWLEDGE) {
        /* Send begin sample transfer */
        messRet = SMDI_SendBeginSampleTransfer(
            transmissionInfo.HA_ID,
            transmissionInfo.SCSI_ID,
            transmissionInfo.dwSampleNumber,
            &transmissionInfo.dwPacketSize);
        
        /* Handle WAIT response */
        if (messRet == SMDIM_WAIT) {
            unitready = FALSE;
            while (unitready == FALSE) {
                /* Delay 100 ms */
                sleep_ms(100);
                printf("Waiting for unit to become ready\n");
                
                /* Check if unit ready */
                unitready = ASPI_TestUnitReady(
                    NULL,
                    transmissionInfo.HA_ID,
                    transmissionInfo.SCSI_ID);
            }
            /* Get the next message */
            messRet = SMDI_GetMessage(
                transmissionInfo.HA_ID,
                transmissionInfo.SCSI_ID);
        }
    }
    
    /* Check for message reject */
    if (messRet == SMDIM_MESSAGEREJECT) {
        return SMDI_GetLastError();
    }
    
    /* Copy back the updated info */
    memcpy(lpTransmissionInfo, &transmissionInfo, sizeof(SMDI_TransmissionInfo));
    
    return messRet;
}

/* Perform a sample transmission */
DWORD SMDI_SampleTransmission(SMDI_TransmissionInfo* lpTransmissionInfo)
{
    SMDI_SampleHeader sampleHeader;
    SMDI_TransmissionInfo transmissionInfo;
    BOOL ur;
    DWORD messRet;
    DWORD transmittedBytes;
    DWORD samLength;
    
    /* Make local copies */
    memcpy(&transmissionInfo, lpTransmissionInfo, sizeof(SMDI_TransmissionInfo));
    memcpy(&sampleHeader, transmissionInfo.lpSampleHeader, sizeof(SMDI_SampleHeader));
    
    /* Calculate bytes already transmitted */
    transmittedBytes = (transmissionInfo.dwPacketSize * transmissionInfo.dwTransmittedPackets);
    
    /* Calculate total sample length in bytes */
    samLength = (sampleHeader.dwLength *
               (DWORD)sampleHeader.NumberOfChannels *
               (DWORD)sampleHeader.BitsPerWord) / 8;
    
    /* If we're sending the last packet, adjust the size */
    if ((transmittedBytes + transmissionInfo.dwPacketSize) > samLength) {
        transmissionInfo.dwPacketSize = samLength - transmittedBytes;
    }
    
    /* Send the data packet */
    messRet = SMDI_SendDataPacket(
        transmissionInfo.HA_ID,
        transmissionInfo.SCSI_ID,
        transmissionInfo.dwTransmittedPackets,
        (void*)((char*)transmissionInfo.lpSampleData),
        transmissionInfo.dwPacketSize);
    
    /* Handle WAIT response */
    if (messRet == SMDIM_WAIT) {
        ur = FALSE;
        while (ur == FALSE) {
            /* Delay 100 ticks (about 10ms) */
            sleep_ms(10);
            
            /* Check if unit ready */
            ur = ASPI_TestUnitReady(
                NULL,
                transmissionInfo.HA_ID,
                transmissionInfo.SCSI_ID);
        }
        /* Get the next message */
        messRet = SMDI_GetMessage(
            transmissionInfo.HA_ID,
            transmissionInfo.SCSI_ID);
    }
    
    /* Increment packet counter */
    transmissionInfo.dwTransmittedPackets++;
    
    /* Copy back the updated info */
    memcpy(lpTransmissionInfo, &transmissionInfo, sizeof(SMDI_TransmissionInfo));
    
    return messRet;
}

/* Initialize a sample reception */
DWORD SMDI_InitSampleReception(SMDI_TransmissionInfo* tiATemp) {
    SMDI_TransmissionInfo tiTemp;
    DWORD messRet;
    
    /* Make sure we have a valid pointer */
    if (tiATemp == NULL) {
        printf("tiATemp is NULL!!\n");
        return SMDIM_ERROR;
    }
    
    /* Make a local copy */
    memcpy(&tiTemp, tiATemp, sizeof(SMDI_TransmissionInfo));
    
    /* Initialize */
    tiTemp.dwTransmittedPackets = 0;
    
    /* Request the sample header */
    messRet = SMDI_SampleHeaderRequest(
        tiTemp.HA_ID,
        tiTemp.SCSI_ID,
        tiTemp.dwSampleNumber,
        tiTemp.lpSampleHeader);
    
    if (messRet == SMDIM_SAMPLEHEADER) {
        /* Set default packet size */
        tiTemp.dwPacketSize = PACKETSIZE;
        
        /* Begin the sample transfer */
        messRet = SMDI_SendBeginSampleTransfer(
            tiTemp.HA_ID,
            tiTemp.SCSI_ID,
            tiTemp.dwSampleNumber,
            &tiTemp.dwPacketSize);
    }
    
    /* Check for message reject */
    if (messRet == SMDIM_MESSAGEREJECT) {
        return SMDI_GetLastError();
    }
    
    /* Copy back the updated info */
    memcpy(tiATemp, &tiTemp, sizeof(SMDI_TransmissionInfo));
    
    return messRet;
}

/* Receive a sample packet */
DWORD SMDI_SampleReception(SMDI_TransmissionInfo* lpTransmissionInfo) {
    SMDI_TransmissionInfo transmissionInfo;
    SMDI_SampleHeader sampleHeader;
    DWORD messRet;
    DWORD transmittedBytes;
    DWORD samLength;
    void* dataPtr;
    
    /* Make local copies */
    memcpy(&transmissionInfo, lpTransmissionInfo, sizeof(SMDI_TransmissionInfo));
    memcpy(&sampleHeader, transmissionInfo.lpSampleHeader, sizeof(SMDI_SampleHeader));
    
    /* Calculate bytes already transmitted */
    transmittedBytes = (transmissionInfo.dwPacketSize * transmissionInfo.dwTransmittedPackets);
    
    /* Calculate total sample length in bytes */
    samLength = (sampleHeader.dwLength *
               (DWORD)sampleHeader.NumberOfChannels *
               (DWORD)sampleHeader.BitsPerWord) / 8;
    
    /* Calculate data pointer for this packet */
    dataPtr = (void*)((char*)transmissionInfo.lpSampleData + transmittedBytes);
    
    /* Request the next data packet */
    messRet = SMDI_NextDataPacketRequest(
        transmissionInfo.HA_ID,
        transmissionInfo.SCSI_ID,
        transmissionInfo.dwTransmittedPackets,
        dataPtr,
        transmissionInfo.dwPacketSize);
    
    /* If we've transferred enough data, return END OF PROCEDURE */
    if ((transmittedBytes + transmissionInfo.dwPacketSize) >= samLength) {
        messRet = SMDIM_ENDOFPROCEDURE;
    }
    
    /* Increment packet counter */
    transmissionInfo.dwTransmittedPackets++;
    
    /* Copy back the updated info */
    memcpy(lpTransmissionInfo, &transmissionInfo, sizeof(SMDI_TransmissionInfo));
    
    return messRet;
}

/*
 * File-based sample operations
 */

/* Get sample header from a file */
DWORD SMDI_GetFileSampleHeader(char cFileName[], SMDI_SampleHeader* lpSampleHeader) {
    FILE* hFile;
    NativeSampleHeader nativeHdr;
    SMDI_SampleHeader shMyTemp;
    char buffer[8];
    
    /* Make sure we have valid pointers */
    if (cFileName == NULL || lpSampleHeader == NULL) {
        return FE_OPENERROR;
    }
    
    /* Initialize header */
    memcpy(&shMyTemp, lpSampleHeader, sizeof(SMDI_SampleHeader));
    shMyTemp.LoopControl = 0;  /* No loop by default */
    shMyTemp.wPitch = 60;      /* Middle C */
    shMyTemp.wPitchFraction = 0;
    shMyTemp.dwLoopStart = 0;
    shMyTemp.dwLoopEnd = 0;
    shMyTemp.dwLength = 0;     /* Make sure this starts at 0 */
    
    /* Open the file */
    hFile = fopen(cFileName, "rb");
    if (hFile == NULL) {
        return FE_OPENERROR;
    }
    
    /* Read file signature */
    fread(buffer, 1, 4, hFile);
    
    /* Check for native sample format */
    if (memcmp(buffer, "SDMP", 4) == 0) {
        /* Read the header from the beginning */
        fseek(hFile, 0, SEEK_SET);
        
        /* Read native sample header */
        if (fread(&nativeHdr, sizeof(NativeSampleHeader), 1, hFile) != 1) {
            fclose(hFile);
            return FE_UNKNOWNFORMAT;
        }
        
        /* Fill sample header from native format */
        shMyTemp.bDoesExist = TRUE;
        shMyTemp.BitsPerWord = nativeHdr.bitsPerSample;
        shMyTemp.NumberOfChannels = nativeHdr.channels;
        shMyTemp.LoopControl = nativeHdr.loopType;
        shMyTemp.dwPeriod = 1000000000 / nativeHdr.sampleRate;
        shMyTemp.dwLength = nativeHdr.sampleCount;
        shMyTemp.dwLoopStart = nativeHdr.loopStart;
        shMyTemp.dwLoopEnd = nativeHdr.loopEnd;
        shMyTemp.wPitch = nativeHdr.pitch;
        shMyTemp.wPitchFraction = nativeHdr.pitchFraction;
        
        /* Read name */
        shMyTemp.NameLength = (BYTE)(nativeHdr.nameLength > 255 ? 255 : nativeHdr.nameLength);
        memset(shMyTemp.cName, 0, 256);
        
        if (shMyTemp.NameLength > 0) {
            fread(shMyTemp.cName, 1, shMyTemp.NameLength, hFile);
        }
        
        /* Data offset is right after the header and name */
        shMyTemp.dwDataOffset = sizeof(NativeSampleHeader) + nativeHdr.nameLength;
        
        fclose(hFile);
        
        /* If loop end is 0, set it to the sample length - 1 */
        if (shMyTemp.dwLoopEnd == 0) {
            shMyTemp.dwLoopEnd = shMyTemp.dwLength - 1;
        }
        
        /* Copy the updated header back */
        memcpy(lpSampleHeader, &shMyTemp, sizeof(SMDI_SampleHeader));
        
        return SF_NATIVE;
    }
    
    /* Unknown format */
    fclose(hFile);
    return FE_UNKNOWNFORMAT;
}

/* Initialize a file-based sample transmission */
DWORD SMDI_InitFileSampleTransmission(SMDI_FileTransmissionInfo* lpFileTransmissionInfo) {
    SMDI_FileTransmissionInfo ftiTemp;
    SMDI_TransmissionInfo tiTemp;
    SMDI_SampleHeader shTemp;
    DWORD dwTemp;
    
    /* Make local copies */
    memcpy(&ftiTemp, lpFileTransmissionInfo, sizeof(SMDI_FileTransmissionInfo));
    memcpy(&tiTemp, ftiTemp.lpTransmissionInfo, sizeof(SMDI_TransmissionInfo));
    
    /* Get the sample header from the file */
    dwTemp = SMDI_GetFileSampleHeader(ftiTemp.cFileName, tiTemp.lpSampleHeader);
    
    /* Make a local copy of the sample header */
    memcpy(&shTemp, tiTemp.lpSampleHeader, sizeof(SMDI_SampleHeader));
    
    /* Default pitch settings */
    shTemp.wPitch = 60;         /* Middle C */
    shTemp.wPitchFraction = 0;
    
    /* Check file format */
    if (dwTemp == SF_NATIVE) {
        /* No need for byte swapping on big-endian IRIX */
        tiTemp.dwCopyMode = CM_NORMAL;
        
        /* Initialize the sample transmission */
        dwTemp = SMDI_InitSampleTransmission(&tiTemp);
        
        if (dwTemp == SMDIM_SENDNEXTPACKET) {
            /* Open the file */
            ftiTemp.hFile = fopen(ftiTemp.cFileName, "rb");
            
            /* Seek to the data */
            fseek(ftiTemp.hFile, shTemp.dwDataOffset, SEEK_SET);
            
            /* Allocate buffer for data */
            tiTemp.lpSampleData = malloc(tiTemp.dwPacketSize);
            
            /* Check for allocation failure */
            if (tiTemp.lpSampleData == NULL) {
                fclose(ftiTemp.hFile);
                return SMDIM_ERROR;
            }
        }
        
        /* Copy back the updated headers */
        memcpy(tiTemp.lpSampleHeader, &shTemp, sizeof(SMDI_SampleHeader));
        memcpy(ftiTemp.lpTransmissionInfo, &tiTemp, sizeof(SMDI_TransmissionInfo));
        memcpy(lpFileTransmissionInfo, &ftiTemp, sizeof(SMDI_FileTransmissionInfo));
    }
    
    return dwTemp;
}

/* File transmission functions */
DWORD SMDI_FileSampleTransmission(SMDI_FileTransmissionInfo* lpFileTransmissionInfo)
{
    SMDI_FileTransmissionInfo ftiTemp;
    SMDI_TransmissionInfo tiTemp;
    SMDI_SampleHeader shTemp;
    DWORD dwTemp;
    size_t bytes_read;
    
    /* Make local copies */
    memcpy(&ftiTemp, lpFileTransmissionInfo, sizeof(SMDI_FileTransmissionInfo));
    memcpy(&tiTemp, ftiTemp.lpTransmissionInfo, sizeof(SMDI_TransmissionInfo));
    memcpy(&shTemp, tiTemp.lpSampleHeader, sizeof(SMDI_SampleHeader));
    
    /* Read the next chunk of data from file - ensure we check the actual bytes read */
    bytes_read = fread(tiTemp.lpSampleData, 1, tiTemp.dwPacketSize, ftiTemp.hFile);
    
    /* Handle potential short read */
    if (bytes_read < tiTemp.dwPacketSize) {
        /* If not at EOF, there was an error */
        if (!feof(ftiTemp.hFile)) {
            /* Log the error */
            printf("Error reading from file: %lu bytes requested, %lu bytes read\n", 
                  tiTemp.dwPacketSize, (unsigned long)bytes_read);
            
            /* Close file and free resources */
            free(tiTemp.lpSampleData);
            fclose(ftiTemp.hFile);
            return SMDIM_ERROR;
        }
        
        /* Adjust packet size for final short packet */
        tiTemp.dwPacketSize = bytes_read;
    }
    
    /* Send the data */
    dwTemp = SMDI_SampleTransmission(&tiTemp);
    
    /* Check for end of procedure */
    if (dwTemp == SMDIM_ENDOFPROCEDURE) {
        /* Free resources */
        free(tiTemp.lpSampleData);
        fclose(ftiTemp.hFile);
        return dwTemp;
    }
    else if (dwTemp != SMDIM_SENDNEXTPACKET) {
        /* Error - free resources */
        free(tiTemp.lpSampleData);
        fclose(ftiTemp.hFile);
    }
    
    /* Copy back the updated headers */
    memcpy(tiTemp.lpSampleHeader, &shTemp, sizeof(SMDI_SampleHeader));
    memcpy(ftiTemp.lpTransmissionInfo, &tiTemp, sizeof(SMDI_TransmissionInfo));
    memcpy(lpFileTransmissionInfo, &ftiTemp, sizeof(SMDI_FileTransmissionInfo));
    
    return dwTemp;
}

/* Initialize file-based sample reception */
DWORD SMDI_InitFileSampleReception(SMDI_FileTransmissionInfo* lpFileTransmissionInfo) {
    SMDI_FileTransmissionInfo ftiTemp;
    SMDI_TransmissionInfo tiTemp;
    SMDI_SampleHeader shTemp;
    DWORD dwTemp;
    DWORD nameLen;
    NativeSampleHeader nativeHdr;
    
    /* Make local copies */
    memcpy(&ftiTemp, lpFileTransmissionInfo, sizeof(SMDI_FileTransmissionInfo));
    memcpy(&tiTemp, ftiTemp.lpTransmissionInfo, sizeof(SMDI_TransmissionInfo));
    memcpy(&shTemp, tiTemp.lpSampleHeader, sizeof(SMDI_SampleHeader));
    
    /* Request the sample header */
    dwTemp = SMDI_SampleHeaderRequest(
        tiTemp.HA_ID, 
        tiTemp.SCSI_ID, 
        tiTemp.dwSampleNumber, 
        &shTemp);
    
    if (dwTemp != SMDIM_SAMPLEHEADER) {
        return dwTemp;
    }
    
    /* Set default packet size */
    tiTemp.dwPacketSize = PACKETSIZE;
    
    /* Open output file */
    ftiTemp.hFile = fopen(ftiTemp.cFileName, "wb");
    if (ftiTemp.hFile == NULL) {
        return FE_OPENERROR;
    }
    
    /* Write native sample format header */
    memcpy(nativeHdr.signature, "SDMP", 4);
    nativeHdr.version = 1;
    nativeHdr.bitsPerSample = shTemp.BitsPerWord;
    nativeHdr.channels = shTemp.NumberOfChannels;
    nativeHdr.loopType = shTemp.LoopControl;
    nativeHdr.reserved = 0;
    nativeHdr.sampleRate = 1000000000 / shTemp.dwPeriod;
    nativeHdr.sampleCount = shTemp.dwLength;
    nativeHdr.loopStart = shTemp.dwLoopStart;
    nativeHdr.loopEnd = shTemp.dwLoopEnd;
    nativeHdr.pitch = shTemp.wPitch;
    nativeHdr.pitchFraction = shTemp.wPitchFraction;
    
    /* Get the name length */
    nameLen = strlen(shTemp.cName);
    if (nameLen > 255) {
        nameLen = 255;
    }
    nativeHdr.nameLength = nameLen;
    
    /* Write header */
    fwrite(&nativeHdr, sizeof(NativeSampleHeader), 1, ftiTemp.hFile);
    
    /* Write name */
    if (nameLen > 0) {
        fwrite(shTemp.cName, 1, nameLen, ftiTemp.hFile);
    }
    
    /* Initialize the sample reception */
    dwTemp = SMDI_InitSampleReception(&tiTemp);
    if (dwTemp != SMDIM_TRANSFERACKNOWLEDGE) {
        fclose(ftiTemp.hFile);
        remove(ftiTemp.cFileName);
        return dwTemp;
    }
    
    /* We're on big-endian IRIX, so no byte swapping needed */
    tiTemp.dwCopyMode = CM_NORMAL;
    
    /* Allocate sample data buffer */
    tiTemp.lpSampleData = malloc(tiTemp.dwPacketSize);
    if (tiTemp.lpSampleData == NULL) {
        fclose(ftiTemp.hFile);
        remove(ftiTemp.cFileName);
        return SMDIM_ERROR;
    }
    
    /* Copy back the updated headers */
    memcpy(tiTemp.lpSampleHeader, &shTemp, sizeof(SMDI_SampleHeader));
    memcpy(ftiTemp.lpTransmissionInfo, &tiTemp, sizeof(SMDI_TransmissionInfo));
    memcpy(lpFileTransmissionInfo, &ftiTemp, sizeof(SMDI_FileTransmissionInfo));
    
    return dwTemp;
}

/* Perform file-based sample reception */
DWORD SMDI_FileSampleReception(SMDI_FileTransmissionInfo* lpFileTransmissionInfo) {
    SMDI_FileTransmissionInfo ftiTemp;
    SMDI_TransmissionInfo tiTemp;
    SMDI_SampleHeader shTemp;
    DWORD dwTemp;
    DWORD bytesToWrite;
    
    /* Make local copies */
    memcpy(&ftiTemp, lpFileTransmissionInfo, sizeof(SMDI_FileTransmissionInfo));
    memcpy(&tiTemp, ftiTemp.lpTransmissionInfo, sizeof(SMDI_TransmissionInfo));
    memcpy(&shTemp, tiTemp.lpSampleHeader, sizeof(SMDI_SampleHeader));
    
    /* Receive the next packet */
    dwTemp = SMDI_SampleReception(&tiTemp);
    
    /* Calculate bytes to write */
    bytesToWrite = tiTemp.dwPacketSize;
    
    /* If not data packet or last packet, adjust size */
    if (dwTemp != SMDIM_DATAPACKET) {
        bytesToWrite = (shTemp.dwLength *
                      ((DWORD)shTemp.NumberOfChannels) *
                      (((DWORD)shTemp.BitsPerWord) / 8)) -
                     (tiTemp.dwPacketSize *
                      (tiTemp.dwTransmittedPackets - 1));
    }
    
    /* Write the data */
    fwrite(tiTemp.lpSampleData, 1, bytesToWrite, ftiTemp.hFile);
    
    /* Check for end of procedure */
    if (dwTemp == SMDIM_ENDOFPROCEDURE) {
        /* Done - close file and free buffer */
        fclose(ftiTemp.hFile);
        free(tiTemp.lpSampleData);
    }
    
    /* Copy back the updated transmission info */
    memcpy(ftiTemp.lpTransmissionInfo, &tiTemp, sizeof(SMDI_TransmissionInfo));
    
    return dwTemp;
}

/* Helper function for sample transmission (for SendFile) */
unsigned long SMDI_SendFileMain(void* lpStart) {
    SMDI_FileTransmissionInfo ftiTemp;
    SMDI_TransmissionInfo tiTemp;
    SMDI_SampleHeader shTemp;
    DWORD dwTemp;
    
    /* Extract parameters from the pointer */
    memcpy(&ftiTemp, lpStart, sizeof(ftiTemp));
    memcpy(&tiTemp, (void*)((char*)lpStart + sizeof(ftiTemp)), sizeof(tiTemp));
    memcpy(&shTemp, (void*)((char*)lpStart + sizeof(ftiTemp) + sizeof(tiTemp)), sizeof(shTemp));
    
    /* Set up the transmission info */
    ftiTemp.lpTransmissionInfo = &tiTemp;
    tiTemp.lpSampleHeader = &shTemp;
    
    /* Free the parameter pointer */
    free(lpStart);
    
    /* Initialize the file transmission */
    dwTemp = SMDI_InitFileSampleTransmission(&ftiTemp);
    if (dwTemp == SMDIM_SENDNEXTPACKET) {
        /* Continue sending packets until done */
        dwTemp = SMDIM_SENDNEXTPACKET;
        while (dwTemp == SMDIM_SENDNEXTPACKET) {
            /* Send the next packet */
            dwTemp = SMDI_FileSampleTransmission(&ftiTemp);
            
            /* Call callback if provided */
            if (ftiTemp.lpCallBackProcedure != NULL) {
                (*ftiTemp.lpCallBackProcedure)(&ftiTemp, ftiTemp.dwUserData);
            }
        }
    }
    
    /* Store result if pointer provided */
    if (ftiTemp.lpReturnValue != NULL) {
        *(ftiTemp.lpReturnValue) = dwTemp;
    }
    
    return dwTemp;
}

/* Send a file to the device */
DWORD SMDI_SendFile(SMDI_FileTransfer* lpFileTransfer) {
    SMDI_FileTransmissionInfo* ftiTemp;
    SMDI_TransmissionInfo* tiTemp;
    SMDI_SampleHeader* shTemp;
    void* lpTemp;
    SMDI_FileTransfer fileTransfer;
    
    /* Allocate memory for the structures */
    ftiTemp = (SMDI_FileTransmissionInfo*)malloc(
        sizeof(SMDI_FileTransmissionInfo) + 
        sizeof(SMDI_TransmissionInfo) + 
        sizeof(SMDI_SampleHeader));
    
    if (ftiTemp == NULL) {
        return SMDIM_ERROR;
    }
    
    /* Calculate pointers */
    lpTemp = ftiTemp;
    tiTemp = (SMDI_TransmissionInfo*)((char*)ftiTemp + sizeof(SMDI_FileTransmissionInfo));
    shTemp = (SMDI_SampleHeader*)((char*)ftiTemp + sizeof(SMDI_FileTransmissionInfo) + sizeof(SMDI_TransmissionInfo));
    
    /* Set default values */
    fileTransfer.dwStructSize = sizeof(fileTransfer);
    fileTransfer.HA_ID = 0;
    fileTransfer.SCSI_ID = 0;
    fileTransfer.dwSampleNumber = 0;
    fileTransfer.lpFileName = NULL;
    fileTransfer.lpSampleName = NULL;
    fileTransfer.bAsync = FALSE;
    fileTransfer.lpCallback = NULL;
    fileTransfer.dwUserData = 0;
    fileTransfer.lpReturnValue = NULL;
    
    /* Copy the provided structure (using the minimum of the two sizes) */
    memcpy(&fileTransfer, lpFileTransfer,
        (lpFileTransfer->dwStructSize > sizeof(fileTransfer)) ?
        sizeof(fileTransfer) : lpFileTransfer->dwStructSize);
    
    /* Initialize structures */
    ftiTemp->dwStructSize = sizeof(*ftiTemp);
    tiTemp->dwStructSize = sizeof(*tiTemp);
    shTemp->dwStructSize = sizeof(*shTemp);
    
    /* Copy parameters */
    tiTemp->HA_ID = fileTransfer.HA_ID;
    tiTemp->SCSI_ID = fileTransfer.SCSI_ID;
    tiTemp->dwSampleNumber = fileTransfer.dwSampleNumber;
    strcpy(ftiTemp->cFileName, fileTransfer.lpFileName);
    
    /* Set up sample name if provided */
    if (fileTransfer.lpSampleName != NULL) {
        strcpy(shTemp->cName, fileTransfer.lpSampleName);
        shTemp->NameLength = (BYTE)strlen(shTemp->cName);
    }
    else {
        /* If no sample name provided, use default or empty */
        memset(shTemp->cName, 0, 256);
        shTemp->NameLength = 0;
    }
    
    /* Always use normal copy mode on big-endian system */
    tiTemp->dwCopyMode = CM_NORMAL;
    
    /* Set callback and user data */
    ftiTemp->lpCallBackProcedure = (void (*)(SMDI_FileTransmissionInfo*, DWORD))fileTransfer.lpCallback;
    ftiTemp->lpReturnValue = fileTransfer.lpReturnValue;
    ftiTemp->dwUserData = fileTransfer.dwUserData;
    
    /* Set references to each other */
    ftiTemp->lpTransmissionInfo = tiTemp;
    tiTemp->lpSampleHeader = shTemp;
    
    /* Initialize return value if provided */
    if (fileTransfer.lpReturnValue != NULL) {
        *(fileTransfer.lpReturnValue) = (DWORD)-1;
    }
    
    /* Execute synchronously */
    return SMDI_SendFileMain(lpTemp);
}

/* Helper function for sample reception (for ReceiveFile) */
unsigned long SMDI_ReceiveFileMain(void* lpStart) {
    SMDI_FileTransmissionInfo ftiTemp;
    SMDI_TransmissionInfo tiTemp;
    SMDI_SampleHeader shTemp;
    DWORD dwTemp;
    
    /* Extract parameters from the pointer */
    memcpy(&ftiTemp, lpStart, sizeof(ftiTemp));
    memcpy(&tiTemp, (void*)((char*)lpStart + sizeof(ftiTemp)), sizeof(tiTemp));
    memcpy(&shTemp, (void*)((char*)lpStart + sizeof(ftiTemp) + sizeof(tiTemp)), sizeof(shTemp));
    
    /* Set up the transmission info */
    ftiTemp.lpTransmissionInfo = &tiTemp;
    tiTemp.lpSampleHeader = &shTemp;
    
    /* Free the parameter pointer */
    free(lpStart);
    
    /* Initialize the file reception */
    dwTemp = SMDI_InitFileSampleReception(&ftiTemp);
    if (dwTemp == SMDIM_TRANSFERACKNOWLEDGE) {
        /* Continue receiving packets until done */
        dwTemp = SMDIM_DATAPACKET;
        while (dwTemp == SMDIM_DATAPACKET) {
            /* Receive the next packet */
            dwTemp = SMDI_FileSampleReception(&ftiTemp);
            
            /* Call callback if provided */
            if (ftiTemp.lpCallBackProcedure != NULL) {
                (*ftiTemp.lpCallBackProcedure)(&ftiTemp, ftiTemp.dwUserData);
            }
        }
    }
    
    /* Store result if pointer provided */
    if (ftiTemp.lpReturnValue != NULL) {
        *(ftiTemp.lpReturnValue) = dwTemp;
    }
    
    return dwTemp;
}

/* Receive a file from the device */
DWORD SMDI_ReceiveFile(SMDI_FileTransfer* lpFileTransfer) {
    SMDI_FileTransmissionInfo* ftiTemp;
    SMDI_TransmissionInfo* tiTemp;
    SMDI_SampleHeader* shTemp;
    void* lpTemp;
    SMDI_FileTransfer fileTransfer;
    
    /* Allocate memory for the structures */
    ftiTemp = (SMDI_FileTransmissionInfo*)malloc(
        sizeof(SMDI_FileTransmissionInfo) + 
        sizeof(SMDI_TransmissionInfo) + 
        sizeof(SMDI_SampleHeader));
    
    if (ftiTemp == NULL) {
        return SMDIM_ERROR;
    }
    
    /* Calculate pointers */
    lpTemp = ftiTemp;
    tiTemp = (SMDI_TransmissionInfo*)((char*)ftiTemp + sizeof(SMDI_FileTransmissionInfo));
    shTemp = (SMDI_SampleHeader*)((char*)ftiTemp + sizeof(SMDI_FileTransmissionInfo) + sizeof(SMDI_TransmissionInfo));
    
    /* Set default values */
    fileTransfer.dwStructSize = sizeof(fileTransfer);
    fileTransfer.HA_ID = 0;
    fileTransfer.SCSI_ID = 0;
    fileTransfer.dwSampleNumber = 0;
    fileTransfer.lpFileName = NULL;
    fileTransfer.dwFileType = SF_NATIVE;
    fileTransfer.bAsync = FALSE;
    fileTransfer.lpCallback = NULL;
    fileTransfer.dwUserData = 0;
    fileTransfer.lpReturnValue = NULL;
    
    /* Copy the provided structure (using the minimum of the two sizes) */
    memcpy(&fileTransfer, lpFileTransfer,
        (lpFileTransfer->dwStructSize > sizeof(fileTransfer)) ?
        sizeof(fileTransfer) : lpFileTransfer->dwStructSize);
    
    /* Initialize structures */
    ftiTemp->dwStructSize = sizeof(*ftiTemp);
    tiTemp->dwStructSize = sizeof(*tiTemp);
    shTemp->dwStructSize = sizeof(*shTemp);
    
    /* Copy parameters */
    tiTemp->dwSampleNumber = fileTransfer.dwSampleNumber;
    tiTemp->HA_ID = fileTransfer.HA_ID;
    tiTemp->SCSI_ID = fileTransfer.SCSI_ID;
    
    /* Always use normal copy mode on big-endian system */
    tiTemp->dwCopyMode = CM_NORMAL;
    
    ftiTemp->dwFileType = fileTransfer.dwFileType;
    ftiTemp->lpReturnValue = fileTransfer.lpReturnValue;
    strcpy(ftiTemp->cFileName, fileTransfer.lpFileName);
    ftiTemp->lpCallBackProcedure = (void (*)(SMDI_FileTransmissionInfo*, DWORD))fileTransfer.lpCallback;
    ftiTemp->dwUserData = fileTransfer.dwUserData;
    
    /* Set references to each other */
    ftiTemp->lpTransmissionInfo = tiTemp;
    tiTemp->lpSampleHeader = shTemp;
    
    /* Initialize return value if provided */
    if (fileTransfer.lpReturnValue != NULL) {
        *(fileTransfer.lpReturnValue) = (DWORD)-1;
    }
    
    /* Execute synchronously */
    return SMDI_ReceiveFileMain(lpTemp);
}
