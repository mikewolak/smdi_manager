/*
 * SMDI utility functions for IRIX 5.3
 * ANSI C90 compliant implementation for MIPS big-endian architecture
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include "smdi.h"
#include "aspi_irix.h"
#include "scsi_debug.h"

/* Standard data packet size for SMDI transfers */
#define PACKETSIZE 16384

/* Global command buffer */
static unsigned char smdicmd[256];

/* Global debug flag - changed to non-static so it can be accessed from other files */
int g_smdi_debug_enabled = 0;

/* Sleep function for IRIX */
static void sleep_ms(int ms) {
    /* Convert ms to clock ticks (10ms each), rounding up */
    sginap((ms + 9) / 10);
}

/* Debug print function */
static void debug_print(const char* format, ...) {
    va_list args;
    
    if (!g_smdi_debug_enabled) {
        return;
    }
    
    va_start(args, format);
    printf("SMDI DEBUG: ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

/* Get the last SMDI error code */
DWORD SMDI_GetLastError(void) {
    DWORD error_code;
    
    /* For MessageReject, the error code is in bytes 11-14 */
    /* Reconstruct 32-bit value in big-endian order */
    error_code = ((DWORD)smdicmd[11] << 24) |
                ((DWORD)smdicmd[12] << 16) |
                ((DWORD)smdicmd[13] << 8) |
                (DWORD)smdicmd[14];
    
    return error_code;
}

/* Public function to get/set debug mode */
void SMDI_SetDebugMode(int enable) {
    g_smdi_debug_enabled = enable ? 1 : 0;
    
    if (enable) {
        printf("SMDI DEBUG: Debug mode enabled\n");
    } else {
        printf("SMDI DEBUG: Debug mode disabled\n");
    }
}

/* Get debug mode status */
int SMDI_GetDebugMode(void) {
    return g_smdi_debug_enabled;
}

/* Create an SMDI message header */
static void SMDI_MakeMessageHeader(unsigned char mheader[], 
                                  DWORD messageID, 
                                  DWORD additionalLength) {
    /* The "SMDI" signature */
    memcpy(mheader, "SMDI", 4);
    
    /* Message ID - SMDI protocol splits this into 2 16-bit values:
       - First 16 bits: Message ID
       - Second 16 bits: Message Sub-ID
       IMPORTANT: Big-endian order for each 16-bit value */
    mheader[4] = (unsigned char)((messageID >> 24) & 0xFF);
    mheader[5] = (unsigned char)((messageID >> 16) & 0xFF);
    mheader[6] = (unsigned char)((messageID >> 8) & 0xFF);
    mheader[7] = (unsigned char)(messageID & 0xFF);
    
    /* Additional length - SMDI uses a 24-bit big-endian value */
    mheader[8] = (unsigned char)((additionalLength >> 16) & 0xFF);
    mheader[9] = (unsigned char)((additionalLength >> 8) & 0xFF);
    mheader[10] = (unsigned char)(additionalLength & 0xFF);
    
    debug_print("Created SMDI header: ID=%08lX, Len=%lu", messageID, additionalLength);
}

/* Get additional length from an SMDI message header */
static DWORD SMDI_GetAdditionalLength(unsigned char mheader[]) {
    DWORD length;
    
    /* Extract 24-bit value (3 bytes) */
    length = ((DWORD)mheader[8] << 16) | 
             ((DWORD)mheader[9] << 8) | 
             (DWORD)mheader[10];
    
    return length;
}

/* Get whole message ID from an SMDI message header */
static DWORD SMDI_GetWholeMessageID(unsigned char mheader[]) {
    DWORD messageID;
    
    /* Reconstruct 32-bit message ID from 4 bytes in big-endian order */
    messageID = ((DWORD)mheader[4] << 24) |
               ((DWORD)mheader[5] << 16) |
               ((DWORD)mheader[6] << 8) |
               (DWORD)mheader[7];
    
    debug_print("Parsed message ID: 0x%08lX from bytes: %02X %02X %02X %02X",
               messageID, mheader[4], mheader[5], mheader[6], mheader[7]);
    
    return messageID;
}

/* Get a message from the device */
DWORD SMDI_GetMessage(BYTE ha_id, BYTE id) {
    scsi_debug_t debug;
    unsigned long bytes_received;
    
    scsi_debug_init(&debug);
    debug.enabled = g_smdi_debug_enabled;
    
    debug_print("Getting message from device %d:%d", ha_id, id);
    bytes_received = ASPI_Receive(&debug, ha_id, id, smdicmd, 256);
    
    debug_print("Received %lu bytes", bytes_received);
    
    return SMDI_GetWholeMessageID(smdicmd);
}

/* Send an SMDI data packet to the device */
DWORD SMDI_SendDataPacket(BYTE ha_id,
                        BYTE id, 
                        DWORD pn, 
                        void* data, 
                        DWORD length) {
    void* datamessage;
    DWORD result;
    scsi_debug_t debug;
    int send_success;
    
    scsi_debug_init(&debug);
    debug.enabled = g_smdi_debug_enabled;

    debug_print("SendDataPacket to %d:%d, packet %lu, length %lu", 
                ha_id, id, pn, length);

    datamessage = malloc(14 + length);
    if (datamessage == NULL) {
        debug_print("ERROR: Failed to allocate memory for data packet");
        return SMDIM_ERROR;
    }
    
    /* Prepare the message header */
    SMDI_MakeMessageHeader(smdicmd, SMDIM_DATAPACKET, 3 + length);
    
    /* Set packet number (24-bit value) */
    smdicmd[11] = (unsigned char)((pn >> 16) & 0xFF);
    smdicmd[12] = (unsigned char)((pn >> 8) & 0xFF);
    smdicmd[13] = (unsigned char)(pn & 0xFF);
    
    /* Copy the header to the message buffer */
    memcpy(datamessage, smdicmd, 14);

    /* Copy the data directly (no byte swapping needed on big-endian system) */
    memcpy((void*)(((unsigned long)datamessage) + 14), data, length);
    
    /* Send the data packet */
    send_success = ASPI_Send(&debug, ha_id, id, datamessage, 14 + length);
    
    if (!send_success) {
        debug_print("ERROR: ASPI_Send failed");
        free(datamessage);
        return SMDIM_ERROR;
    }
    
    /* Wait a short time for the device to process */
    sleep_ms(50);
    
    /* Receive the response */
    ASPI_Receive(&debug, ha_id, id, smdicmd, 256);
    
    /* Get the message ID from the response */
    result = SMDI_GetWholeMessageID(smdicmd);
    debug_print("Response message ID: 0x%08lX", result);
    
    /* Free the message buffer */
    free(datamessage);
    
    return result;
}

/* Send a sample name to the device */
DWORD SMDI_SampleName(BYTE ha_id,
                    BYTE id,
                    DWORD sampleNum,
                    char sampleName[]) {
    DWORD nameLen;
    scsi_debug_t debug;
    int send_success;
    
    scsi_debug_init(&debug);
    debug.enabled = g_smdi_debug_enabled;
    
    nameLen = strlen(sampleName);
    
    debug_print("SMDI_SampleName: Setting name for sample %lu to '%s'", 
                sampleNum, sampleName);
    
    /* Prepare the message header */
    SMDI_MakeMessageHeader(smdicmd, SMDIM_SAMPLENAME, 0x000004 + nameLen);
    
    /* Sample Number (24-bit value) */
    smdicmd[11] = (unsigned char)((sampleNum >> 16) & 0xFF);
    smdicmd[12] = (unsigned char)((sampleNum >> 8) & 0xFF);
    smdicmd[13] = (unsigned char)(sampleNum & 0xFF);
    
    /* Sample Name Length */
    smdicmd[14] = (unsigned char)nameLen;
    
    /* Sample Name */
    memcpy(&smdicmd[15], sampleName, nameLen);
    
    /* Send the command */
    send_success = ASPI_Send(&debug, ha_id, id, smdicmd, 15 + nameLen);
    
    if (!send_success) {
        debug_print("ERROR: ASPI_Send failed");
        return SMDIM_ERROR;
    }
    
    /* Wait a short time for the device to process */
    sleep_ms(50);
    
    /* Receive the response */
    ASPI_Receive(&debug, ha_id, id, smdicmd, 256);
    
    return SMDI_GetWholeMessageID(smdicmd);
}

/* Send a "Begin Sample Transfer" command */
DWORD SMDI_SendBeginSampleTransfer(BYTE ha_id,
                                 BYTE id,
                                 DWORD sampleNum,
                                 void* packetLength) {
    scsi_debug_t debug;
    DWORD result;
    DWORD length;
    int send_success;
    
    scsi_debug_init(&debug);
    debug.enabled = g_smdi_debug_enabled;
    
    /* Copy the packet length */
    memcpy(&length, packetLength, sizeof(DWORD));
    
    debug_print("Begin Sample Transfer for sample %lu, packet length %lu", 
                sampleNum, length);
    
    /* Prepare the message header */
    SMDI_MakeMessageHeader(smdicmd, SMDIM_BEGINSAMPLETRANSFER, 0x000006);
    
    /* Sample Number (24-bit value) */
    smdicmd[11] = (unsigned char)((sampleNum >> 16) & 0xFF);
    smdicmd[12] = (unsigned char)((sampleNum >> 8) & 0xFF);
    smdicmd[13] = (unsigned char)(sampleNum & 0xFF);
    
    /* Data Packet Length (24-bit value) */
    smdicmd[14] = (unsigned char)((length >> 16) & 0xFF);
    smdicmd[15] = (unsigned char)((length >> 8) & 0xFF);
    smdicmd[16] = (unsigned char)(length & 0xFF);
    
    /* Send the command */
    send_success = ASPI_Send(&debug, ha_id, id, smdicmd, 17);
    
    if (!send_success) {
        debug_print("ERROR: ASPI_Send failed");
        return SMDIM_ERROR;
    }
    
    /* Wait a short time for the device to process */
    sleep_ms(50);
    
    /* Receive the response */
    ASPI_Receive(&debug, ha_id, id, smdicmd, 256);
    
    result = SMDI_GetWholeMessageID(smdicmd);
    
    /* If transfer acknowledge, get the packet length from the response */
    if (result == SMDIM_TRANSFERACKNOWLEDGE) {
        DWORD respLength;
        
        /* Extract 24-bit value */
        respLength = ((DWORD)smdicmd[14] << 16) |
                     ((DWORD)smdicmd[15] << 8) |
                     (DWORD)smdicmd[16];
        
        debug_print("Transfer acknowledged, new packet length is %lu", respLength);
        
        /* Update the packet length */
        memcpy(packetLength, &respLength, sizeof(DWORD));
    }
    
    return result;
}

/* Send a sample header */
DWORD SMDI_SendSampleHeader(BYTE ha_id,
                          BYTE id,
                          DWORD sampleNum,
                          SMDI_SampleHeader* shA,
                          DWORD* dataPacketLength) {
    SMDI_SampleHeader sh;
    scsi_debug_t debug;
    DWORD result;
    int send_success;
    
    scsi_debug_init(&debug);
    debug.enabled = g_smdi_debug_enabled;
    
    /* Make a copy of the sample header */
    memcpy(&sh, shA, sizeof(SMDI_SampleHeader));
    
    debug_print("Sending sample header for sample %lu", sampleNum);
    debug_print("  Bits: %d, Channels: %d, Length: %lu", 
               sh.BitsPerWord, sh.NumberOfChannels, sh.dwLength);
    
    /* Prepare the message header */
    SMDI_MakeMessageHeader(smdicmd, SMDIM_SAMPLEHEADER, 0x00001a + (DWORD)sh.NameLength);
    
    /* Sample number (24-bit value) */
    smdicmd[11] = (unsigned char)((sampleNum >> 16) & 0xFF);
    smdicmd[12] = (unsigned char)((sampleNum >> 8) & 0xFF);
    smdicmd[13] = (unsigned char)(sampleNum & 0xFF);
    
    /* Bits per word */
    smdicmd[14] = sh.BitsPerWord;
    
    /* Number of channels */
    smdicmd[15] = sh.NumberOfChannels;
    
    /* Sample Period (nanoseconds) */
    smdicmd[16] = (unsigned char)((sh.dwPeriod >> 16) & 0xFF);
    smdicmd[17] = (unsigned char)((sh.dwPeriod >> 8) & 0xFF);
    smdicmd[18] = (unsigned char)(sh.dwPeriod & 0xFF);
    
    /* Sample Length (words) */
    smdicmd[19] = (unsigned char)((sh.dwLength >> 24) & 0xFF);
    smdicmd[20] = (unsigned char)((sh.dwLength >> 16) & 0xFF);
    smdicmd[21] = (unsigned char)((sh.dwLength >> 8) & 0xFF);
    smdicmd[22] = (unsigned char)(sh.dwLength & 0xFF);
    
    /* Loop Start (word number) */
    smdicmd[23] = (unsigned char)((sh.dwLoopStart >> 24) & 0xFF);
    smdicmd[24] = (unsigned char)((sh.dwLoopStart >> 16) & 0xFF);
    smdicmd[25] = (unsigned char)((sh.dwLoopStart >> 8) & 0xFF);
    smdicmd[26] = (unsigned char)(sh.dwLoopStart & 0xFF);
    
    /* Loop End (word number) */
    smdicmd[27] = (unsigned char)((sh.dwLoopEnd >> 24) & 0xFF);
    smdicmd[28] = (unsigned char)((sh.dwLoopEnd >> 16) & 0xFF);
    smdicmd[29] = (unsigned char)((sh.dwLoopEnd >> 8) & 0xFF);
    smdicmd[30] = (unsigned char)(sh.dwLoopEnd & 0xFF);
    
    /* Loop Control */
    smdicmd[31] = sh.LoopControl;
    
    /* Pitch Integer */
    smdicmd[32] = (unsigned char)((sh.wPitch >> 8) & 0xFF);
    smdicmd[33] = (unsigned char)(sh.wPitch & 0xFF);
    
    /* Pitch Fraction */
    smdicmd[34] = (unsigned char)((sh.wPitchFraction >> 8) & 0xFF);
    smdicmd[35] = (unsigned char)(sh.wPitchFraction & 0xFF);
    
    /* Sample Name Length */
    smdicmd[36] = sh.NameLength;
    
    /* Sample Name */
    memcpy(&smdicmd[37], &sh.cName, (unsigned long)sh.NameLength);
    
    /* Send the command */
    send_success = ASPI_Send(&debug, ha_id, id, smdicmd, 37 + (unsigned long)sh.NameLength);
    
    if (!send_success) {
        debug_print("ERROR: ASPI_Send failed");
        return SMDIM_ERROR;
    }
    
    /* Wait a short time for the device to process */
    sleep_ms(50);
    
    /* Receive the response */
    ASPI_Receive(&debug, ha_id, id, smdicmd, 256);
    
    result = SMDI_GetWholeMessageID(smdicmd);
    
    /* If transfer acknowledge, get the packet length from the response */
    if (result == SMDIM_TRANSFERACKNOWLEDGE) {
        DWORD respLength;
        
        /* Extract 24-bit value */
        respLength = ((DWORD)smdicmd[14] << 16) |
                     ((DWORD)smdicmd[15] << 8) |
                     (DWORD)smdicmd[16];
        
        debug_print("Transfer acknowledged, packet length is %lu", respLength);
        
        /* Update the packet length */
        *dataPacketLength = respLength;
    }

    if (result != SMDIM_TRANSFERACKNOWLEDGE) {
        debug_print("Error response from sampler: 0x%08lx", result);
        if (result == SMDIM_MESSAGEREJECT) {
            debug_print("Last error code: 0x%08lx", SMDI_GetLastError());
        }
    }
    
    return result;
}

/* Request the next data packet */
DWORD SMDI_NextDataPacketRequest(BYTE ha_id,
                               BYTE id,
                               DWORD packetNumber,
                               void* buffer,
                               DWORD maxlen) {
    void* mybuffer;
    DWORD reply;
    scsi_debug_t debug;
    int send_success;
    
    scsi_debug_init(&debug);
    debug.enabled = g_smdi_debug_enabled;
    
    debug_print("NextDataPacketRequest: Requesting packet %lu from device %d:%d", 
               packetNumber, ha_id, id);
    
    mybuffer = malloc(maxlen + 14);
    if (mybuffer == NULL) {
        debug_print("ERROR: Failed to allocate memory");
        return SMDIM_ERROR;
    }
    
    /* Prepare the message header */
    SMDI_MakeMessageHeader(smdicmd, SMDIM_SENDNEXTPACKET, 0x000003);
    
    /* Packet Number (24-bit value) */
    smdicmd[11] = (unsigned char)((packetNumber >> 16) & 0xFF);
    smdicmd[12] = (unsigned char)((packetNumber >> 8) & 0xFF);
    smdicmd[13] = (unsigned char)(packetNumber & 0xFF);
    
    /* Send the command */
    send_success = ASPI_Send(&debug, ha_id, id, smdicmd, 14);
    
    if (!send_success) {
        debug_print("ERROR: ASPI_Send failed");
        free(mybuffer);
        return SMDIM_ERROR;
    }
    
    /* Wait a short time for the device to process */
    sleep_ms(50);
    
    /* Receive the response */
    ASPI_Receive(&debug, ha_id, id, mybuffer, maxlen + 14);
    
    /* Copy the data directly (no byte swapping needed on big-endian system) */
    memcpy(buffer, (void*)(((unsigned long)mybuffer) + 14), maxlen);
    
    /* Get the message ID from the response */
    reply = SMDI_GetWholeMessageID(mybuffer);
    debug_print("Reply message ID: 0x%08lX", reply);
    
    /* Free the temporary buffer */
    free(mybuffer);
    
    return reply;
}

/* Request a sample header */
DWORD SMDI_SampleHeaderRequest(BYTE ha_id,
                             BYTE id,
                             DWORD sampleNum,
                             SMDI_SampleHeader* shTemp) {
    SMDI_SampleHeader sh;
    scsi_debug_t debug;
    DWORD result;
    unsigned char cmd[14];
    int i;
    int send_result; /* Declare variable with correct name */
    
    scsi_debug_init(&debug);
    debug.enabled = g_smdi_debug_enabled;
    
    debug_print("SampleHeaderRequest for sample %lu on device %d:%d", 
               sampleNum, ha_id, id);
    
    /* Add NULL check */
    if (shTemp == NULL) {
        debug_print("ERROR: shTemp is NULL");
        return SMDIM_ERROR;
    }
    
    /* Make a copy of the sample header */
    memcpy(&sh, shTemp, sizeof(SMDI_SampleHeader));
    
    /* Prepare the message header */
    /* SMDI header signature */
    memcpy(cmd, "SMDI", 4);
    
    /* Message ID for SMDIM_SAMPLEHEADERREQUEST (0x01200000) */
    cmd[4] = 0x01; 
    cmd[5] = 0x20; 
    /* cmd[4] = 0x00; */
    /* cmd[5] = 0x12; */
    cmd[6] = 0x00;
    cmd[7] = 0x00;
    
    /* Additional length (3 bytes) */
    cmd[8] = 0x00;
    cmd[9] = 0x00;
    cmd[10] = 0x03;
    
    /* Sample Number (24-bit value) */
    cmd[11] = (unsigned char)((sampleNum >> 16) & 0xFF);
    cmd[12] = (unsigned char)((sampleNum >> 8) & 0xFF);
    cmd[13] = (unsigned char)(sampleNum & 0xFF);
    
    if (g_smdi_debug_enabled) {
        debug_print("Command bytes:");
        for (i = 0; i < 14; i++) {
            printf("%02X ", cmd[i]);
        }
        printf("\n");
    }
    
    /* Send the command */
    send_result = ASPI_Send(&debug, ha_id, id, cmd, 14);
    
    /* Check what ASPI_Send returned to determine the cause of failure */
    debug_print("ASPI_Send returned %d", send_result);
    
    if (!send_result) {
        /* If ASPI_Send failed, let's check if the device exists and is ready */
        int ready = ASPI_TestUnitReady(&debug, ha_id, id);
        debug_print("ASPI_TestUnitReady returned %d", ready);
        
        if (!ready) {
            debug_print("ERROR: Device %d:%d is not ready", ha_id, id);
        } else {
            debug_print("ERROR: Device is ready but ASPI_Send failed");
        }
        
        return SMDIM_ERROR;
    }
    
    /* Wait a bit longer for the device to process - 100ms */
    sleep_ms(100);
    
    /* Clear the receive buffer first */
    memset(smdicmd, 0, sizeof(smdicmd));
    
    /* Receive the response */
    ASPI_Receive(&debug, ha_id, id, smdicmd, 256);
    
    if (g_smdi_debug_enabled) {
        debug_print("Response first 16 bytes:");
        for (i = 0; i < 16; i++) {
            printf("%02X ", smdicmd[i]);
        }
        printf("\n");
    }
    
    /* Check if it's a valid SMDI response */
    if (memcmp(smdicmd, "SMDI", 4) == 0) {
        /* Extract message ID - directly from bytes 4-7 */
        result = ((DWORD)smdicmd[4] << 24) |
                ((DWORD)smdicmd[5] << 16) |
                ((DWORD)smdicmd[6] << 8) |
                (DWORD)smdicmd[7];
        
        debug_print("SMDI response ID: 0x%08lX", result);
        
        /* Check for SAMPLEHEADER response (0x01210000) */
        if (result == SMDIM_SAMPLEHEADER) {
            /* Parse sample header data */
            sh.bDoesExist = TRUE;
            sh.BitsPerWord = smdicmd[14]; /* Bits Per Word */
            sh.NumberOfChannels = smdicmd[15]; /* Number Of Channels */
            
            /* Period (24-bit value) */
            sh.dwPeriod = ((DWORD)smdicmd[16] << 16) |
                         ((DWORD)smdicmd[17] << 8) |
                         (DWORD)smdicmd[18];
            
            /* Sample Length */
            sh.dwLength = ((DWORD)smdicmd[19] << 24) |
                         ((DWORD)smdicmd[20] << 16) |
                         ((DWORD)smdicmd[21] << 8) |
                         (DWORD)smdicmd[22];
            
            /* Loop Start */
            sh.dwLoopStart = ((DWORD)smdicmd[23] << 24) |
                            ((DWORD)smdicmd[24] << 16) |
                            ((DWORD)smdicmd[25] << 8) |
                            (DWORD)smdicmd[26];
            
            /* Loop End */
            sh.dwLoopEnd = ((DWORD)smdicmd[27] << 24) |
                          ((DWORD)smdicmd[28] << 16) |
                          ((DWORD)smdicmd[29] << 8) |
                          (DWORD)smdicmd[30];
            
            /* Loop Control */
            sh.LoopControl = smdicmd[31];
            
            /* Pitch */
            sh.wPitch = ((WORD)smdicmd[32] << 8) |
                       (WORD)smdicmd[33];
            
            /* Pitch Fraction */
            sh.wPitchFraction = ((WORD)smdicmd[34] << 8) |
                               (WORD)smdicmd[35];
            
            /* Name length */
            sh.NameLength = smdicmd[36];
            
            /* Name */
            memset(&sh.cName, 0, 256);
            memcpy(sh.cName, &smdicmd[37], (unsigned long)sh.NameLength);
            
            debug_print("Sample exists: %s", sh.bDoesExist ? "Yes" : "No");
            debug_print("Bits per word: %d", sh.BitsPerWord);
            debug_print("Channels: %d", sh.NumberOfChannels);
            debug_print("Period: %lu", sh.dwPeriod);
            debug_print("Length: %lu", sh.dwLength);
            debug_print("Name: %s", sh.cName);
        } else {
            sh.bDoesExist = FALSE;
            debug_print("Sample does not exist (response: 0x%08lX)", result);
        }
    } else {
        debug_print("Not a valid SMDI response");
        result = SMDIM_ERROR;
        sh.bDoesExist = FALSE;
    }
    
    /* Copy the sample header back */
    memcpy(shTemp, &sh, sizeof(SMDI_SampleHeader));
    
    return result;
}

/* Delete a sample */
DWORD SMDI_DeleteSample(BYTE ha_id,
                      BYTE id,
                      DWORD sampleNum) {
    DWORD messageID;
    scsi_debug_t debug;
    unsigned char cmd[14];
    int i;
    int send_success;
    
    scsi_debug_init(&debug);
    debug.enabled = g_smdi_debug_enabled;
    
    debug_print("Deleting sample number: %lu", sampleNum);
    
    /* Prepare the message header */
    /* SMDI header signature */
    memcpy(cmd, "SMDI", 4);
    
    /* Message ID for SMDIM_DELETESAMPLE (0x01240000) */
    cmd[4] = 0x01;
    cmd[5] = 0x24;
    cmd[6] = 0x00;
    cmd[7] = 0x00;
    
    /* Additional length (3 bytes) */
    cmd[8] = 0x00;
    cmd[9] = 0x00;
    cmd[10] = 0x03;
    
    /* Sample Number (24-bit value) */
    cmd[11] = (unsigned char)((sampleNum >> 16) & 0xFF);
    cmd[12] = (unsigned char)((sampleNum >> 8) & 0xFF);
    cmd[13] = (unsigned char)(sampleNum & 0xFF);
    
    if (g_smdi_debug_enabled) {
        debug_print("Command to send:");
        for (i = 0; i < 14; i++) {
            printf(" %02X", cmd[i]);
        }
        printf("\n");
    }
    
    /* Send the command */
    send_success = ASPI_Send(&debug, ha_id, id, cmd, 14);
    
    if (!send_success) {
        debug_print("ERROR: ASPI_Send failed");
        return SMDIM_ERROR;
    }
    
    /* Wait a bit longer for the device to process - 100ms */
    sleep_ms(100);
    
    /* Clear the receive buffer first */
    memset(smdicmd, 0, sizeof(smdicmd));
    
    /* Receive the response */
    ASPI_Receive(&debug, ha_id, id, smdicmd, 256);
    
    /* Enhanced debug - dump full response buffer */
    if (g_smdi_debug_enabled) {
        debug_print("Response dump (first 32 bytes):");
        for (i = 0; i < 32; i++) {
            if (i % 16 == 0) {
                printf("\n%04X: ", i);
            }
            printf("%02X ", smdicmd[i]);
        }
        printf("\n");
    }
    
    /* Check if it's a valid SMDI response */
    if (memcmp(smdicmd, "SMDI", 4) == 0) {
        /* Extract message ID - directly from bytes 4-7 */
        messageID = ((DWORD)smdicmd[4] << 24) |
                   ((DWORD)smdicmd[5] << 16) |
                   ((DWORD)smdicmd[6] << 8) |
                   (DWORD)smdicmd[7];
        
        debug_print("Received message ID: 0x%08lX", messageID);
        
        if (g_smdi_debug_enabled) {
            debug_print("Response Bytes:");
            for (i = 0; i < 16; i++) {
                printf(" %02X", smdicmd[i]);
            }
            printf("\n");
        }
        
        if (messageID == SMDIM_MESSAGEREJECT) {
            debug_print("Error in MessageReject, looking at raw bytes around error code");
            if (g_smdi_debug_enabled) {
                debug_print("Bytes 8-15:");
                for (i = 8; i < 16; i++) {
                    printf(" %02X", smdicmd[i]);
                }
                printf("\n");
            }
            
            return SMDI_GetLastError();
        } else {
            debug_print("Successful response: 0x%08lX", messageID);
            return messageID;
        }
    } else {
        debug_print("Not a valid SMDI response");
        return SMDIM_ERROR;
    }
}

/* Identify a master device */
DWORD SMDI_MasterIdentify(BYTE ha_id, BYTE id) {
    scsi_debug_t debug;
    DWORD response;
    int i;
    int send_success;
    
    /* Use exactly the same command bytes that worked in the Linux version */
    unsigned char cmd[] = {
        'S', 'M', 'D', 'I',  /* Signature */
        0x00, 0x01, 0x00, 0x00,  /* Message ID (SMDIM_MASTERIDENTIFY) */
        0x00, 0x00, 0x00  /* Additional length (0) */
    };
    
    scsi_debug_init(&debug);
    debug.enabled = g_smdi_debug_enabled;
    
    debug_print("SMDI_MasterIdentify: Sending to device %d:%d", ha_id, id);
    
    if (g_smdi_debug_enabled) {
        debug_print("SMDI Command Bytes:");
        for (i = 0; i < 11; i++) {
            printf(" %02X", cmd[i]);
        }
        printf("\n");
    }
    
    /* Send the manually constructed command */
    send_success = ASPI_Send(&debug, ha_id, id, cmd, 11);
    
    if (!send_success) {
        debug_print("ERROR: ASPI_Send failed");
        return SMDIM_ERROR;
    }
    
    /* Wait longer for the device to process - 50ms */
    sleep_ms(50);
    
    /* Clear the receive buffer first */
    memset(smdicmd, 0, sizeof(smdicmd));
    
    /* Receive the response */
    ASPI_Receive(&debug, ha_id, id, smdicmd, 256);
    
    if (g_smdi_debug_enabled) {
        debug_print("Raw Response:");
        for (i = 0; i < 16; i++) {
            printf(" %02X", smdicmd[i]);
        }
        printf("\n");
    }
    
    /* Check if it's a valid SMDI response */
    if (memcmp(smdicmd, "SMDI", 4) == 0) {
        /* Extract message ID - directly from bytes 4-7 */
        response = ((DWORD)smdicmd[4] << 24) |
                  ((DWORD)smdicmd[5] << 16) |
                  ((DWORD)smdicmd[6] << 8) |
                  (DWORD)smdicmd[7];
        
        debug_print("Valid SMDI response: 0x%08lX", response);
    } else {
        debug_print("Not a valid SMDI response");
        response = 0; /* Not a valid response */
    }
    
    return response;
}

/* Initialize SMDI */
BYTE SMDI_Init(void) {
    int result;
    
    debug_print("SMDI_Init: Initializing SMDI");
    
    /* Cast int return value from ASPI_Check to BYTE */
    result = ASPI_Check(NULL);
    
    debug_print("ASPI_Check returned %d, casting to BYTE", result);
    
    return (BYTE)result;
}

/* Test if a device is ready */
BOOL SMDI_TestUnitReady(BYTE ha_id, BYTE id) {
    int result;
    scsi_debug_t debug;
    
    scsi_debug_init(&debug);
    debug.enabled = g_smdi_debug_enabled;
    
    debug_print("SMDI_TestUnitReady: Checking device %d:%d", ha_id, id);
    
    /* ASPI_TestUnitReady returns int which we are expecting as BOOL */
    result = ASPI_TestUnitReady(&debug, ha_id, id);
    
    debug_print("ASPI_TestUnitReady returned %d", result);
    
    return result;
}

/* Get device information */
void SMDI_GetDeviceInfo(BYTE ha_id, BYTE id, SCSI_DevInfo* info) {
    SCSI_DevInfo devInfo;
    char inquire[96];
    scsi_debug_t debug;
    DWORD response;
    
    scsi_debug_init(&debug);
    debug.enabled = g_smdi_debug_enabled;
    
    debug_print("SMDI_GetDeviceInfo: Getting info for device %d:%d", ha_id, id);
    
    memset(inquire, 0, 96);
    devInfo.dwStructSize = sizeof(SCSI_DevInfo);
    
    ASPI_InquireDevice(&debug, inquire, ha_id, id);
    devInfo.DevType = inquire[0] & 0x1f;
    devInfo.bSMDI = FALSE;
    
    memset(&devInfo.cName, 0, 20);
    memset(&devInfo.cManufacturer, 0, 12);
    memcpy(devInfo.cName, &inquire[16], 16);
    memcpy(devInfo.cManufacturer, &inquire[8], 8);
    
    debug_print("Device %d:%d - Type: %d, Manufacturer: %.8s, Name: %.16s", 
               ha_id, id, devInfo.DevType, devInfo.cManufacturer, devInfo.cName);
    
    /* Try SMDI identification. We'll primarily check "Processor"
       type devices (Type 3), but will try others if needed. */
    if (devInfo.DevType == 3) {
        /* Try to send SMDI Master Identify command */
        debug_print("Sending SMDI Master Identify to device %d:%d...", ha_id, id);
        
        /* Send the SMDI Master Identify command */
        response = SMDI_MasterIdentify(ha_id, id);
        
        /* Check for SMDI Slave Identify response (0x00010001) */
        if (response == SMDIM_SLAVEIDENTIFY) {
            debug_print("Device is SMDI capable (SlaveIdentify response: 0x%08lX)", response);
            devInfo.bSMDI = TRUE;
        } else {
            debug_print("Device is not SMDI capable (response: 0x%08lX)", response);
        }
    }
    
    /* Copy back to caller's structure */
    memcpy(info, &devInfo, sizeof(SCSI_DevInfo));
}
