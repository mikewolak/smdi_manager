/*
 * SCSI Debugging implementation
 * For IRIX 5.3 SCSI ASPI implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "scsi_debug.h"

/* Initialize debug structure */
void scsi_debug_init(scsi_debug_t *debug)
{
    if (debug == NULL)
    {
        return;
    }
    
    memset(debug, 0, sizeof(scsi_debug_t));
    debug->enabled = 0;
    debug->log_to_file = 0;
    debug->logfile[0] = '\0';
    debug->log_callback = NULL;
}

/* Log a SCSI packet */
void scsi_debug_log(scsi_debug_t *debug, scsi_debug_packet_t *packet)
{
    FILE *fp;
    int i;
    int j;
    unsigned long bytes_to_print;
    unsigned char c;
    
    if (debug == NULL || packet == NULL || !debug->enabled)
    {
        return;
    }

    /* Call user callback if provided */
    if (debug->log_callback != NULL)
    {
        debug->log_callback(packet);
    }
    
    /* Log to file if enabled */
    if (debug->log_to_file && debug->logfile[0] != '\0')
    {
        fp = fopen(debug->logfile, "a");
        if (fp != NULL)
        {
            fprintf(fp, "=== SCSI Command at %lu ===\n", packet->timestamp);
            fprintf(fp, "Host Adapter: %d, Target: %d\n", 
                    packet->ha_id, packet->target_id);
            fprintf(fp, "Command: ");
            
            /* Print command bytes */
            for (i = 0; i < packet->cmd_len; i++)
            {
                fprintf(fp, "%02X ", packet->cmd[i]);
            }
            fprintf(fp, "\n");
            
            /* Print direction and length */
            if (packet->direction == SCSI_DIR_IN)
            {
                fprintf(fp, "Direction: IN, Length: %lu\n", packet->data_len);
            }
            else if (packet->direction == SCSI_DIR_OUT)
            {
                fprintf(fp, "Direction: OUT, Length: %lu\n", packet->data_len);
            }
            else
            {
                fprintf(fp, "Direction: NONE\n");
            }
            
            /* Print data if any */
            if (packet->data_len > 0)
            {
                fprintf(fp, "Data:\n");
                
                /* Limit data to print */
                bytes_to_print = packet->data_len;
                if (bytes_to_print > SCSI_DEBUG_MAX_DATA)
                {
                    bytes_to_print = SCSI_DEBUG_MAX_DATA;
                }
                
                /* Print data in hex format, 16 bytes per line */
                for (i = 0; i < bytes_to_print; i += 16)
                {
                    fprintf(fp, "%04X: ", i);
                    
                    /* Print hex values */
                    for (j = 0; j < 16 && (i + j) < bytes_to_print; j++)
                    {
                        fprintf(fp, "%02X ", packet->data[i + j]);
                    }
                    
                    /* Padding for incomplete lines */
                    for (; j < 16; j++)
                    {
                        fprintf(fp, "   ");
                    }
                    
                    /* Print ASCII representation */
                    fprintf(fp, " | ");
                    for (j = 0; j < 16 && (i + j) < bytes_to_print; j++)
                    {
                        c = packet->data[i + j];
                        if (c >= 32 && c <= 126)
                        {
                            fprintf(fp, "%c", c);
                        }
                        else
                        {
                            fprintf(fp, ".");
                        }
                    }
                    fprintf(fp, "\n");
                }
                
                if (bytes_to_print < packet->data_len)
                {
                    fprintf(fp, "... (%lu more bytes)\n", 
                            packet->data_len - bytes_to_print);
                }
            }
            
            /* Print status and result */
            fprintf(fp, "Status: %02X, Result: %d\n", packet->status, packet->result);
            
            /* Print sense data if available */
            if (packet->sense_len > 0)
            {
                fprintf(fp, "Sense Data: ");
                for (i = 0; i < packet->sense_len; i++)
                {
                    fprintf(fp, "%02X ", packet->sense_data[i]);
                }
                fprintf(fp, "\n");
            }
            
            fprintf(fp, "\n");
            fclose(fp);
        }
    }
}

/* Dump a packet in hex format to stdout */
void scsi_debug_dump_packet(scsi_debug_packet_t *packet)
{
    int i;
    int j;
    unsigned long bytes_to_print;
    unsigned char c;
    
    if (packet == NULL)
    {
        return;
    }
    
    printf("=== SCSI Command at %lu ===\n", packet->timestamp);
    printf("Host Adapter: %d, Target: %d\n", packet->ha_id, packet->target_id);
    printf("Command: ");
    
    /* Print command bytes */
    for (i = 0; i < packet->cmd_len; i++)
    {
        printf("%02X ", packet->cmd[i]);
    }
    printf("\n");
    
    /* Print direction and length */
    if (packet->direction == SCSI_DIR_IN)
    {
        printf("Direction: IN, Length: %lu\n", packet->data_len);
    }
    else if (packet->direction == SCSI_DIR_OUT)
    {
        printf("Direction: OUT, Length: %lu\n", packet->data_len);
    }
    else
    {
        printf("Direction: NONE\n");
    }
    
    /* Print data if any */
    if (packet->data_len > 0)
    {
        printf("Data:\n");
        
        /* Limit data to print */
        bytes_to_print = packet->data_len;
        if (bytes_to_print > SCSI_DEBUG_MAX_DATA)
        {
            bytes_to_print = SCSI_DEBUG_MAX_DATA;
        }
        
        /* Print data in hex format, 16 bytes per line */
        for (i = 0; i < bytes_to_print; i += 16)
        {
            printf("%04X: ", i);
            
            /* Print hex values */
            for (j = 0; j < 16 && (i + j) < bytes_to_print; j++)
            {
                printf("%02X ", packet->data[i + j]);
            }
            
            /* Padding for incomplete lines */
            for (; j < 16; j++)
            {
                printf("   ");
            }
            
            /* Print ASCII representation */
            printf(" | ");
            for (j = 0; j < 16 && (i + j) < bytes_to_print; j++)
            {
                c = packet->data[i + j];
                if (c >= 32 && c <= 126)
                {
                    printf("%c", c);
                }
                else
                {
                    printf(".");
                }
            }
            printf("\n");
        }
        
        if (bytes_to_print < packet->data_len)
        {
            printf("... (%lu more bytes)\n", 
                    packet->data_len - bytes_to_print);
        }
    }
    
    /* Print status and result */
    printf("Status: %02X, Result: %d\n", packet->status, packet->result);
    
    /* Print sense data if available */
    if (packet->sense_len > 0)
    {
        printf("Sense Data: ");
        for (i = 0; i < packet->sense_len; i++)
        {
            printf("%02X ", packet->sense_data[i]);
        }
        printf("\n");
    }
    
    printf("\n");
}

/* Set log file */
int scsi_debug_set_logfile(scsi_debug_t *debug, const char *filename)
{
    FILE *fp;
    
    if (debug == NULL || filename == NULL)
    {
        return 0;
    }
    
    /* Try to open the file to make sure it's writable */
    fp = fopen(filename, "a");
    if (fp == NULL)
    {
        return 0;
    }
    
    fclose(fp);
    
    /* Set the log file */
    strncpy(debug->logfile, filename, sizeof(debug->logfile) - 1);
    debug->logfile[sizeof(debug->logfile) - 1] = '\0';
    debug->log_to_file = 1;
    
    return 1;
}
