/*
 * SMDI AIF file format support for IRIX 5.3
 * ANSI C90 compliant implementation for MIPS big-endian architecture
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dmedia/audioutil.h>
#include <dmedia/audiofile.h>
#include "smdi.h"
#include "smdi_sample.h"

/* Load an AIF file into SMDI sample format */
SMDI_Sample* SMDI_LoadAIFSample(const char* filename) {
    AFfilehandle file;
    SMDI_Sample* sample = NULL;
    long sampfmt, sampwidth;
    long nframes, channels, rate;
    int file_format;
    void* buffer;
    long ids[32]; /* Max 32 misc chunks */
    int nmisc, i;
    long size;
    char name_buffer[256];
    const char* basename;
    char* dot;
    
    /* Open the AIF file */
    file = AFopenfile(filename, "r", AF_NULL_FILESETUP);
    if (file == AF_NULL_FILEHANDLE) {
        fprintf(stderr, "SMDI_LoadAIFSample: Failed to open file '%s'\n", filename);
        return NULL;
    }
    
    /* Get file format - verify it's AIFF or AIFF-C */
    file_format = AFgetfilefmt(file, NULL);
    if (file_format != AF_FILE_AIFF && file_format != AF_FILE_AIFFC) {
        fprintf(stderr, "SMDI_LoadAIFSample: File '%s' is not AIFF or AIFF-C format\n", filename);
        AFclosefile(file);
        return NULL;
    }
    
    /* Get audio track information */
    nframes = AFgetframecnt(file, AF_DEFAULT_TRACK);
    channels = AFgetchannels(file, AF_DEFAULT_TRACK);
    rate = (long)AFgetrate(file, AF_DEFAULT_TRACK);
    AFgetsampfmt(file, AF_DEFAULT_TRACK, &sampfmt, &sampwidth);
    
    /* Only support 8-bit or 16-bit samples for now */
    if (sampfmt != AF_SAMPFMT_TWOSCOMP || (sampwidth != 8 && sampwidth != 16)) {
        fprintf(stderr, "SMDI_LoadAIFSample: Unsupported sample format in '%s'\n", filename);
        AFclosefile(file);
        return NULL;
    }
    
    /* Create a new sample */
    sample = SMDI_CreateSample(rate, sampwidth, channels, nframes);
    if (sample == NULL) {
        fprintf(stderr, "SMDI_LoadAIFSample: Failed to create sample\n");
        AFclosefile(file);
        return NULL;
    }
    
    /* Extract loop information if available */
    if (AFgetinstids(file, NULL) > 0) {
        long loop_id, loop_mode, loop_start_id, loop_end_id;
        long loop_start, loop_end;
        
        /* Get sustain loop information */
        loop_id = AFgetinstparamlong(file, AF_DEFAULT_INST, AF_INST_SUSLOOPID);
        if (loop_id > 0) {
            loop_mode = AFgetloopmode(file, AF_DEFAULT_INST, loop_id);
            loop_start_id = AFgetloopstart(file, AF_DEFAULT_INST, loop_id);
            loop_end_id = AFgetloopend(file, AF_DEFAULT_INST, loop_id);
            
            if (loop_start_id > 0 && loop_end_id > 0) {
                loop_start = AFgetmarkpos(file, AF_DEFAULT_TRACK, loop_start_id);
                loop_end = AFgetmarkpos(file, AF_DEFAULT_TRACK, loop_end_id);
                
                /* Set loop parameters */
                sample->loop_type = (loop_mode == 1) ? SAMPLE_LOOP_FORWARD : 
                                   (loop_mode == 2) ? SAMPLE_LOOP_BIDIRECTIONAL : 
                                   SAMPLE_LOOP_NONE;
                sample->loop_start = loop_start;
                sample->loop_end = loop_end;
            }
        }
        
        /* Get root note information */
        sample->root_note = AFgetinstparamlong(file, AF_DEFAULT_INST, AF_INST_MIDI_BASENOTE);
        
        /* Get detune information */
        sample->fine_tune = AFgetinstparamlong(file, AF_DEFAULT_INST, AF_INST_NUMCENTS_DETUNE);
        
        /* Ensure fine tune is in range -50 to +50 cents */
        if (sample->fine_tune < -50) sample->fine_tune = -50;
        if (sample->fine_tune > 50) sample->fine_tune = 50;
    }
    
    /* Get sample name if available */
    nmisc = AFgetmiscids(file, NULL);
    if (nmisc > 0) {
        if (nmisc > 32) nmisc = 32; /* Limit to our array size */
        AFgetmiscids(file, ids);
        
        /* Look for a name chunk */
        for (i = 0; i < nmisc; i++) {
            if (AFgetmisctype(file, ids[i]) == AF_MISC_AIFF_NAME) {
                size = AFgetmiscsize(file, ids[i]);
                if (size > 0 && size < 256) {
                    AFreadmisc(file, ids[i], name_buffer, size);
                    name_buffer[size] = '\0';
                    strncpy(sample->name, name_buffer, 255);
                    sample->name[255] = '\0';
                }
                break;
            }
        }
    }
    
    /* If no name was found, use filename as fallback */
    if (sample->name[0] == '\0') {
        basename = strrchr(filename, '/');
        if (basename) {
            basename++; /* Skip the slash */
        } else {
            basename = filename;
        }
        
        /* Copy basename and remove extension */
        strncpy(sample->name, basename, 255);
        sample->name[255] = '\0';
        dot = strrchr(sample->name, '.');
        if (dot) {
            *dot = '\0';
        }
    }
    
    /* Allocate buffer for reading frames */
    buffer = malloc(nframes * channels * (sampwidth / 8));
    if (buffer == NULL) {
        fprintf(stderr, "SMDI_LoadAIFSample: Failed to allocate buffer\n");
        SMDI_FreeSample(sample);
        AFclosefile(file);
        return NULL;
    }
    
    /* Read all frames at once */
    if (AFreadframes(file, AF_DEFAULT_TRACK, buffer, nframes) != nframes) {
        fprintf(stderr, "SMDI_LoadAIFSample: Failed to read frames\n");
        free(buffer);
        SMDI_FreeSample(sample);
        AFclosefile(file);
        return NULL;
    }
    
    /* Copy audio data to sample */
    memcpy(sample->sample_data, buffer, nframes * channels * (sampwidth / 8));
    
    /* Clean up */
    free(buffer);
    AFclosefile(file);
    
    return sample;
}

/* Save SMDI sample as AIF file */
BOOL SMDI_SaveAIFSample(SMDI_Sample* sample, const char* filename, int use_aifc) {
    AFfilehandle file;
    AFfilesetup setup;
    int error = 0;
    long marker_ids[2];
    long misc_ids[1];
    long inst_ids[1];
    long loop_ids[1];
    
    /* Verify parameters */
    if (sample == NULL || filename == NULL) {
        return FALSE;
    }
    
    /* Create a new file setup structure */
    setup = AFnewfilesetup();
    if (setup == AF_NULL_FILESETUP) {
        fprintf(stderr, "SMDI_SaveAIFSample: Failed to create file setup\n");
        return FALSE;
    }
    
    /* Set file format */
    AFinitfilefmt(setup, use_aifc ? AF_FILE_AIFFC : AF_FILE_AIFF);
    
    /* Initialize audio track parameters */
    AFinitchannels(setup, AF_DEFAULT_TRACK, sample->channels);
    AFinitsampfmt(setup, AF_DEFAULT_TRACK, AF_SAMPFMT_TWOSCOMP, sample->bits_per_sample);
    AFinitrate(setup, AF_DEFAULT_TRACK, sample->sample_rate);
    AFinitcompression(setup, AF_DEFAULT_TRACK, AF_COMPRESSION_NONE);
    
    /* Set up loops if present */
    if (sample->loop_type != SAMPLE_LOOP_NONE && 
        sample->loop_start < sample->loop_end) {
        
        /* Set up markers for loop points */
        marker_ids[0] = 1;  /* Loop start = 1 */
        marker_ids[1] = 2;  /* Loop end = 2 */
        AFinitmarkids(setup, AF_DEFAULT_TRACK, marker_ids, 2);
        AFinitmarkname(setup, AF_DEFAULT_TRACK, 1, "Loop Start");
        AFinitmarkname(setup, AF_DEFAULT_TRACK, 2, "Loop End");
        
        /* Set up instrument chunk */
        inst_ids[0] = 1;
        AFinitinstids(setup, inst_ids, 1);
        
        /* Set up loop IDs */
        loop_ids[0] = 1;
        AFinitloopids(setup, AF_DEFAULT_INST, loop_ids, 1);
    }
    
    /* Set up name chunk if sample has a name */
    if (sample->name[0] != '\0') {
        misc_ids[0] = 1;
        AFinitmiscids(setup, misc_ids, 1);
        AFinitmisctype(setup, 1, AF_MISC_AIFF_NAME);
        AFinitmiscsize(setup, 1, strlen(sample->name));
    }
    
    /* Open output file */
    file = AFopenfile(filename, "w", setup);
    if (file == AF_NULL_FILEHANDLE) {
        fprintf(stderr, "SMDI_SaveAIFSample: Failed to open file '%s' for writing\n", filename);
        AFfreefilesetup(setup);
        return FALSE;
    }
    
    /* Set loop points if present */
    if (sample->loop_type != SAMPLE_LOOP_NONE && 
        sample->loop_start < sample->loop_end) {
        
        /* Set marker positions */
        AFsetmarkpos(file, AF_DEFAULT_TRACK, 1, sample->loop_start);
        AFsetmarkpos(file, AF_DEFAULT_TRACK, 2, sample->loop_end);
        
        /* Set instrument parameters */
        AFsetinstparamlong(file, AF_DEFAULT_INST, AF_INST_MIDI_BASENOTE, sample->root_note);
        AFsetinstparamlong(file, AF_DEFAULT_INST, AF_INST_NUMCENTS_DETUNE, sample->fine_tune);
        
        /* Set loop parameters */
        AFsetinstparamlong(file, AF_DEFAULT_INST, AF_INST_SUSLOOPID, 1);
        AFsetloopmode(file, AF_DEFAULT_INST, 1, 
                    (sample->loop_type == SAMPLE_LOOP_FORWARD) ? 1 : 
                    (sample->loop_type == SAMPLE_LOOP_BIDIRECTIONAL) ? 2 : 0);
        AFsetloopstart(file, AF_DEFAULT_INST, 1, 1);  /* Marker ID 1 */
        AFsetloopend(file, AF_DEFAULT_INST, 1, 2);    /* Marker ID 2 */
    }
    
    /* Write name chunk if sample has a name */
    if (sample->name[0] != '\0') {
        AFwritemisc(file, 1, sample->name, strlen(sample->name));
    }
    
    /* Write audio frames */
    if (AFwriteframes(file, AF_DEFAULT_TRACK, sample->sample_data, sample->sample_count) 
        != sample->sample_count) {
        fprintf(stderr, "SMDI_SaveAIFSample: Failed to write frames\n");
        error = 1;
    }
    
    /* Clean up */
    AFclosefile(file);
    AFfreefilesetup(setup);
    
    return (error == 0);
}
