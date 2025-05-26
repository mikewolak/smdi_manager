/*
 * SMDI AIF file format support for IRIX 5.3
 * ANSI C90 compliant implementation for MIPS big-endian architecture
 */

#ifndef _SMDI_AIF_H
#define _SMDI_AIF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "smdi.h"
#include "smdi_sample.h"

/* Load an AIF file into SMDI sample format */
SMDI_Sample* SMDI_LoadAIFSample(const char* filename);

/* Save SMDI sample as AIF file */
BOOL SMDI_SaveAIFSample(SMDI_Sample* sample, const char* filename, int use_aifc);

#ifdef __cplusplus
}
#endif

#endif /* _SMDI_AIF_H */
