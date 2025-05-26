/*
 * ASPI interface for IRIX 5.3
 * Based on OpenSMDI by Christian Nowak
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <errno.h>

/* Use dslib for SCSI access */
#include <dslib.h>

#include "aspi_irix.h"
#include "scsi_debug.h"
#include "aspi_defs.h"

/*
 * Get device path string for a given host adapter and target ID
 */

static void ASPI_GetDevNameByID(char cResult[], unsigned char ha_id, unsigned char id)
{
    /* Format confirmed working by inquire command: /dev/scsi/sc0d5l0 */
    sprintf(cResult, "/dev/scsi/sc%dd%dl0", ha_id, id);
}

/*
 * Populate a debug packet with command information
 */

static void fill_debug_packet(scsi_debug_t *debug, 
                             scsi_debug_packet_t *packet,
                             unsigned char ha_id, 
                             unsigned char id,
                             unsigned char *cmd, 
                             unsigned char cmd_len,
                             void *data, 
                             unsigned long data_len,
                             scsi_direction_t direction)
{
    struct dsreq *dsp;
    
    if (debug == NULL || !debug->enabled || packet == NULL)
    {
        return;
    }
    
    memset(packet, 0, sizeof(scsi_debug_packet_t));
    packet->ha_id = ha_id;
    packet->target_id = id;
    packet->timestamp = (unsigned long)time(NULL);
    packet->direction = direction;
    
    /* Copy command */
    if (cmd != NULL && cmd_len > 0)
    {
        memcpy(packet->cmd, cmd, cmd_len);
        packet->cmd_len = cmd_len;
    }
    
    /* Copy data */
    if (data != NULL && data_len > 0)
    {
        unsigned long copy_len = data_len;
        if (copy_len > SCSI_DEBUG_MAX_DATA)
        {
            copy_len = SCSI_DEBUG_MAX_DATA;
        }
        memcpy(packet->data, data, copy_len);
        packet->data_len = data_len;
    }
    
    /* If we have a dsreq structure, get sense data and status */
    dsp = (struct dsreq *)data;
    if (direction == SCSI_DIR_NONE && dsp != NULL)
    {
        packet->status = dsp->ds_status;
        packet->result = dsp->ds_ret;
        
        if (dsp->ds_sensesent > 0 && dsp->ds_sensebuf != NULL)
        {
            unsigned char copy_len = dsp->ds_sensesent;
            if (copy_len > sizeof(packet->sense_data))
            {
                copy_len = sizeof(packet->sense_data);
            }
            memcpy(packet->sense_data, dsp->ds_sensebuf, copy_len);
            packet->sense_len = copy_len;
        }
    }
}

/*
 * Check if ASPI is available
 */
int ASPI_Check(scsi_debug_t *debug)
{
    char dev_path[MAX_PATH];
    int fd;
    int i;
    scsi_debug_packet_t packet;
    
    /* Check if debug is enabled and initialize packet */
    if (debug != NULL && debug->enabled)
    {
        memset(&packet, 0, sizeof(packet));
        packet.timestamp = (unsigned long)time(NULL);
        packet.direction = SCSI_DIR_NONE;
    }
    
    /* Try to open known device that works */
    sprintf(dev_path, "/dev/scsi/sc0d5l0");
    fd = open(dev_path, O_RDONLY);
    
    if (fd >= 0)
    {
        close(fd);
        
        /* Log success if debug enabled */
        if (debug != NULL && debug->enabled)
        {
            packet.result = 1;
            strcpy((char*)packet.data, "ASPI available");
            packet.data_len = strlen((char*)packet.data);
            scsi_debug_log(debug, &packet);
        }
        
        return 1;
    }
    
    /* Try host adapter 0 with different target IDs */
    for (i = 0; i < 8; i++)
    {
        if (i == 7) continue;  /* Skip host adapter ID */
        
        sprintf(dev_path, "/dev/scsi/sc0d%dl0", i);
        fd = open(dev_path, O_RDONLY);
        
        if (fd >= 0)
        {
            close(fd);
            
            /* Log success if debug enabled */
            if (debug != NULL && debug->enabled)
            {
                packet.result = 1;
                strcpy((char*)packet.data, "ASPI available");
                packet.data_len = strlen((char*)packet.data);
                scsi_debug_log(debug, &packet);
            }
            
            return 1;
        }
    }
    
    /* Log failure if debug enabled */
    if (debug != NULL && debug->enabled)
    {
        packet.result = 0;
        strcpy((char*)packet.data, "ASPI not available");
        packet.data_len = strlen((char*)packet.data);
        scsi_debug_log(debug, &packet);
    }
    
    return 0;
}

/*
 * Rescan SCSI bus (on IRIX this is a no-op)
 */
void ASPI_RescanPort(scsi_debug_t *debug, unsigned char ha_id)
{
    scsi_debug_packet_t packet;
    
    /* If debug enabled, log that this is a no-op on IRIX */
    if (debug != NULL && debug->enabled)
    {
        memset(&packet, 0, sizeof(packet));
        packet.timestamp = (unsigned long)time(NULL);
        packet.direction = SCSI_DIR_NONE;
        packet.ha_id = ha_id;
        strcpy((char*)packet.data, "RescanPort is a no-op on IRIX");
        packet.data_len = strlen((char*)packet.data);
        scsi_debug_log(debug, &packet);
    }
}

/*
 * Get SCSI device type
 */
int ASPI_GetDevType(scsi_debug_t *debug, unsigned char ha_id, unsigned char id)
{
    unsigned char cDevType;
    char dev_path[MAX_PATH];
    struct dsreq *dsp;
    unsigned char inqbuf[36];  /* Standard inquiry data */
    scsi_debug_packet_t packet;
    
    /* Get device path and open device */
    ASPI_GetDevNameByID(dev_path, ha_id, id);
    dsp = dsopen(dev_path, O_RDONLY);
    
    if (dsp == NULL)
    {
        /* Log failure if debug enabled */
        if (debug != NULL && debug->enabled)
        {
            memset(&packet, 0, sizeof(packet));
            packet.timestamp = (unsigned long)time(NULL);
            packet.direction = SCSI_DIR_NONE;
            packet.ha_id = ha_id;
            packet.target_id = id;
            strcpy((char*)packet.data, "Failed to open device");
            packet.data_len = strlen((char*)packet.data);
            packet.result = -1;
            scsi_debug_log(debug, &packet);
        }
        return 0xFF;  /* Error code */
    }
    
    /* Perform INQUIRY command */
    memset(inqbuf, 0, sizeof(inqbuf));
    
    if (inquiry12(dsp, inqbuf, sizeof(inqbuf), 0) != 0)
    {
        /* Log failure if debug enabled */
        if (debug != NULL && debug->enabled)
        {
            memset(&packet, 0, sizeof(packet));
            packet.timestamp = (unsigned long)time(NULL);
            packet.direction = SCSI_DIR_IN;
            packet.ha_id = ha_id;
            packet.target_id = id;
            strcpy((char*)packet.data, "Inquiry command failed");
            packet.data_len = strlen((char*)packet.data);
            packet.result = dsp->ds_ret;
            packet.status = dsp->ds_status;
            memcpy(packet.cmd, "INQUIRY", 7);
            packet.cmd_len = 7;
            scsi_debug_log(debug, &packet);
        }
        
        dsclose(dsp);
        return 0xFF;  /* Error code */
    }
    
    /* Get device type from inquiry data */
    cDevType = inqbuf[0] & 0x1F;
    dsclose(dsp);
    
    /* Log success if debug enabled */
    if (debug != NULL && debug->enabled)
    {
        memset(&packet, 0, sizeof(packet));
        packet.timestamp = (unsigned long)time(NULL);
        packet.direction = SCSI_DIR_IN;
        packet.ha_id = ha_id;
        packet.target_id = id;
        memcpy(packet.cmd, "INQUIRY", 7);
        packet.cmd_len = 7;
        memcpy(packet.data, inqbuf, sizeof(inqbuf));
        packet.data_len = sizeof(inqbuf);
        packet.result = 0;
        packet.status = 0;
        scsi_debug_log(debug, &packet);
    }
    
    return cDevType;
}

/*
 * Test if SCSI unit is ready
 */
int ASPI_TestUnitReady(scsi_debug_t *debug, unsigned char ha_id, unsigned char id)
{
    int result;
    struct dsreq *dsp;
    char dev_path[MAX_PATH];
    scsi_debug_packet_t packet;
    
    /* Get device path and open device */
    ASPI_GetDevNameByID(dev_path, ha_id, id);
    dsp = dsopen(dev_path, O_RDONLY);
    
    if (dsp == NULL)
    {
        /* Log failure if debug enabled */
        if (debug != NULL && debug->enabled)
        {
            memset(&packet, 0, sizeof(packet));
            packet.timestamp = (unsigned long)time(NULL);
            packet.direction = SCSI_DIR_NONE;
            packet.ha_id = ha_id;
            packet.target_id = id;
            strcpy((char*)packet.data, "Failed to open device");
            packet.data_len = strlen((char*)packet.data);
            packet.result = -1;
            scsi_debug_log(debug, &packet);
        }
        return FALSE;
    }
    
    /* Issue Test Unit Ready command */
    result = testunitready00(dsp);
    
    /* Log result if debug enabled */
    if (debug != NULL && debug->enabled)
    {
        unsigned char cmd[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};  /* TEST UNIT READY */
        
        memset(&packet, 0, sizeof(packet));
        packet.timestamp = (unsigned long)time(NULL);
        packet.direction = SCSI_DIR_NONE;
        packet.ha_id = ha_id;
        packet.target_id = id;
        memcpy(packet.cmd, cmd, 6);
        packet.cmd_len = 6;
        packet.result = dsp->ds_ret;
        packet.status = dsp->ds_status;
        
        /* Get sense data if available */
        if (dsp->ds_sensesent > 0 && dsp->ds_sensebuf != NULL)
        {
            unsigned char sense_len = dsp->ds_sensesent;
            if (sense_len > sizeof(packet.sense_data))
            {
                sense_len = sizeof(packet.sense_data);
            }
            memcpy(packet.sense_data, dsp->ds_sensebuf, sense_len);
            packet.sense_len = sense_len;
        }
        
        scsi_debug_log(debug, &packet);
    }
    
    dsclose(dsp);
    return (result == 0) ? TRUE : FALSE;
}

/*
 * Send data to SCSI device
 */

BOOL ASPI_Send(scsi_debug_t *debug, unsigned char ha_id, unsigned char id, void *buffer, unsigned long size)
{
    struct dsreq *dsp;
    char dev_path[MAX_PATH];
    unsigned char cmd[6];
    int result;
    
    /* Get device path and open it using dslib */
    ASPI_GetDevNameByID(dev_path, ha_id, id);
    dsp = dsopen(dev_path, O_RDWR);
    
    if (dsp == NULL) {
        if (debug != NULL && debug->enabled) {
            printf("ASPI_Send: Failed to open device %s\n", dev_path);
        }
        return FALSE;
    }
    
    /* Set up command exactly like in Linux */
    cmd[0] = 0x0A;  /* WRITE command */
    cmd[1] = 0x00;
    cmd[2] = (unsigned char)((size & 0x00FF0000) >> 16);  /* Size bytes exactly as Linux */
    cmd[3] = (unsigned char)((size & 0x0000FF00) >> 8);
    cmd[4] = (unsigned char)(size & 0x000000FF);
    cmd[5] = 0x00;
    
    /* Copy command to dsp command buffer */
    memcpy(CMDBUF(dsp), cmd, 6);
    CMDLEN(dsp) = 6;
    
    /* Set up data buffer */
    DATABUF(dsp) = (caddr_t)buffer;
    DATALEN(dsp) = size;
    
    /* Set flags for write operation with sense data */
    dsp->ds_flags = DSRQ_WRITE | DSRQ_SENSE;
    dsp->ds_time = 30 * 1000;  /* 30 second timeout */
    
    /* Execute command */
    result = doscsireq(getfd(dsp), dsp);
    
    /* Check result */
    if (result != 0) {
        if (debug != NULL && debug->enabled) {
            printf("ASPI_Send: Command failed, result=%d, ds_ret=%d, status=%d\n", 
                   result, dsp->ds_ret, dsp->ds_status);
        }
        dsclose(dsp);
        return FALSE;
    }
    
    dsclose(dsp);
    return TRUE;
}

/*
 * Receive data from SCSI device
 */

unsigned long ASPI_Receive(scsi_debug_t *debug, unsigned char ha_id, unsigned char id, void *buffer, unsigned long size)
{
    int fd;
    char dev_path[MAX_PATH];
    struct dsreq ds_req;
    unsigned char cmd[6];
    int result;
    unsigned long bytes_received = 0;
    
    /* Get device path and open it directly */
    ASPI_GetDevNameByID(dev_path, ha_id, id);
    fd = open(dev_path, O_RDWR);
    
    if (fd < 0) {
        if (debug != NULL && debug->enabled) {
            printf("ASPI_Receive: Failed to open device %s, errno=%d\n", dev_path, errno);
        }
        return 0;
    }
    
    /* Initialize the dsreq structure */
    memset(&ds_req, 0, sizeof(ds_req));
    
    /* Prepare the command buffer similar to the Linux implementation */
    cmd[0] = 0x08;  /* READ */
    cmd[1] = 0x00;
    cmd[2] = (unsigned char)((size & 0x00FF0000) >> 16);
    cmd[3] = (unsigned char)((size & 0x0000FF00) >> 8);
    cmd[4] = (unsigned char)(size & 0x000000FF);
    cmd[5] = 0x00;
    
    /* Set up the dsreq structure */
    ds_req.ds_cmdbuf = (caddr_t)cmd;
    ds_req.ds_cmdlen = 6;
    ds_req.ds_databuf = (caddr_t)buffer;
    ds_req.ds_datalen = size;
    ds_req.ds_flags = DSRQ_READ | DSRQ_SENSE;
    ds_req.ds_time = 10 * 1000;  /* 10 second timeout */
    
    /* Execute the SCSI command */
    result = ioctl(fd, DS_ENTER, &ds_req);
    
    /* Get bytes received */
    if (result >= 0 && ds_req.ds_ret == 0) {
        bytes_received = ds_req.ds_datasent;
        
        if (debug != NULL && debug->enabled) {
            printf("ASPI_Receive: Received %lu bytes\n", bytes_received);
        }
    }
    else if (debug != NULL && debug->enabled) {
        printf("ASPI_Receive: ioctl failed, result=%d, ds_ret=%d\n", result, ds_req.ds_ret);
    }
    
    /* Close the device */
    close(fd);
    
    return bytes_received;
}


/*
 * Inquire SCSI device (get identity information)
 */
void ASPI_InquireDevice(scsi_debug_t *debug, char result[], unsigned char ha_id, unsigned char id)
{
    struct dsreq *dsp;
    char dev_path[MAX_PATH];
    unsigned char inqbuf[96];  /* Inquiry data buffer */
    scsi_debug_packet_t packet;
    
    /* Check parameters */
    if (result == NULL)
    {
        return;
    }
    
    /* Initialize result */
    memset(result, 0, 96);
    
    /* Get device path and open device */
    ASPI_GetDevNameByID(dev_path, ha_id, id);
    dsp = dsopen(dev_path, O_RDONLY);
    
    if (dsp == NULL)
    {
        /* Log failure if debug enabled */
        if (debug != NULL && debug->enabled)
        {
            memset(&packet, 0, sizeof(packet));
            packet.timestamp = (unsigned long)time(NULL);
            packet.direction = SCSI_DIR_IN;
            packet.ha_id = ha_id;
            packet.target_id = id;
            strcpy((char*)packet.data, "Failed to open device");
            packet.data_len = strlen((char*)packet.data);
            packet.result = -1;
            scsi_debug_log(debug, &packet);
        }
        return;
    }
    
    /* Perform INQUIRY command */
    memset(inqbuf, 0, sizeof(inqbuf));
    
    if (inquiry12(dsp, inqbuf, sizeof(inqbuf), 0) == 0)
    {
        /* Copy inquiry data to result buffer */
        memcpy(result, inqbuf, sizeof(inqbuf));
        
        /* Log success if debug enabled */
        if (debug != NULL && debug->enabled)
        {
            unsigned char cmd[6] = {0x12, 0, 0, 0, 96, 0};  /* INQUIRY */
            
            memset(&packet, 0, sizeof(packet));
            packet.timestamp = (unsigned long)time(NULL);
            packet.direction = SCSI_DIR_IN;
            packet.ha_id = ha_id;
            packet.target_id = id;
            memcpy(packet.cmd, cmd, 6);
            packet.cmd_len = 6;
            packet.result = dsp->ds_ret;
            packet.status = dsp->ds_status;
            
            /* Copy inquiry data to debug packet */
            memcpy(packet.data, inqbuf, sizeof(inqbuf));
            packet.data_len = sizeof(inqbuf);
            
            scsi_debug_log(debug, &packet);
        }
    }
    else
    {
        /* Log failure if debug enabled */
        if (debug != NULL && debug->enabled)
        {
            unsigned char cmd[6] = {0x12, 0, 0, 0, 96, 0};  /* INQUIRY */
            
            memset(&packet, 0, sizeof(packet));
            packet.timestamp = (unsigned long)time(NULL);
            packet.direction = SCSI_DIR_IN;
            packet.ha_id = ha_id;
            packet.target_id = id;
            memcpy(packet.cmd, cmd, 6);
            packet.cmd_len = 6;
            packet.result = dsp->ds_ret;
            packet.status = dsp->ds_status;
            
            /* Get sense data if available */
            if (dsp->ds_sensesent > 0 && dsp->ds_sensebuf != NULL)
            {
                unsigned char sense_len = dsp->ds_sensesent;
                if (sense_len > sizeof(packet.sense_data))
                {
                    sense_len = sizeof(packet.sense_data);
                }
                memcpy(packet.sense_data, dsp->ds_sensebuf, sense_len);
                packet.sense_len = sense_len;
            }
            
            scsi_debug_log(debug, &packet);
        }
    }
    
    dsclose(dsp);
}
