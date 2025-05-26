/*
 * SMDI sample handling implementation for IRIX 5.3
 * ANSI C90 compliant for MIPS big-endian architecture
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "smdi.h"
#include "smdi_sample.h"

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

/* Create a new sample */
SMDI_Sample* SMDI_CreateSample(DWORD sample_rate, BYTE bits_per_sample, 
                             BYTE channels, DWORD sample_count) {
    SMDI_Sample* sample;
    DWORD data_size;
    
    /* Verify parameters */
    if (sample_rate == 0 || sample_count == 0 || channels == 0 || 
        (bits_per_sample != 8 && bits_per_sample != 16)) {
        return NULL;
    }
    
    /* Allocate the sample structure */
    sample = (SMDI_Sample*)malloc(sizeof(SMDI_Sample));
    if (sample == NULL) {
        return NULL;
    }
    
    /* Initialize the structure */
    sample->sample_rate = sample_rate;
    sample->bits_per_sample = bits_per_sample;
    sample->channels = channels;
    sample->loop_type = SAMPLE_LOOP_NONE;
    sample->reserved = 0;
    sample->sample_count = sample_count;
    sample->loop_start = 0;
    sample->loop_end = 0;
    sample->root_note = 60;  /* Middle C */
    sample->fine_tune = 0;
    sample->name[0] = '\0';  /* Empty name */
    
    /* Calculate data size in bytes */
    if (bits_per_sample == 16) {
        data_size = sample_count * channels * sizeof(short);
    } else {
        data_size = sample_count * channels;
    }
    
    /* Allocate sample data */
    sample->sample_data = (short*)malloc(data_size);
    if (sample->sample_data == NULL) {
        free(sample);
        return NULL;
    }
    
    /* Clear the sample data */
    memset(sample->sample_data, 0, data_size);
    
    /* Store the data size */
    sample->data_size = data_size;
    
    return sample;
}

/* Free a sample */
void SMDI_FreeSample(SMDI_Sample* sample) {
    if (sample == NULL) {
        return;
    }
    
    /* Free the sample data */
    if (sample->sample_data != NULL) {
        free(sample->sample_data);
    }
    
    /* Free the sample structure */
    free(sample);
}

/* Convert SMDI sample header to our sample structure */
SMDI_Sample* SMDI_HeaderToSample(SMDI_SampleHeader* header, void* data) {
    SMDI_Sample* sample;
    DWORD data_size;
    
    /* Verify parameters */
    if (header == NULL || data == NULL) {
        return NULL;
    }
    
    /* Check if the sample exists */
    if (!header->bDoesExist) {
        return NULL;
    }
    
    /* Create a new sample */
    sample = SMDI_CreateSample(
        1000000000 / header->dwPeriod,    /* Convert period to rate */
        header->BitsPerWord,
        header->NumberOfChannels,
        header->dwLength);
    
    if (sample == NULL) {
        return NULL;
    }
    
    /* Copy the sample properties */
    sample->loop_type = header->LoopControl;
    sample->loop_start = header->dwLoopStart;
    sample->loop_end = header->dwLoopEnd;
    sample->root_note = header->wPitch;
    sample->fine_tune = header->wPitchFraction;
    strncpy(sample->name, header->cName, 255);
    sample->name[255] = '\0';
    
    /* Copy the sample data */
    data_size = (header->dwLength * header->NumberOfChannels * header->BitsPerWord) / 8;
    memcpy(sample->sample_data, data, data_size);
    
    /* Store the data size */
    sample->data_size = data_size;
    
    return sample;
}

/* Convert our sample structure to SMDI sample header */
void SMDI_SampleToHeader(SMDI_Sample* sample, SMDI_SampleHeader* header) {
    /* Verify parameters */
    if (sample == NULL || header == NULL) {
        return;
    }
    
    /* Initialize the header structure */
    memset(header, 0, sizeof(SMDI_SampleHeader));
    
    /* Set the structure size */
    header->dwStructSize = sizeof(SMDI_SampleHeader);
    
    /* Copy the sample properties */
    header->bDoesExist = TRUE;
    header->BitsPerWord = sample->bits_per_sample;
    header->NumberOfChannels = sample->channels;
    header->LoopControl = sample->loop_type;
    header->dwPeriod = 1000000000 / sample->sample_rate;  /* Convert rate to period */
    header->dwLength = sample->sample_count;
    header->dwLoopStart = sample->loop_start;
    header->dwLoopEnd = sample->loop_end;
    header->wPitch = sample->root_note;
    header->wPitchFraction = sample->fine_tune;
    
    /* Copy the name */
    header->NameLength = (BYTE)strlen(sample->name);
    if (header->NameLength > 255) {
        header->NameLength = 255;
    }
    strncpy(header->cName, sample->name, header->NameLength);
    header->cName[header->NameLength] = '\0';
}

/* Save a sample to a file */
BOOL SMDI_SaveSample(SMDI_Sample* sample, const char* filename) {
    FILE* file;
    NativeSampleHeader header;
    size_t name_len;
    
    /* Verify parameters */
    if (sample == NULL || filename == NULL) {
        return FALSE;
    }
    
    /* Open the file */
    file = fopen(filename, "wb");
    if (file == NULL) {
        return FALSE;
    }
    
    /* Create the file header */
    memcpy(header.signature, "SDMP", 4);
    header.version = 1;
    header.bitsPerSample = sample->bits_per_sample;
    header.channels = sample->channels;
    header.loopType = sample->loop_type;
    header.reserved = 0;
    header.sampleRate = sample->sample_rate;
    header.sampleCount = sample->sample_count;
    header.loopStart = sample->loop_start;
    header.loopEnd = sample->loop_end;
    header.pitch = sample->root_note;
    header.pitchFraction = sample->fine_tune;
    
    /* Get the name length */
    name_len = strlen(sample->name);
    if (name_len > 255) {
        name_len = 255;
    }
    header.nameLength = name_len;
    
    /* Write the header */
    if (fwrite(&header, sizeof(NativeSampleHeader), 1, file) != 1) {
        fclose(file);
        return FALSE;
    }
    
    /* Write the name */
    if (name_len > 0) {
        if (fwrite(sample->name, 1, name_len, file) != name_len) {
            fclose(file);
            return FALSE;
        }
    }
    
    /* Write the sample data */
    if (fwrite(sample->sample_data, 1, sample->data_size, file) != sample->data_size) {
        fclose(file);
        return FALSE;
    }
    
    /* Close the file */
    fclose(file);
    
    return TRUE;
}

/* Load a sample from a file */
SMDI_Sample* SMDI_LoadSample(const char* filename) {
    FILE* file;
    NativeSampleHeader header;
    SMDI_Sample* sample;
    char* name_buffer;
    DWORD data_size;
    
    /* Verify parameters */
    if (filename == NULL) {
        return NULL;
    }
    
    /* Open the file */
    file = fopen(filename, "rb");
    if (file == NULL) {
        return NULL;
    }
    
    /* Read the header */
    if (fread(&header, sizeof(NativeSampleHeader), 1, file) != 1) {
        fclose(file);
        return NULL;
    }
    
    /* Verify the signature */
    if (memcmp(header.signature, "SDMP", 4) != 0) {
        fclose(file);
        return NULL;
    }
    
    /* Create a new sample */
    sample = SMDI_CreateSample(
        header.sampleRate,
        header.bitsPerSample,
        header.channels,
        header.sampleCount);
    
    if (sample == NULL) {
        fclose(file);
        return NULL;
    }
    
    /* Copy the sample properties */
    sample->loop_type = header.loopType;
    sample->loop_start = header.loopStart;
    sample->loop_end = header.loopEnd;
    sample->root_note = header.pitch;
    sample->fine_tune = header.pitchFraction;
    
    /* Read the name */
    if (header.nameLength > 0) {
        /* Allocate a buffer for the name */
        name_buffer = (char*)malloc(header.nameLength + 1);
        if (name_buffer == NULL) {
            SMDI_FreeSample(sample);
            fclose(file);
            return NULL;
        }
        
        /* Read the name */
        if (fread(name_buffer, 1, header.nameLength, file) != header.nameLength) {
            free(name_buffer);
            SMDI_FreeSample(sample);
            fclose(file);
            return NULL;
        }
        
        /* Null-terminate the name */
        name_buffer[header.nameLength] = '\0';
        
        /* Copy the name to the sample */
        strncpy(sample->name, name_buffer, 255);
        sample->name[255] = '\0';
        
        /* Free the name buffer */
        free(name_buffer);
    }
    
    /* Read the sample data */
    if (fread(sample->sample_data, 1, sample->data_size, file) != sample->data_size) {
        SMDI_FreeSample(sample);
        fclose(file);
        return NULL;
    }
    
    /* Close the file */
    fclose(file);
    
    return sample;
}
