/*
 * SCSI Debugging structure and functions
 * For IRIX 5.3 SCSI ASPI implementation
 */

#ifndef __SCSI_DEBUG_H__
#define __SCSI_DEBUG_H__

#include <sys/types.h>

/* Maximum data size for debug capture */
#define SCSI_DEBUG_MAX_DATA 8192

/* SCSI packet direction */
typedef enum {
    SCSI_DIR_NONE = 0,
    SCSI_DIR_IN,
    SCSI_DIR_OUT
} scsi_direction_t;

/* SCSI packet debug info */
typedef struct {
    unsigned char   cmd[12];           /* SCSI command */
    unsigned char   cmd_len;           /* Command length */
    scsi_direction_t direction;        /* Data direction */
    unsigned long   data_len;          /* Actual data length */
    unsigned char   data[SCSI_DEBUG_MAX_DATA]; /* Data buffer */
    unsigned char   sense_data[32];    /* Sense data if available */
    unsigned char   sense_len;         /* Sense data length */
    unsigned char   status;            /* Completion status */
    unsigned char   ha_id;             /* Host adapter ID */
    unsigned char   target_id;         /* Target ID */
    unsigned long   timestamp;         /* Timestamp for packet */
    int             result;            /* Result code */
} scsi_debug_packet_t;

/* SCSI debug context */
typedef struct {
    int             enabled;           /* Debug enabled flag */
    char            logfile[256];      /* Log file name */
    int             log_to_file;       /* Whether to log to file */
    void            (*log_callback)(scsi_debug_packet_t *packet); /* Optional callback */
} scsi_debug_t;

/* Initialize debug structure */
void scsi_debug_init(scsi_debug_t *debug);

/* Log a SCSI packet */
void scsi_debug_log(scsi_debug_t *debug, scsi_debug_packet_t *packet);

/* Dump a packet in hex format */
void scsi_debug_dump_packet(scsi_debug_packet_t *packet);

/* Set log file */
int scsi_debug_set_logfile(scsi_debug_t *debug, const char *filename);

#endif /* __SCSI_DEBUG_H__ */
