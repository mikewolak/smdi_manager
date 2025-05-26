/*
 * SMDI sample handling for IRIX 5.3
 * ANSI C90 compliant implementation for MIPS big-endian architecture
 */

#ifndef _SMDI_SAMPLE_H
#define _SMDI_SAMPLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "smdi.h"

/* Native sample file signature */
#define SAMPLE_FILE_SIGNATURE "SDMP"

/* Loop types */
#define SAMPLE_LOOP_NONE          0
#define SAMPLE_LOOP_FORWARD       1
#define SAMPLE_LOOP_BIDIRECTIONAL 2

/* Sample structure - contains interleaved audio data */
typedef struct {
    DWORD sample_rate;         /* In Hz */
    BYTE  bits_per_sample;     /* Typically 16 */
    BYTE  channels;            /* 1=mono, 2=stereo */
    BYTE  loop_type;           /* 0=none, 1=forward, 2=bidirectional */
    BYTE  reserved;
    DWORD sample_count;        /* Per channel */
    DWORD loop_start;          /* Sample point */
    DWORD loop_end;            /* Sample point */
    WORD  root_note;           /* MIDI note number (0-127, 60=middle C) */
    WORD  fine_tune;           /* Cents, -50 to +50 */
    char  name[256];           /* Sample name */
    short* sample_data;        /* Interleaved if stereo */
    DWORD data_size;           /* Size in bytes */
} SMDI_Sample;

/* Create a new sample */
SMDI_Sample* SMDI_CreateSample(DWORD sample_rate, BYTE bits_per_sample, 
                             BYTE channels, DWORD sample_count);

/* Free a sample */
void SMDI_FreeSample(SMDI_Sample* sample);

/* Convert SMDI sample header to our sample structure */
SMDI_Sample* SMDI_HeaderToSample(SMDI_SampleHeader* header, void* data);

/* Convert our sample structure to SMDI sample header */
void SMDI_SampleToHeader(SMDI_Sample* sample, SMDI_SampleHeader* header);

/* Save a sample to a file */
BOOL SMDI_SaveSample(SMDI_Sample* sample, const char* filename);

/* Load a sample from a file */
SMDI_Sample* SMDI_LoadSample(const char* filename);

#ifdef __cplusplus
}
#endif

#endif /* _SMDI_SAMPLE_H */
