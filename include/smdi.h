/*
 * SMDI - SCSI Musical Data Interchange protocol for IRIX 5.3
 * ANSI C90 compliant implementation
 */

#ifndef _SMDI_H
#define _SMDI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

/* Type definitions for 32-bit IRIX 5.3 */
#ifndef BYTE
#define BYTE unsigned char       /* 8-bit */
#endif
#ifndef WORD
#define WORD unsigned short      /* 16-bit */
#endif
#ifndef DWORD
#define DWORD unsigned long      /* 32-bit */
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef MAX_PATH
#define MAX_PATH 1024            /* IRIX PATH_MAX is typically 1024 */
#endif
#ifndef BOOL
#define BOOL int
#endif

/* SMDI message ID's */
#define	SMDIM_ERROR                     0x00000000
#define	SMDIM_MASTERIDENTIFY            0x00010000
#define	SMDIM_SLAVEIDENTIFY             0x00010001
#define	SMDIM_MESSAGEREJECT             0x00020000
#define	SMDIM_ACK                       0x01000000
#define	SMDIM_NAK                       0x01010000
#define	SMDIM_WAIT                      0x01020000
#define	SMDIM_SENDNEXTPACKET            0x01030000
#define	SMDIM_ENDOFPROCEDURE            0x01040000
#define	SMDIM_ABORTPROCEDURE            0x01050000
#define	SMDIM_DATAPACKET                0x01100000
#define	SMDIM_SAMPLEHEADERREQUEST       0x01200000
#define	SMDIM_SAMPLEHEADER              0x01210000
#define	SMDIM_SAMPLENAME                0x01230000
#define	SMDIM_DELETESAMPLE              0x01240000
#define	SMDIM_BEGINSAMPLETRANSFER       0x01220000
#define	SMDIM_TRANSFERACKNOWLEDGE       0x01220001
#define	SMDIM_TRANSMITMIDIMESSAGE       0x02000000

/* SMDI Errors */
#define	SMDIE_OUTOFRANGE                0x00200000
#define	SMDIE_NOSAMPLE                  0x00200002
#define	SMDIE_NOMEMORY                  0x00200004
#define SMDIE_UNSUPPSAMBITS             0x00200006

/* Copy modes for SMDI_SendDataPacket - Not used in IRIX (big-endian) */
/* We keep the defines but don't actually use them */
#define	CM_NORMAL                       0x00000000 /* Does a 1:1 copy */

/* Sample format identifiers */
#define SF_NATIVE                       0x00000001 /* SMDI native sample format */

/* File errors */
#define FE_OPENERROR                    0x00010001 /* Couldn't open the file */
#define	FE_UNKNOWNFORMAT                0x00010002 /* Unsupported file format */

/* Default SMDI Packet Size */
#define PACKETSIZE 16384

/* Maximum number of samples to scan */
#define MAX_SAMPLES_TO_SCAN 64

/* SCSI device information structure */
typedef struct SCSI_DevInfo
{
  DWORD dwStructSize;                   /* (00) */
  BOOL bSMDI;                           /* (04) */
  BYTE DevType;                         /* (08) */
  BYTE Rsvd1;                           /* (09) */
  BYTE Rsvd2;                           /* (10) */
  BYTE Rsvd3;                           /* (11) */
  char cName[20];                       /* (12) */
  char cManufacturer[12];               /* (32) */
} SCSI_DevInfo;

/* SMDI sample header structure */
typedef struct SMDI_SampleHeader
{
  DWORD dwStructSize;                   /* (000) */
  BOOL bDoesExist;                      /* (004) */
  BYTE BitsPerWord;                     /* (008) */
  BYTE NumberOfChannels;                /* (009) */
  BYTE LoopControl;                     /* (010) - 0=none, 1=forward, 2=bidirectional */
  BYTE NameLength;                      /* (011) */
  DWORD dwPeriod;                       /* (012) - nanoseconds per sample */
  DWORD dwLength;                       /* (016) - in sample points (per channel) */
  DWORD dwLoopStart;                    /* (020) - sample point */
  DWORD dwLoopEnd;                      /* (024) - sample point */
  WORD wPitch;                          /* (028) - MIDI note number */
  WORD wPitchFraction;                  /* (030) - cents, -50 to +50 */
  char cName[256];                      /* (032) */
  DWORD dwDataOffset;                   /* (288) - for file operations only */
} SMDI_SampleHeader;

/* SMDI transmission information structure */
typedef struct SMDI_TransmissionInfo
{
  DWORD dwStructSize;                   /* (00) */
  SMDI_SampleHeader * lpSampleHeader;   /* (04) */
  DWORD dwTransmittedPackets;           /* (08) */
  DWORD dwPacketSize;                   /* (12) */
  DWORD dwSampleNumber;                 /* (16) */
  DWORD dwCopyMode;                     /* (20) */
  void * lpSampleData;                  /* (24) */
  BYTE SCSI_ID;                         /* (28) */
  BYTE HA_ID;                           /* (29) */
  BYTE Rsvd1;                           /* (30) */
  BYTE Rsvd2;                           /* (31) */
} SMDI_TransmissionInfo;

/* SMDI file transmission information structure */
typedef struct SMDI_FileTransmissionInfo
{
  DWORD dwStructSize;
  void (*lpCallBackProcedure)(struct SMDI_FileTransmissionInfo*, DWORD);
  SMDI_TransmissionInfo * lpTransmissionInfo;
  DWORD dwFileType;
  FILE * hFile;
  char cFileName[MAX_PATH];
  DWORD * lpReturnValue;
  DWORD dwUserData;
} SMDI_FileTransmissionInfo;

/* SMDI file transfer structure */
typedef struct SMDI_FileTransfer
{
  DWORD dwStructSize;
  BYTE HA_ID;
  BYTE rsvd1;
  BYTE rsvd2;
  BYTE rsvd3;
  BYTE SCSI_ID;
  BYTE rsvd4;
  BYTE rsvd5;
  BYTE rsvd6;
  DWORD dwSampleNumber;
  char * lpFileName;
  DWORD dwFileType;                    /* Only needed when receiving a file */
  char * lpSampleName;                 /* Only needed when sending a file */
  void* lpCallback;                    /* Callback function */
  DWORD dwUserData;
  BOOL bAsync;
  DWORD * lpReturnValue;
} SMDI_FileTransfer;

/* Core SMDI functions */
unsigned char SMDI_Init(void);
BOOL SMDI_TestUnitReady(BYTE HA_ID, BYTE SCSI_ID);
void SMDI_GetDeviceInfo(BYTE HA_ID, BYTE SCSI_ID, SCSI_DevInfo* info);

/* Debug functions */
void SMDI_SetDebugMode(int enable);
int SMDI_GetDebugMode(void);

/* File operations */
DWORD SMDI_SendFile(SMDI_FileTransfer* ft);
DWORD SMDI_ReceiveFile(SMDI_FileTransfer* ft);

/* Sample operations */
DWORD SMDI_DeleteSample(BYTE HA_ID, BYTE SCSI_ID, DWORD sample_number);
DWORD SMDI_SampleHeaderRequest(BYTE HA_ID, BYTE SCSI_ID, DWORD sample_number, SMDI_SampleHeader* sh);

/* Internal functions - not typically called directly by applications */
DWORD SMDI_SendDataPacket(BYTE ha_id, BYTE id, DWORD pn, void* data, DWORD length);
DWORD SMDI_SendBeginSampleTransfer(BYTE ha_id, BYTE id, DWORD sampleNum, void* packetLength);
DWORD SMDI_SendSampleHeader(BYTE ha_id, BYTE id, DWORD sampleNum, SMDI_SampleHeader* sh, DWORD* DataPacketLength);
DWORD SMDI_NextDataPacketRequest(BYTE ha_id, BYTE id, DWORD packetNumber, void* buffer, DWORD maxlen);
DWORD SMDI_MasterIdentify(BYTE ha_id, BYTE id);
DWORD SMDI_SampleName(BYTE ha_id, BYTE id, DWORD sampleNum, char sampleName[]);
DWORD SMDI_GetMessage(BYTE ha_id, BYTE id);
DWORD SMDI_GetLastError(void);

/* Sample transmission functions */
DWORD SMDI_InitSampleTransmission(SMDI_TransmissionInfo* lpTransmissionInfo);
DWORD SMDI_SampleTransmission(SMDI_TransmissionInfo* lpTransmissionInfo);
DWORD SMDI_InitSampleReception(SMDI_TransmissionInfo* tiATemp);
DWORD SMDI_SampleReception(SMDI_TransmissionInfo* lpTransmissionInfo);

/* File transmission functions */
DWORD SMDI_InitFileSampleTransmission(SMDI_FileTransmissionInfo* lpFileTransmissionInfo);
DWORD SMDI_FileSampleTransmission(SMDI_FileTransmissionInfo* lpFileTransmissionInfo);
DWORD SMDI_InitFileSampleReception(SMDI_FileTransmissionInfo* lpFileTransmissionInfo);
DWORD SMDI_FileSampleReception(SMDI_FileTransmissionInfo* lpFileTransmissionInfo);
DWORD SMDI_GetFileSampleHeader(char cFileName[], SMDI_SampleHeader* lpSampleHeader);

#ifdef __cplusplus
}
#endif

#endif /* _SMDI_H */
