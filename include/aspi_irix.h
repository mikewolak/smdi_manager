/*
 * ASPI interface for IRIX 5.3
 * Based on OpenSMDI by Christian Nowak
 */

#ifndef __ASPI_IRIX_H__
#define __ASPI_IRIX_H__

#include "scsi_debug.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Type definitions */
#ifndef BYTE
#define BYTE unsigned char
#endif
#ifndef WORD
#define WORD unsigned short
#endif
#ifndef DWORD
#define DWORD unsigned long
#endif
#ifndef BOOL
#define BOOL int
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef LPVOID
#define LPVOID void*
#endif
#ifndef MAX_PATH
#define MAX_PATH 260
#endif

/* ASPI function declarations - fixed return types */
int ASPI_Check(scsi_debug_t *debug);
void ASPI_RescanPort(scsi_debug_t *debug, unsigned char ha_id);
int ASPI_GetDevType(scsi_debug_t *debug, unsigned char ha_id, unsigned char id);
int ASPI_TestUnitReady(scsi_debug_t *debug, unsigned char ha_id, unsigned char id);
int ASPI_Send(scsi_debug_t *debug, unsigned char ha_id, unsigned char id, void *buffer, unsigned long size);
unsigned long ASPI_Receive(scsi_debug_t *debug, unsigned char ha_id, unsigned char id, void *buffer, unsigned long size);
void ASPI_InquireDevice(scsi_debug_t *debug, char result[], unsigned char ha_id, unsigned char id);

#ifdef __cplusplus
}
#endif

#endif /* __ASPI_IRIX_H__ */
