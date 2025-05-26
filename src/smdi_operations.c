/* src/smdi_operations.c - SMDI device operations */
#include "app_all.h"

/* Structure for progress callback */
typedef struct {
    int sample_id;
    char status_message[256];
} ProgressData;

/* Progress callback function for file transfers */
void progress_callback(SMDI_FileTransmissionInfo* fti, DWORD userData)
{
    SMDI_TransmissionInfo* ti;
    SMDI_SampleHeader* header;
    DWORD total_bytes;
    DWORD bytes_sent;
    int percent;
    ProgressData* progress_data;

    /* ANSI C90: All variables must be declared at the beginning */
    progress_data = (ProgressData*)userData;

    if (fti == NULL || fti->lpTransmissionInfo == NULL) {
        return;
    }

    ti = fti->lpTransmissionInfo;
    
    if (ti->lpSampleHeader == NULL) {
        return;
    }
    
    header = ti->lpSampleHeader;

    /* Calculate total bytes */
    total_bytes = header->dwLength * header->NumberOfChannels * header->BitsPerWord / 8;

    /* Calculate bytes sent so far */
    bytes_sent = ti->dwTransmittedPackets * ti->dwPacketSize;

    /* Adjust for last packet */
    if (bytes_sent > total_bytes) {
        bytes_sent = total_bytes;
    }

    /* Calculate percentage */
    if (total_bytes > 0) {
        percent = (int)((bytes_sent * 100) / total_bytes);
    } else {
        percent = 0;
    }

    /* Update progress in UI */
    show_progress(percent, progress_data->status_message);
}

/* Scan for SCSI devices on a host adapter */
int scan_scsi_devices(int ha_id, char *device_names[], int *device_types)
{
    int i;
    SCSI_DevInfo dev_info;
    int count;
    int old_debug;
    
    /* Initialize */
    count = 0;
    
    /* This is a debugging function - temporarily enable debugging */
    old_debug = SMDI_GetDebugMode();
    SMDI_SetDebugMode(1);
    
    update_status("Scanning SCSI bus %d for devices...", ha_id);
    
    for (i = 0; i < 16; i++) {
        /* Skip LUN 0 of target 7 (typically host adapter) */
        if (i == 7) {
            continue;
        }
        
        /* Test if unit is ready */
        if (SMDI_TestUnitReady(ha_id, i)) {
            /* Get device info */
            memset(&dev_info, 0, sizeof(SCSI_DevInfo));
            dev_info.dwStructSize = sizeof(SCSI_DevInfo);
            
            SMDI_GetDeviceInfo(ha_id, i, &dev_info);
            
            /* Copy device info */
            if (count < MAX_SCSI_DEVICES) {
                /* Copy name */
                strncpy(device_names[count], dev_info.cName, 31);
                device_names[count][31] = '\0';
                
                /* Set device type */
                device_types[count] = dev_info.DevType;
                
                /* Set SMDI flag in high bit if SMDI capable */
                if (dev_info.bSMDI) {
                    device_types[count] |= 0x80;
                }
                
                count++;
            }
        }
    }
    
    /* Restore debug state */
    SMDI_SetDebugMode(old_debug);
    
    return count;
}

/* Connect to a SMDI device */
int connect_to_device(int ha_id, int id)
{
    SCSI_DevInfo dev_info;
    DWORD response;
    
    /* Initialize device info */
    memset(&dev_info, 0, sizeof(SCSI_DevInfo));
    dev_info.dwStructSize = sizeof(SCSI_DevInfo);
    
    update_status("Connecting to device %d:%d...", ha_id, id);
    
    /* Test if unit is ready */
    if (!SMDI_TestUnitReady(ha_id, id)) {
        update_status("Device %d:%d is not ready", ha_id, id);
        return 0;
    }
    
    /* Get device info */
    SMDI_GetDeviceInfo(ha_id, id, &dev_info);
    
    /* Verify that it's an SMDI device */
    if (!dev_info.bSMDI) {
        update_status("Device %d:%d is not SMDI capable", ha_id, id);
        return 0;
    }
    
    /* Try sending an SMDI Master Identify command */
    response = SMDI_MasterIdentify(ha_id, id);
    
    if (response != SMDIM_SLAVEIDENTIFY) {
        update_status("Device did not respond correctly to SMDI identity check");
        return 0;
    }
    
    /* Store connection info */
    app_data.connected = 1;
    app_data.currentHA = ha_id;
    app_data.currentID = id;
    
    /* Update device info */
    update_device_info(dev_info.cName, dev_info.cManufacturer);
    
    update_status("Connected to SMDI device: %s %s", 
                dev_info.cManufacturer, dev_info.cName);
    
    return 1;
}


int refresh_sample_list(void)
{
    SMDI_SampleHeader sh;
    SampleInfo sample_info;
    int i;
    int found;
    DWORD result;
    
    /* Check if connected */
    if (!app_data.connected) {
        update_status("Not connected to any device");
        return 0;
    }
    
    update_status("Reading sample list from device %d:%d...", 
                app_data.currentHA, app_data.currentID);
    
    /* Clear the list */
    clear_sample_list();
    
    /* Scan for samples */
    found = 0;
    
    /* Modified to scan the first MAX_SAMPLES_TO_SCAN samples */
    for (i = 0; i < MAX_SAMPLES_TO_SCAN; i++) {
        /* Initialize sample header */
        memset(&sh, 0, sizeof(SMDI_SampleHeader));
        sh.dwStructSize = sizeof(SMDI_SampleHeader);
        
        /* Request sample header */
        result = SMDI_SampleHeaderRequest(app_data.currentHA, app_data.currentID, i, &sh);
        
        /* If we got a valid header response */
        if (result == SMDIM_SAMPLEHEADER) {
            /* Check if the sample exists */
            if (sh.bDoesExist) {
                /* Format sample properties */
                sample_info.id = i;
                strncpy(sample_info.name, sh.cName, 255);
                sample_info.name[255] = '\0';
                sample_info.rate = (int)(1000000000 / sh.dwPeriod);  /* Convert period to rate */
                sample_info.length = (int)sh.dwLength;
                sample_info.bits = (int)sh.BitsPerWord;
                sample_info.channels = (int)sh.NumberOfChannels;
                sample_info.exists = 1;
                
                /* Add to list */
                add_sample_to_list(&sample_info);
                
                found++;
            }
        } else {
            /* Log error but continue scanning */
            update_status("Error reading sample %d: response code 0x%08lx", i, result);
        }
    }
    
    /* Hide any progress indicator that might be showing */
    hide_progress();
    
    if (found == 0) {
        update_status("No samples found on device %d:%d", 
                    app_data.currentHA, app_data.currentID);
    } else {
        update_status("Found %d samples on device %d:%d", 
                    found, app_data.currentHA, app_data.currentID);
    }
    
    return found;
}




/* Receive a sample as AIF file */
int receive_sample_as_aif(int sample_id, const char *filename)
{
    ProgressData progress_data;
    SMDI_SampleHeader sh;
    SMDI_Sample *sample;
    void *buffer;
    DWORD result;
    DWORD packet_size;
    DWORD total_data_size;
    DWORD bytes_received;
    DWORD packet_num;
    void *current_pos;
    DWORD bytes_to_receive;
    int percent;
    
    /* Check if connected */
    if (!app_data.connected) {
        update_status("Not connected to any device");
        return 0;
    }
    
    /* Initialize all pointers to NULL */
    sample = NULL;
    buffer = NULL;
    
    /* Setup progress data */
    progress_data.sample_id = sample_id;
    sprintf(progress_data.status_message, "Receiving sample %d", sample_id);
    
    update_status("Receiving sample %d from device %d:%d...", 
                sample_id, app_data.currentHA, app_data.currentID);
    
    /* Initialize packet size */
    packet_size = PACKETSIZE;  /* Now defined in smdi.h */
    
    /* First, get sample header to determine size */
    memset(&sh, 0, sizeof(SMDI_SampleHeader));
    sh.dwStructSize = sizeof(SMDI_SampleHeader);
    
    result = SMDI_SampleHeaderRequest(app_data.currentHA, app_data.currentID, sample_id, &sh);
    if (result != SMDIM_SAMPLEHEADER || !sh.bDoesExist) {
        update_status("Sample %d not found on device", sample_id);
        return 0;
    }
    
    /* Validate header information */
    if (sh.dwLength == 0 || sh.NumberOfChannels == 0 || sh.BitsPerWord == 0) {
        update_status("Invalid sample parameters in header");
        return 0;
    }
    
    /* Calculate total data size with overflow check */
    if (sh.dwLength > 0xFFFFFFFF / (sh.NumberOfChannels * sh.BitsPerWord / 8)) {
        update_status("Sample too large to process");
        return 0;
    }
    
    total_data_size = (sh.dwLength * sh.NumberOfChannels * sh.BitsPerWord) / 8;
    
    /* Additional sanity check on data size */
    if (total_data_size == 0 || total_data_size > 100 * 1024 * 1024) {  /* 100MB max */
        update_status("Invalid sample data size: %lu bytes", total_data_size);
        return 0;
    }
    
    /* Allocate memory for the sample data */
    buffer = malloc(total_data_size);
    if (buffer == NULL) {
        update_status("Failed to allocate memory for sample data (%lu bytes)", total_data_size);
        return 0;
    }
    
    /* Zero the buffer */
    memset(buffer, 0, total_data_size);
    
    /* Begin receiving sample */
    result = SMDI_SendBeginSampleTransfer(app_data.currentHA, app_data.currentID, 
                                        sample_id, &packet_size);
    
    if (result == SMDIM_TRANSFERACKNOWLEDGE) {
        /* Initialize for packet reception */
        bytes_received = 0;
        packet_num = 0;
        
        /* Set operation in progress flag */
        app_data.operationInProgress = 1;
        
        /* Loop receiving packets */
        while (bytes_received < total_data_size) {
            current_pos = (char*)buffer + bytes_received;
            bytes_to_receive = packet_size;
            
            /* For the last packet, adjust size if needed */
            if (bytes_received + bytes_to_receive > total_data_size) {
                bytes_to_receive = total_data_size - bytes_received;
            }
            
            /* Receive packet */
            result = SMDI_NextDataPacketRequest(app_data.currentHA, app_data.currentID, 
                                         packet_num, current_pos, bytes_to_receive);
            
            if (result != SMDIM_DATAPACKET && result != SMDIM_ENDOFPROCEDURE) {
                /* Error receiving packet */
                update_status("Error receiving packet %d: 0x%08lX", packet_num, result);
                free(buffer);
                app_data.operationInProgress = 0;
                return 0;
            }
            
            /* Update progress */
            bytes_received += bytes_to_receive;
            packet_num++;
            
            /* Show progress */
            if (total_data_size > 0) {
                percent = (int)((bytes_received * 100) / total_data_size);
                show_progress(percent, progress_data.status_message);
            }
            
            /* Check if complete */
            if (result == SMDIM_ENDOFPROCEDURE || bytes_received >= total_data_size) {
                break;
            }
        }
        
        /* Clear operation flag */
        app_data.operationInProgress = 0;
        
        /* Create sample object from header and buffer */
        sample = SMDI_HeaderToSample(&sh, buffer);
        
        /* Free the raw buffer, it's copied into the sample structure */
        free(buffer);
        buffer = NULL;  /* Prevent accidental use after free */
        
        if (sample != NULL) {
            /* Save as AIF */
            if (SMDI_SaveAIFSample(sample, filename, 0)) {  /* 0 = not AIFC */
                /* Success - clean up and return */
                SMDI_FreeSample(sample);
                hide_progress();
                update_status("Sample %d received and saved as %s", sample_id, filename);
                return 1;
            } else {
                /* Failed to save AIF */
                SMDI_FreeSample(sample);
                hide_progress();
                update_status("Failed to convert sample to AIF format");
                return 0;
            }
        } else {
            /* Failed to create sample object */
            update_status("Failed to process downloaded sample");
            return 0;
        }
    } else {
        /* Failed to begin transfer */
        free(buffer);
        update_status("Failed to begin sample transfer. Error code: 0x%08lX", result);
        return 0;
    }
}


/* Modify the delete_sample function in smdi_operations.c */
int delete_sample(int sample_id)
{
    DWORD result;
    DWORD error_code;
    SMDI_SampleHeader sh;
    int success = 0;  /* Initialize to failure */
    
    /* Check if connected */
    if (!app_data.connected) {
        update_status("Not connected to any device");
        return 0;
    }
    
    update_status("Deleting sample %d from device %d:%d...", 
                sample_id, app_data.currentHA, app_data.currentID);
    
    /* First verify the sample exists */
    memset(&sh, 0, sizeof(SMDI_SampleHeader));
    sh.dwStructSize = sizeof(SMDI_SampleHeader);
    
    if (SMDI_SampleHeaderRequest(app_data.currentHA, app_data.currentID, sample_id, &sh) != SMDIM_SAMPLEHEADER || !sh.bDoesExist) {
        update_status("Sample %d not found on device %d:%d", 
                    sample_id, app_data.currentHA, app_data.currentID);
        return 0;
    }
    
    /* Perform the deletion */
    result = SMDI_DeleteSample(app_data.currentHA, app_data.currentID, sample_id);
    
    /* Handle the result */
    if (result == SMDIM_ACK || result == SMDIM_ENDOFPROCEDURE) {
        update_status("Sample %d deleted successfully", sample_id);
        hide_progress();
        success = 1;  /* Set success flag */
    } 
    /* Check for specific error codes */
    else if (result == SMDIM_MESSAGEREJECT) {
        /* Get the last error */
        error_code = SMDI_GetLastError();
        
        /* Provide specific error message based on the error code */
        switch (error_code) {
            case SMDIE_OUTOFRANGE:
                update_status("Delete failed: Sample ID out of range");
                break;
                
            case SMDIE_NOSAMPLE:
                update_status("Delete failed: Sample %d does not exist on the device", sample_id);
                break;
                
            case SMDIE_NOMEMORY:
                update_status("Delete failed: Device has insufficient memory for this operation");
                break;
                
            case SMDIE_UNSUPPSAMBITS:
                update_status("Delete failed: Unsupported sample bits format");
                break;
                
            default:
                update_status("Delete failed with error code: 0x%08lX", error_code);
                break;
        }
    } else {
        update_status("Failed to delete sample %d. Response: 0x%08lX", 
                    sample_id, result);
        hide_progress();
    }
    
    return success;
}


/* Send an AIF file to the device */
int send_aif_file(const char *filename, int sample_id)
{
    SMDI_Sample* sample;
    SMDI_FileTransfer ft;
    DWORD result;
    ProgressData progress_data;
    char temp_filename[MAX_PATH];
    
    /* Check if connected */
    if (!app_data.connected) {
        update_status("Not connected to any device");
        return 0;
    }
    
    update_status("Loading AIF file '%s'...", filename);
    
    /* Load AIF file */
    sample = SMDI_LoadAIFSample(filename);
    if (sample == NULL) {
        update_status("Failed to load AIF file '%s'", filename);
        return 0;
    }
    
    update_status("AIF file loaded: %s, %lu Hz, %d bits, %d channels",
                sample->name, sample->sample_rate, sample->bits_per_sample, sample->channels);
    
    /* Create temporary file in native format */
    sprintf(temp_filename, "/tmp/smdi_temp_%lu.sdmp", (unsigned long)sample_id);
    if (!SMDI_SaveSample(sample, temp_filename)) {
        update_status("Failed to create temporary file");
        SMDI_FreeSample(sample);
        return 0;
    }
    
    /* Setup progress data */
    progress_data.sample_id = sample_id;
    sprintf(progress_data.status_message, "Sending to sample %d", sample_id);
    
    /* Set up file transfer */
    memset(&ft, 0, sizeof(SMDI_FileTransfer));
    ft.dwStructSize = sizeof(SMDI_FileTransfer);
    ft.HA_ID = app_data.currentHA;
    ft.SCSI_ID = app_data.currentID;
    ft.dwSampleNumber = sample_id;
    ft.lpFileName = temp_filename;
    ft.lpSampleName = sample->name;
    ft.lpCallback = (void*)progress_callback;
    ft.dwUserData = (DWORD)&progress_data;
    ft.bAsync = FALSE;
    ft.lpReturnValue = &result;
    
    /* Set operation in progress flag */
    app_data.operationInProgress = 1;
    
    /* Send the file */
    result = SMDI_SendFile(&ft);
    
    /* Clear operation flag */
    app_data.operationInProgress = 0;
    
    /* Clean up */
    SMDI_FreeSample(sample);
    unlink(temp_filename);  /* Remove temporary file */
    
    if (result == SMDIM_ENDOFPROCEDURE || result == SMDIM_ACK) {
        update_status("Sample uploaded successfully to sample %d", sample_id);
	hide_progress(); 
        return 1;
    } else {
        update_status("Failed to upload sample. Error code: 0x%08lX", result);
	hide_progress(); 
        return 0;
    }
}
