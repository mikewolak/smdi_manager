/* src/callbacks.c - Callback functions for GUI events */
#include "app_all.h"

/* Structure for sample ID dialog callbacks */
typedef struct {
    Widget dialog;
    Widget text_field;
    char filename[MAX_PATH];
} OkCallbackData;


/* Function declarations */
static void sample_id_ok_callback(Widget widget, XtPointer client_data, XtPointer call_data);

/* Exit application */
void exit_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
    /* Exit the application */
    exit(0);
}

/* Show help/about dialog */
void help_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Widget dialog;
    XmString message;
    
    /* Create an information dialog */
    dialog = XmCreateInformationDialog(
        widget,                    /* Parent widget */
        "about_dialog",            /* Dialog name */
        NULL, 0);                  /* No arguments */
    
    /* Set the message with copyright information */
    message = XmStringCreateLtoR(
        "IRIX 5.3 SMDI Sample Manager\n\n"
        "A GUI tool for managing samples on SMDI devices\n"
        "using SCSI on IRIX 5.3 systems.\n\n"
        "(c)2025 Mike Wolak\n"
        "mikewolak@gmail.com",
        XmSTRING_DEFAULT_CHARSET
    );
    
    /* Set the message */
    XtVaSetValues(
        XtParent(dialog),          /* Parent shell */
        XmNtitle, "About",         /* Dialog title */
        NULL);                     /* Terminate list */
    
    XtVaSetValues(
        dialog,                    /* Widget to set */
        XmNmessageString, message,
        NULL);                     /* Terminate list */
    
    /* Free the XmString */
    XmStringFree(message);
    
    /* Manage the dialog's buttons */
    XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
    XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
    
    /* Display the dialog */
    XtManageChild(dialog);
}

/* Connect to SMDI device */
void connect_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
    int ha_id;
    int id;
    XmString str;
    
    /* Get the selected values from the app_data structure */
    ha_id = app_data.currentHA;
    id = app_data.currentID;
    
    update_status("Connecting to SMDI device on %d:%d...", ha_id, id);
    
    /* Try to connect to the device */
    if (connect_to_device(ha_id, id)) {
        /* Change the Connect button to Disconnect */
        str = XmStringCreateLocalized("Disconnect");
        XtVaSetValues(app_data.connectButton, XmNlabelString, str, NULL);
        XmStringFree(str);
        
        /* Clear and refresh the sample list */
        clear_sample_list();
        refresh_sample_list();
    } else {
        /* Show error message */
        show_message_dialog(app_data.mainWindow, "Connection Error", 
                           "Failed to connect to SMDI device. Make sure the device is powered on and properly connected.",
                           XmDIALOG_ERROR);
    }
}

/* Scan for SCSI devices */
void scan_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Widget dialog_shell;
    Widget form;
    Widget label;
    Widget text_w;
    Widget ok_button;
    XmString str;
    Arg args[20];
    int n;
    int ha_id;
    char device_names[MAX_SCSI_DEVICES][32];
    char *device_ptrs[MAX_SCSI_DEVICES];
    int device_types[MAX_SCSI_DEVICES];
    int target_ids[MAX_SCSI_DEVICES];
    int i;
    int scsi_id;
    int count;
    char buffer[1024];
    char line[80];
    const char *type_str;
    int found_smdi;
    int smdi_ha;
    int smdi_id;
    char temp_buffer[32];
    SCSI_DevInfo dev_info;
    
    /* Initialize variables */
    found_smdi = 0;
    smdi_ha = 0;
    smdi_id = 0;
    
    /* Get the selected Host Adapter ID from the app_data struct */
    ha_id = app_data.currentHA;
    
    update_status("Scanning SCSI bus %d for devices...", ha_id);
    
    /* Set up device name pointers */
    for (i = 0; i < MAX_SCSI_DEVICES; i++) {
        device_ptrs[i] = device_names[i];
    }
    
    /* Scan each target ID on the SCSI bus */
    count = 0;
    for (scsi_id = 0; scsi_id < 8; scsi_id++) {
        /* Skip ID 7 which is typically the host adapter */
        if (scsi_id == 7) continue;
        
        /* Test if device is ready */
        if (SMDI_TestUnitReady(ha_id, scsi_id)) {
            /* Get device info */
            memset(&dev_info, 0, sizeof(SCSI_DevInfo));
            dev_info.dwStructSize = sizeof(SCSI_DevInfo);
            
            SMDI_GetDeviceInfo(ha_id, scsi_id, &dev_info);
            
            /* Store device info */
            if (count < MAX_SCSI_DEVICES) {
                /* Store the target ID */
                target_ids[count] = scsi_id;
                
                /* Copy name */
                strncpy(device_names[count], dev_info.cName, 31);
                device_names[count][31] = '\0';
                
                /* Set device type */
                device_types[count] = dev_info.DevType;
                
                /* Set SMDI flag in high bit if SMDI capable */
                if (dev_info.bSMDI) {
                    device_types[count] |= 0x80;
                    
                    /* Remember the first SMDI device we found */
                    if (!found_smdi) {
                        found_smdi = 1;
                        smdi_ha = ha_id;
                        smdi_id = scsi_id;
                    }
                }
                
                count++;
            }
        }
    }
    
    /* Format results for display */
    if (count > 0) {
        /* Start with a header */
        sprintf(buffer, "Devices found on SCSI Host Adapter %d:\n\n", ha_id);
        
        /* Add header line with fixed-width columns */
        strcat(buffer, " HA:ID | Type       | Device Name\n");
        strcat(buffer, "-------+------------+------------------------------------------\n");
        
        for (i = 0; i < count; i++) {
            /* Get device type string */
            switch (device_types[i] & 0x1F) {
                case 0x00: type_str = "Disk      "; break;
                case 0x01: type_str = "Tape      "; break;
                case 0x02: type_str = "Printer   "; break;
                case 0x03: type_str = "Processor "; break;
                case 0x04: type_str = "WORM      "; break;
                case 0x05: type_str = "CD-ROM    "; break;
                case 0x06: type_str = "Scanner   "; break;
                case 0x07: type_str = "Optical   "; break;
                case 0x08: type_str = "Changer   "; break;
                case 0x09: type_str = "Comm      "; break;
                default:   type_str = "Unknown   "; break;
            }
            
            /* Format each line with fixed-width columns including actual SCSI IDs */
            sprintf(line, "  %d:%d  | %s | %-30.30s%s\n", 
                   ha_id, target_ids[i], type_str, device_names[i],
                   (device_types[i] & 0x80) ? " (SMDI)" : "");
            strcat(buffer, line);
        }
        
        /* If we found an SMDI device, set the Host Adapter and Target ID */
        if (found_smdi) {
            /* Set Host Adapter */
            app_data.currentHA = smdi_ha;
            sprintf(temp_buffer, "Host Adapter: %d", smdi_ha);
            str = XmStringCreateLocalized(temp_buffer);
            XtVaSetValues(app_data.haOption, XmNlabelString, str, NULL);
            XmStringFree(str);
            
            /* Set Target ID */
            app_data.currentID = smdi_id;
            sprintf(temp_buffer, "Target ID: %d", smdi_id);
            str = XmStringCreateLocalized(temp_buffer);
            XtVaSetValues(app_data.idOption, XmNlabelString, str, NULL);
            XmStringFree(str);
            
            /* Add a note to the scan results */
            strcat(buffer, "\n\nFound SMDI device at Host Adapter ");
            sprintf(line, "%d, Target ID %d.\n", smdi_ha, smdi_id);
            strcat(buffer, line);
            strcat(buffer, "These values have been automatically selected in the main window.\n");
            strcat(buffer, "You can now press the Connect button to connect to this device.\n");
        }
        
        /* Create a dialog shell */
        dialog_shell = XtVaCreatePopupShell(
            "scan_results",
            transientShellWidgetClass, app_data.mainWindow,
            XmNtitle, "SCSI Scan Results",
            XmNdeleteResponse, XmDESTROY,
            XmNwidth, 500,
            XmNheight, 300,
            NULL);
        
        /* Create a form in the dialog */
        form = XtVaCreateWidget(
            "form",
            xmFormWidgetClass, dialog_shell,
            XmNfractionBase, 100,
            NULL);
        
        /* Create a label */
        str = XmStringCreateLocalized("SCSI Device Scan Results:");
        label = XtVaCreateManagedWidget(
            "label",
            xmLabelWidgetClass, form,
            XmNlabelString, str,
            XmNtopAttachment, XmATTACH_FORM,
            XmNtopOffset, 10,
            XmNleftAttachment, XmATTACH_FORM,
            XmNleftOffset, 10,
            XmNrightAttachment, XmATTACH_FORM,
            XmNrightOffset, 10,
            NULL);
        XmStringFree(str);
        
        /* Create a text widget */
        n = 0;
        XtSetArg(args[n], XmNrows, 12); n++;
        XtSetArg(args[n], XmNcolumns, 60); n++;
        XtSetArg(args[n], XmNeditable, False); n++;
        XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
        XtSetArg(args[n], XmNcursorPositionVisible, False); n++;
        XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
        XtSetArg(args[n], XmNtopWidget, label); n++;
        XtSetArg(args[n], XmNtopOffset, 10); n++;
        XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
        XtSetArg(args[n], XmNleftOffset, 10); n++;
        XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
        XtSetArg(args[n], XmNrightOffset, 10); n++;
        XtSetArg(args[n], XmNbottomAttachment, XmATTACH_POSITION); n++;
        XtSetArg(args[n], XmNbottomPosition, 80); n++;
        XtSetArg(args[n], XmNvalue, buffer); n++;
        
        text_w = XmCreateScrolledText(form, "text", args, n);
        XtManageChild(text_w);
        
        /* Create an OK button */
        str = XmStringCreateLocalized("OK");
        ok_button = XtVaCreateManagedWidget(
            "ok_button",
            xmPushButtonWidgetClass, form,
            XmNlabelString, str,
            XmNtopAttachment, XmATTACH_POSITION,
            XmNtopPosition, 85,
            XmNbottomAttachment, XmATTACH_POSITION,
            XmNbottomPosition, 95,
            XmNleftAttachment, XmATTACH_POSITION,
            XmNleftPosition, 40,
            XmNrightAttachment, XmATTACH_POSITION,
            XmNrightPosition, 60,
            NULL);
        XmStringFree(str);
        
        /* Add callback to close the dialog */
        XtAddCallback(ok_button, XmNactivateCallback, 
                     dialog_close_callback, (XtPointer)dialog_shell);
        
        /* Manage the form */
        XtManageChild(form);
        
        /* Show the dialog */
        XtPopup(dialog_shell, XtGrabNone);
        
        /* Update status with info about SMDI device if found */
        if (found_smdi) {
            update_status("Found SMDI device at Host Adapter %d, Target ID %d", smdi_ha, smdi_id);
        } else {
            update_status("Found %d devices on SCSI bus %d (no SMDI devices)", count, ha_id);
        }
    } else {
        /* No devices found */
        show_message_dialog(app_data.mainWindow, "Scan Results", 
                           "No SCSI devices found on the selected host adapter.",
                           XmDIALOG_INFORMATION);
        update_status("No devices found on SCSI bus %d", ha_id);
    }
}

/* Refresh sample list */
void refresh_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
    /* Check if connected */
    if (!app_data.connected) {
        show_message_dialog(app_data.mainWindow, "Not Connected", 
                           "Please connect to a SMDI device first.",
                           XmDIALOG_WARNING);
        return;
    }
    
    /* Clear the list */
    clear_sample_list();
    
    /* Refresh the list */
    refresh_sample_list();
}

/* Sample selected (double-click) */
void sample_selected_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
    XmDrawingAreaCallbackStruct *cbs;
    int selected_row;
    SampleInfo *sample;
    
    cbs = (XmDrawingAreaCallbackStruct *)call_data;
    
    /* Only show dialog for double-click event */
    if (cbs->reason != XmCR_DEFAULT_ACTION) {
        return;
    }
    
    /* Get the selected row */
    selected_row = grid_get_selected_row(widget);
    
    /* Get the sample info */
    if (selected_row >= 0 && selected_row < app_data.numSamples) {
        sample = &app_data.samples[selected_row];
        
        /* Show dialog with sample info */
        show_sample_details(sample);
    }
}

/* Display detailed sample information in a dialog */
void show_sample_details(SampleInfo *sample)
{
    Widget dialog_shell;
    Widget form;
    Widget labels[6];
    Widget values[6];
    XmString label_strs[6];
    XmString value_strs[6];
    Widget ok_button;
    XmString ok_str;
    char buffer[256];
    int i;
    
    if (sample == NULL) {
        return;
    }
    
    /* Create a transient dialog shell */
    dialog_shell = XtVaCreatePopupShell(
        "sample_details",           /* Widget name */
        transientShellWidgetClass,  /* Widget class for dialog */
        app_data.mainWindow,        /* Parent widget */
        XmNtitle, "Sample Information",  /* Dialog title */
        XmNdeleteResponse, XmDESTROY,
        XmNwidth, 350,
        XmNheight, 250,
        NULL);
    
    /* Create a form in the dialog */
    form = XtVaCreateWidget(
        "form",                     /* Widget name */
        xmFormWidgetClass,          /* Widget class */
        dialog_shell,               /* Parent widget */
        XmNfractionBase, 100,       /* Divide form into 100 units */
        NULL);
    
    /* Create field labels and values */
    label_strs[0] = XmStringCreateLocalized("Sample ID:");
    label_strs[1] = XmStringCreateLocalized("Name:");
    label_strs[2] = XmStringCreateLocalized("Sample Rate:");
    label_strs[3] = XmStringCreateLocalized("Length:");
    label_strs[4] = XmStringCreateLocalized("Bits Per Sample:");
    label_strs[5] = XmStringCreateLocalized("Channels:");
    
    /* Format value strings */
    sprintf(buffer, "%d", sample->id);
    value_strs[0] = XmStringCreateLocalized(buffer);
    
    value_strs[1] = XmStringCreateLocalized(sample->name);
    
    sprintf(buffer, "%d Hz", sample->rate);
    value_strs[2] = XmStringCreateLocalized(buffer);
    
    sprintf(buffer, "%d samples", sample->length);
    value_strs[3] = XmStringCreateLocalized(buffer);
    
    sprintf(buffer, "%d", sample->bits);
    value_strs[4] = XmStringCreateLocalized(buffer);
    
    sprintf(buffer, "%d", sample->channels);
    value_strs[5] = XmStringCreateLocalized(buffer);
    
    /* Create the label and value widgets in a grid layout */
    for (i = 0; i < 6; i++) {
        /* Create label */
        labels[i] = XtVaCreateManagedWidget(
            "label",                 /* Widget name */
            xmLabelWidgetClass,      /* Widget class */
            form,                    /* Parent widget */
            XmNlabelString, label_strs[i],
            XmNalignment, XmALIGNMENT_END,
            XmNtopAttachment, XmATTACH_POSITION,
            XmNtopPosition, 10 + i * 12,
            XmNleftAttachment, XmATTACH_POSITION,
            XmNleftPosition, 5,
            XmNrightAttachment, XmATTACH_POSITION,
            XmNrightPosition, 40,
            NULL);
        
        /* Create value */
        values[i] = XtVaCreateManagedWidget(
            "value",                 /* Widget name */
            xmLabelWidgetClass,      /* Widget class */
            form,                    /* Parent widget */
            XmNlabelString, value_strs[i],
            XmNalignment, XmALIGNMENT_BEGINNING,
            XmNtopAttachment, XmATTACH_POSITION,
            XmNtopPosition, 10 + i * 12,
            XmNleftAttachment, XmATTACH_POSITION,
            XmNleftPosition, 42,
            XmNrightAttachment, XmATTACH_POSITION,
            XmNrightPosition, 95,
            NULL);
    }
    
    /* Create OK button */
    ok_str = XmStringCreateLocalized("OK");
    ok_button = XtVaCreateManagedWidget(
        "ok_button",               /* Widget name */
        xmPushButtonWidgetClass,   /* Widget class */
        form,                      /* Parent widget */
        XmNlabelString, ok_str,
        XmNtopAttachment, XmATTACH_POSITION,
        XmNtopPosition, 85,
        XmNbottomAttachment, XmATTACH_POSITION,
        XmNbottomPosition, 95,
        XmNleftAttachment, XmATTACH_POSITION,
        XmNleftPosition, 40,
        XmNrightAttachment, XmATTACH_POSITION,
        XmNrightPosition, 60,
        NULL);
    
    /* Add callback to destroy the dialog */
    XtAddCallback(ok_button, XmNactivateCallback, 
                 dialog_close_callback, (XtPointer)dialog_shell);
    
    /* Free XmStrings */
    XmStringFree(ok_str);
    for (i = 0; i < 6; i++) {
        XmStringFree(label_strs[i]);
        XmStringFree(value_strs[i]);
    }
    
    /* Manage the form */
    XtManageChild(form);
    
    /* Show the dialog */
    XtPopup(dialog_shell, XtGrabNone);
}

/* Dialog close callback helper */
void dialog_close_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Widget dialog = (Widget)client_data;
    XtDestroyWidget(dialog);
}

/* Sample ID selection dialog - Called before sending AIF file */
void select_sample_id_dialog(Widget parent, const char *filename)
{
    Widget dialog_shell;
    Widget form;
    Widget label;
    Widget id_text;
    Widget ok_button;
    Widget cancel_button;
    XmString str;
    char buffer[32];
    int next_id;
    OkCallbackData *ok_data;
    
    /* Calculate next available sample ID */
    next_id = 0;
    
    /* If we have samples, find the next available ID */
    if (app_data.numSamples > 0) {
        int i;
        int highest_id = -1;
        
        for (i = 0; i < app_data.numSamples; i++) {
            if (app_data.samples[i].id > highest_id) {
                highest_id = app_data.samples[i].id;
            }
        }
        
        next_id = highest_id + 1;
    }
    
    /* Create a transient dialog shell */
    dialog_shell = XtVaCreatePopupShell(
        "sample_id_dialog",
        transientShellWidgetClass, parent,
        XmNtitle, "Select Sample ID",
        XmNdeleteResponse, XmDESTROY,
        XmNwidth, 300,
        XmNheight, 150,
        NULL);
    
    /* Create a form in the dialog */
    form = XtVaCreateWidget(
        "form",
        xmFormWidgetClass, dialog_shell,
        XmNfractionBase, 100,
        NULL);
    
    /* Create a label */
    str = XmStringCreateLocalized("Enter Sample ID for upload:");
    label = XtVaCreateManagedWidget(
        "label",
        xmLabelWidgetClass, form,
        XmNlabelString, str,
        XmNtopAttachment, XmATTACH_FORM,
        XmNtopOffset, 15,
        XmNleftAttachment, XmATTACH_FORM,
        XmNleftOffset, 10,
        XmNrightAttachment, XmATTACH_FORM,
        XmNrightOffset, 10,
        NULL);
    XmStringFree(str);
    
    /* Create a text field with default value */
    sprintf(buffer, "%d", next_id);
    id_text = XtVaCreateManagedWidget(
        "id_text",
        xmTextFieldWidgetClass, form,
        XmNvalue, buffer,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label,
        XmNtopOffset, 10,
        XmNleftAttachment, XmATTACH_FORM,
        XmNleftOffset, 10,
        XmNrightAttachment, XmATTACH_FORM,
        XmNrightOffset, 10,
        XmNmaxLength, 5,
        NULL);
    
    /* Create OK button */
    str = XmStringCreateLocalized("OK");
    ok_button = XtVaCreateManagedWidget(
        "ok_button",
        xmPushButtonWidgetClass, form,
        XmNlabelString, str,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, id_text,
        XmNtopOffset, 20,
        XmNbottomAttachment, XmATTACH_FORM,
        XmNbottomOffset, 10,
        XmNleftAttachment, XmATTACH_POSITION,
        XmNleftPosition, 20,
        XmNrightAttachment, XmATTACH_POSITION,
        XmNrightPosition, 40,
        NULL);
    XmStringFree(str);
    
    /* Create Cancel button */
    str = XmStringCreateLocalized("Cancel");
    cancel_button = XtVaCreateManagedWidget(
        "cancel_button",
        xmPushButtonWidgetClass, form,
        XmNlabelString, str,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, id_text,
        XmNtopOffset, 20,
        XmNbottomAttachment, XmATTACH_FORM,
        XmNbottomOffset, 10,
        XmNleftAttachment, XmATTACH_POSITION,
        XmNleftPosition, 60,
        XmNrightAttachment, XmATTACH_POSITION,
        XmNrightPosition, 80,
        NULL);
    XmStringFree(str);
    
    /* Prepare callback data for the OK button */
    ok_data = (OkCallbackData *)XtMalloc(sizeof(OkCallbackData));
    ok_data->dialog = dialog_shell;
    ok_data->text_field = id_text;
    strncpy(ok_data->filename, filename, MAX_PATH - 1);
    ok_data->filename[MAX_PATH - 1] = '\0';
    
    /* Add callbacks for the buttons */
    XtAddCallback(ok_button, XmNactivateCallback, sample_id_ok_callback, (XtPointer)ok_data);
    XtAddCallback(cancel_button, XmNactivateCallback, 
                 dialog_close_callback, (XtPointer)dialog_shell);
    
    /* Manage the form and show the dialog */
    XtManageChild(form);
    XtPopup(dialog_shell, XtGrabNone);
}

/* Callback function for the sample ID dialog's OK button */
static void sample_id_ok_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
    OkCallbackData *data = (OkCallbackData *)client_data;
    char *text_value;
    int sample_id;
    
    /* Get the sample ID from the text field */
    text_value = XmTextFieldGetString(data->text_field);
    sample_id = atoi(text_value);
    XtFree(text_value);
    
    /* Validate sample ID */
    if (sample_id < 0 || sample_id > 127) {
        show_message_dialog(app_data.mainWindow, "Invalid Sample ID", 
                           "Sample ID must be between 0 and 127.", 
                           XmDIALOG_ERROR);
        return;
    }
    
    /* Send the AIF file with the specified sample ID */
    update_status("Sending AIF file to sample %d...", sample_id);
    
    if (send_aif_file(data->filename, sample_id)) {
        update_status("AIF file sent successfully to sample %d", sample_id);
        
        /* Refresh the sample list */
        clear_sample_list();
        refresh_sample_list();
    } else {
        update_status("Failed to send AIF file to sample %d", sample_id);
        show_message_dialog(app_data.mainWindow, "Send Error", 
                           "Failed to send the AIF file.", 
                           XmDIALOG_ERROR);
    }
    
    /* Destroy the dialog */
    XtDestroyWidget(data->dialog);
    XtFree((char *)data);
}

/* Send AIF file */
void send_aif_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Widget file_dialog;
    XmString filter;
    
    /* Check if connected */
    if (!app_data.connected) {
        show_message_dialog(app_data.mainWindow, "Not Connected", 
                           "Please connect to a SMDI device first.",
                           XmDIALOG_WARNING);
        return;
    }
    
    /* Create a file selection dialog */
    file_dialog = XmCreateFileSelectionDialog(
        app_data.mainWindow,       /* Parent widget */
        "send_aif_dialog",         /* Dialog name */
        NULL, 0);                  /* No arguments */
    
    /* Set dialog title */
    XtVaSetValues(
        XtParent(file_dialog),     /* Parent shell */
XmNtitle, "Select AIF File", /* Dialog title */
        NULL);                     /* Terminate list */
    
    /* Set file filter pattern */
    filter = XmStringCreateLocalized("*.aif");
    XtVaSetValues(
        file_dialog,
        XmNpattern, filter,
        NULL);
    XmStringFree(filter);
    
    /* Add callbacks for OK and Cancel buttons */
    XtAddCallback(file_dialog, XmNokCallback, file_selected_callback, (XtPointer)1);
    XtAddCallback(file_dialog, XmNcancelCallback, 
                 (XtCallbackProc)XtUnmanageChild, NULL);
    
    /* Show the dialog */
    XtManageChild(file_dialog);
}

/* Receive AIF file (right-click menu) */
void receive_aif_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Widget file_dialog;
    XmString filter;
    int selected_row;
    int sample_id;
    
    /* Check if connected */
    if (!app_data.connected) {
        show_message_dialog(app_data.mainWindow, "Not Connected", 
                           "Please connect to a SMDI device first.",
                           XmDIALOG_WARNING);
        return;
    }
    
    /* Get selected row */
    selected_row = grid_get_selected_row(app_data.sampleGrid);
    
    /* Check if valid selection */
    if (selected_row < 0 || selected_row >= app_data.numSamples) {
        show_message_dialog(app_data.mainWindow, "Selection Error", 
                           "Please select a sample first.",
                           XmDIALOG_WARNING);
        return;
    }
    
    /* Get the sample ID from the selected position */
    sample_id = app_data.samples[selected_row].id;
    
/* Create a file selection dialog */
    file_dialog = XmCreateFileSelectionDialog(
        app_data.mainWindow,   /* Parent widget */
        "receive_aif_dialog",  /* Dialog name */
        NULL, 0);              /* No arguments */
    
    /* Set dialog title */
    XtVaSetValues(
        XtParent(file_dialog), /* Parent shell */
        XmNtitle, "Save As AIF File", /* Dialog title */
        NULL);                 /* Terminate list */
    
    /* Set file filter pattern */
    filter = XmStringCreateLocalized("*.aif");
    XtVaSetValues(
        file_dialog,
        XmNpattern, filter,
        NULL);
    XmStringFree(filter);
    
    /* Add callbacks for OK and Cancel buttons */
    XtAddCallback(file_dialog, XmNokCallback, file_selected_callback, 
                 (XtPointer)(long)sample_id);
    XtAddCallback(file_dialog, XmNcancelCallback, 
                 (XtCallbackProc)XtUnmanageChild, NULL);
    
    /* Show the dialog */
    XtManageChild(file_dialog);
}

/* Delete sample (right-click menu) */
void delete_sample_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Widget dialog;
    XmString message;
    int selected_row;
    int sample_id;
    char confirm_message[256];
    
    /* Check if connected */
    if (!app_data.connected) {
        show_message_dialog(app_data.mainWindow, "Not Connected", 
                           "Please connect to a SMDI device first.",
                           XmDIALOG_WARNING);
        return;
    }
    
    /* Get selected row */
    selected_row = grid_get_selected_row(app_data.sampleGrid);
    
    /* Check if valid selection */
    if (selected_row < 0 || selected_row >= app_data.numSamples) {
        show_message_dialog(app_data.mainWindow, "Selection Error", 
                           "Please select a sample first.",
                           XmDIALOG_WARNING);
        return;
    }
    
    /* Get the sample ID from the selected position */
    sample_id = app_data.samples[selected_row].id;
    
    /* Format confirmation message */
    sprintf(confirm_message, 
           "Are you sure you want to delete sample %d: %s?",
           sample_id, app_data.samples[selected_row].name);
    
    /* Create a confirmation dialog */
    dialog = XmCreateQuestionDialog(
        app_data.mainWindow,   /* Parent widget */
        "confirm_delete",      /* Dialog name */
        NULL, 0);              /* No arguments */
    
    /* Set dialog title and message */
    XtVaSetValues(
        XtParent(dialog),      /* Parent shell */
        XmNtitle, "Confirm Delete", /* Dialog title */
        NULL);                 /* Terminate list */
    
    message = XmStringCreateLocalized(confirm_message);
    XtVaSetValues(
        dialog,                /* Widget to set */
        XmNmessageString, message,
        NULL);                 /* Terminate list */
    XmStringFree(message);
    
    /* Add callbacks for OK and Cancel buttons */
    XtAddCallback(dialog, XmNokCallback, confirm_delete_callback, 
                 (XtPointer)(long)sample_id);
    XtAddCallback(dialog, XmNcancelCallback, 
                 (XtCallbackProc)XtUnmanageChild, NULL);
    
    /* Hide Help button */
    XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
    
    /* Show dialog */
    XtManageChild(dialog);
}

/* Confirm delete callback */
void confirm_delete_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
    int sample_id;
    int result;
    
    /* Get the sample ID */
    sample_id = (int)(long)client_data;
    
    /* Unmanage the dialog */
    XtUnmanageChild(widget);
    
    update_status("Deleting sample %d...", sample_id);
    
    /* Delete the sample */
    result = delete_sample(sample_id);
    
    if (result) {
        update_status("Sample %d deleted successfully", sample_id);
    } else {
        update_status("Error reported during delete of sample %d, refreshing list anyway", sample_id);
    }
    
    /* Always refresh the sample list regardless of success or failure */
    clear_sample_list();
    refresh_sample_list();
}

/* File selection callback */
void file_selected_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
    XmFileSelectionBoxCallbackStruct *cbs;
    char *filename;
    int operation;
    int sample_id;
    
    /* Get callback structure and extract filename */
    cbs = (XmFileSelectionBoxCallbackStruct *)call_data;
    operation = (int)(long)client_data;
    
    /* Convert the XmString to a C string */
    if (!XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &filename)) {
        show_message_dialog(app_data.mainWindow, "Error", 
                           "Invalid filename",
                           XmDIALOG_ERROR);
        return;
    }
    
    /* Unmanage the dialog */
    XtUnmanageChild(widget);
    
    /* Check the operation type */
    if (operation == 1) {
        /* Send AIF file - Now call our sample ID selection dialog */
        select_sample_id_dialog(app_data.mainWindow, filename);
    } else {
        /* Receive sample as AIF (operation is the sample_id) */
        sample_id = operation;
        
        update_status("Receiving sample %d as AIF file...", sample_id);
        
        if (receive_sample_as_aif(sample_id, filename)) {
            update_status("Sample %d received and saved as %s", sample_id, filename);
        } else {
            update_status("Failed to receive sample %d", sample_id);
            show_message_dialog(app_data.mainWindow, "Receive Error", 
                               "Failed to receive the sample.",
                               XmDIALOG_ERROR);
        }
    }
    
    /* Free the filename string */
    XtFree(filename);
}

/* Create popup menu for right-clicking on samples */
void sample_create_popup_menu(Widget widget, XtPointer client_data, XEvent *event, Boolean *continue_to_dispatch)
{
    static Widget popup_menu = NULL;
    XButtonEvent *bevent;
    XmString str;
    int selected_row;
    Widget receive_button;
    Widget delete_button;
    Widget separator;
    
    /* Get button event */
    bevent = (XButtonEvent *)event;
    
    /* Only handle right mouse button (Button3) */
    if (bevent->button != Button3) {
        return;
    }
    
    /* Get the selected row */
    selected_row = grid_get_selected_row(widget);
    
    /* Check if we have a valid selection */
    if (selected_row < 0 || selected_row >= app_data.numSamples) {
        return;  /* No valid selection */
    }
    
    /* Create the popup menu if it doesn't exist */
    if (!popup_menu) {
        popup_menu = XmCreatePopupMenu(app_data.sampleGrid, "sample_popup", NULL, 0);
        
        /* Create "Receive as AIF" menu item */
        str = XmStringCreateLocalized("Receive as AIF...");
        receive_button = XtVaCreateManagedWidget(
            "receive_aif",            /* Widget name */
            xmPushButtonWidgetClass,  /* Widget class */
            popup_menu,               /* Parent widget */
            XmNlabelString, str,      /* Label text */
            NULL);                    /* Terminate list */
        XmStringFree(str);
        
        /* Add callback for Receive button */
        XtAddCallback(receive_button, XmNactivateCallback, receive_aif_callback, NULL);
        
        /* Create separator */
        separator = XtVaCreateManagedWidget(
            "separator",              /* Widget name */
            xmSeparatorWidgetClass,   /* Widget class */
            popup_menu,               /* Parent widget */
            NULL);                    /* Terminate list */
        
        /* Create "Delete" menu item */
        str = XmStringCreateLocalized("Delete Sample");
        delete_button = XtVaCreateManagedWidget(
            "delete_sample",          /* Widget name */
            xmPushButtonWidgetClass,  /* Widget class */
            popup_menu,               /* Parent widget */
            XmNlabelString, str,      /* Label text */
            NULL);                    /* Terminate list */
        XmStringFree(str);
        
        /* Add callback for Delete button */
        XtAddCallback(delete_button, XmNactivateCallback, delete_sample_callback, NULL);
    }
    
    /* Display the popup menu */
    XmMenuPosition(popup_menu, bevent);
    XtManageChild(popup_menu);
    
    /* Set continue_to_dispatch to FALSE to prevent further processing */
    *continue_to_dispatch = False;
}

/* Host Adapter option callback */
void ha_option_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
    int ha_id;
    XmString str;
    char buffer[32];
    
    /* Get the host adapter ID from client data */
    ha_id = (int)(long)client_data;
    
    /* Store the selected value */
    app_data.currentHA = ha_id;
    
    /* Update the option menu label to reflect the selection */
    sprintf(buffer, "Host Adapter: %d", ha_id);
    str = XmStringCreateLocalized(buffer);
    XtVaSetValues(app_data.haOption, XmNlabelString, str, NULL);
    XmStringFree(str);
    
    update_status("Selected Host Adapter: %d", ha_id);
}

/* Target ID option callback */
void id_option_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
    int id;
    XmString str;
    char buffer[32];
    
    /* Get the target ID from client data */
    id = (int)(long)client_data;
    
    /* Store the selected value */
    app_data.currentID = id;
    
    /* Update the option menu label to reflect the selection */
    sprintf(buffer, "Target ID: %d", id);
    str = XmStringCreateLocalized(buffer);
    XtVaSetValues(app_data.idOption, XmNlabelString, str, NULL);
    XmStringFree(str);
    
    update_status("Selected Target ID: %d", id);
}

/* Helper function to select all files in the list */
void select_all_files_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Widget list;
    int item_count;
    int i;
    
    list = (Widget)client_data;
    
    /* Get the number of items in the list */
    XtVaGetValues(list, XmNitemCount, &item_count, NULL);
    
    /* Select all items */
    for (i = 0; i < item_count; i++) {
        XmListSelectPos(list, i + 1, False);  /* Positions are 1-based */
    }
}

/* ID OK button callback */
void id_ok_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
    IdDialogData *data;
    char *id_value;
    int start_id;
    
    data = (IdDialogData *)client_data;
    
    /* Get the starting ID */
    id_value = XmTextFieldGetString(data->text);
    start_id = atoi(id_value);
    XtFree(id_value);
    
    /* Validate the ID */
    if (start_id < 0 || start_id > 127) {
        show_message_dialog(app_data.mainWindow, "Invalid ID",
                          "Starting sample ID must be between 0 and 127.",
                          XmDIALOG_ERROR);
        return;
    }
    
    /* Check if there's enough room */
    if (start_id + data->file_count > 128) {
        show_message_dialog(app_data.mainWindow, "Invalid ID Range",
                          "Not enough sample slots for all files.",
                          XmDIALOG_ERROR);
        return;
    }
    
    /* Upload the files */
    upload_multiple_aif_files(data->filenames, data->file_count, start_id);
    
    /* Destroy the dialog */
    XtDestroyWidget(data->dialog);
    
    /* Free memory */
    XtFree((char *)data);
}

/* ID Cancel button callback */
void id_cancel_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
    IdDialogData *data;
    int i;
    
    data = (IdDialogData *)client_data;
    
    /* Free the filenames */
    for (i = 0; i < data->file_count; i++) {
        XtFree(data->filenames[i]);
    }
    XtFree((char *)data->filenames);
    
    /* Destroy the dialog */
    XtDestroyWidget(data->dialog);
    
    /* Free memory */
    XtFree((char *)data);
}

/* Callback for handling multiple file selections */
void multi_file_ok_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
    MultiFileCallbackData *data;
    XmFileSelectionBoxCallbackStruct *cbs;
    Widget list;
    Widget dialog;
    int selected_count;
    int *selected_items;
    XmString *items;
    int item_count;
    char **filenames;
    char *dirname;
    int i;
    int next_id;
    XmString dir_xmstr;
    Widget id_dialog;
    Widget id_form;
    Widget id_label;
    Widget id_text;
    Widget id_ok;
    Widget id_cancel;
    Arg id_args[10];
    int arg_n;
    char buffer[32];
    IdDialogData *id_data;
    
    /* Extract callback data */
    data = (MultiFileCallbackData *)client_data;
    cbs = (XmFileSelectionBoxCallbackStruct *)call_data;
    list = data->file_list;
    dialog = data->dialog;
    
    /* Get the directory path */
    XtVaGetValues(dialog, XmNdirectory, &dir_xmstr, NULL);
    XmStringGetLtoR(dir_xmstr, XmSTRING_DEFAULT_CHARSET, &dirname);
    XmStringFree(dir_xmstr);
    
    /* Get the selected items */
    if (!XmListGetSelectedPos(list, &selected_items, &selected_count)) {
        /* No selections */
        show_message_dialog(app_data.mainWindow, "Selection Error", 
                           "Please select at least one file.",
                           XmDIALOG_WARNING);
        XtFree(dirname);
        return;
    }
    
    /* Get all list items */
    XtVaGetValues(list, 
                 XmNitems, &items,
                 XmNitemCount, &item_count,
                 NULL);
    
    /* Allocate space for the filenames */
    filenames = (char **)XtMalloc(selected_count * sizeof(char *));
    
    for (i = 0; i < selected_count; i++) {
    char *filename;
    char full_path[MAX_PATH];
    
    /* Convert XmString to C string */
    XmStringGetLtoR(items[selected_items[i] - 1], XmSTRING_DEFAULT_CHARSET, &filename);
    
    /* Check if filename already contains a full path */
    if (filename[0] == '/') {
        /* It's already a full path, use it directly */
        strncpy(full_path, filename, MAX_PATH - 1);
    } else {
        /* Ensure dirname doesn't end with slash before concatenating */
        int dir_len = strlen(dirname);
        if (dir_len > 0 && dirname[dir_len - 1] == '/') {
            /* Directory ends with slash, no need to add one */
            sprintf(full_path, "%s%s", dirname, filename);
        } else {
            /* Add slash between directory and filename */
            sprintf(full_path, "%s/%s", dirname, filename);
        }
    }
    
    full_path[MAX_PATH - 1] = '\0';
    
    /* Store the filename */
    filenames[i] = XtMalloc(strlen(full_path) + 1);
    strcpy(filenames[i], full_path);
    
    /* Free temporary strings */
    XtFree(filename);
    }
    
    /* Free selected items array */
    XtFree((char *)selected_items);
    
    /* Free directory name */
    XtFree(dirname);
    
    /* Calculate next available sample ID */
    next_id = 0;
    
    /* If we have samples, find the next available ID */
    if (app_data.numSamples > 0) {
        int j;
        int highest_id = -1;
        
        for (j = 0; j < app_data.numSamples; j++) {
            if (app_data.samples[j].id > highest_id) {
                highest_id = app_data.samples[j].id;
            }
        }
        
        next_id = highest_id + 1;
    }
    
    /* Unmanage the dialog */
    XtUnmanageChild(dialog);
    
    /* Create dialog shell */
    id_dialog = XtVaCreatePopupShell(
        "id_dialog",
        transientShellWidgetClass, app_data.mainWindow,
        XmNtitle, "Select Starting Sample ID",
        NULL);
    
    /* Create form */
    arg_n = 0;
    XtSetArg(id_args[arg_n], XmNfractionBase, 100); arg_n++;
    id_form = XtCreateManagedWidget("id_form", 
                                   xmFormWidgetClass,
                                   id_dialog, 
                                   id_args, arg_n);
    
    /* Create label */
    arg_n = 0;
    XtSetArg(id_args[arg_n], XmNtopAttachment, XmATTACH_FORM); arg_n++;
    XtSetArg(id_args[arg_n], XmNtopOffset, 10); arg_n++;
    XtSetArg(id_args[arg_n], XmNleftAttachment, XmATTACH_FORM); arg_n++;
    XtSetArg(id_args[arg_n], XmNleftOffset, 10); arg_n++;
    XtSetArg(id_args[arg_n], XmNrightAttachment, XmATTACH_FORM); arg_n++;
    XtSetArg(id_args[arg_n], XmNrightOffset, 10); arg_n++;
    XtSetArg(id_args[arg_n], XmNlabelString, 
            XmStringCreateLocalized("Enter starting sample ID for upload:")); arg_n++;
    
    id_label = XtCreateManagedWidget("id_label", 
                                    xmLabelWidgetClass,
                                    id_form, 
                                    id_args, arg_n);
    
    /* Create text field */
    sprintf(buffer, "%d", next_id);
    arg_n = 0;
    XtSetArg(id_args[arg_n], XmNtopAttachment, XmATTACH_WIDGET); arg_n++;
    XtSetArg(id_args[arg_n], XmNtopWidget, id_label); arg_n++;
    XtSetArg(id_args[arg_n], XmNtopOffset, 10); arg_n++;
    XtSetArg(id_args[arg_n], XmNleftAttachment, XmATTACH_FORM); arg_n++;
    XtSetArg(id_args[arg_n], XmNleftOffset, 10); arg_n++;
    XtSetArg(id_args[arg_n], XmNrightAttachment, XmATTACH_FORM); arg_n++;
    XtSetArg(id_args[arg_n], XmNrightOffset, 10); arg_n++;
    XtSetArg(id_args[arg_n], XmNvalue, buffer); arg_n++;
    
    id_text = XtCreateManagedWidget("id_text", 
                                   xmTextFieldWidgetClass,
                                   id_form, 
                                   id_args, arg_n);
    
    /* Create OK button */
    arg_n = 0;
    XtSetArg(id_args[arg_n], XmNtopAttachment, XmATTACH_WIDGET); arg_n++;
    XtSetArg(id_args[arg_n], XmNtopWidget, id_text); arg_n++;
    XtSetArg(id_args[arg_n], XmNtopOffset, 20); arg_n++;
    XtSetArg(id_args[arg_n], XmNbottomAttachment, XmATTACH_FORM); arg_n++;
    XtSetArg(id_args[arg_n], XmNbottomOffset, 10); arg_n++;
    XtSetArg(id_args[arg_n], XmNleftAttachment, XmATTACH_POSITION); arg_n++;
    XtSetArg(id_args[arg_n], XmNleftPosition, 20); arg_n++;
    XtSetArg(id_args[arg_n], XmNrightAttachment, XmATTACH_POSITION); arg_n++;
    XtSetArg(id_args[arg_n], XmNrightPosition, 40); arg_n++;
    XtSetArg(id_args[arg_n], XmNlabelString, 
            XmStringCreateLocalized("OK")); arg_n++;
    
    id_ok = XtCreateManagedWidget("id_ok", 
                                 xmPushButtonWidgetClass,
                                 id_form, 
                                 id_args, arg_n);
    
    /* Create Cancel button */
    arg_n = 0;
    XtSetArg(id_args[arg_n], XmNtopAttachment, XmATTACH_WIDGET); arg_n++;
    XtSetArg(id_args[arg_n], XmNtopWidget, id_text); arg_n++;
    XtSetArg(id_args[arg_n], XmNtopOffset, 20); arg_n++;
    XtSetArg(id_args[arg_n], XmNbottomAttachment, XmATTACH_FORM); arg_n++;
    XtSetArg(id_args[arg_n], XmNbottomOffset, 10); arg_n++;
    XtSetArg(id_args[arg_n], XmNleftAttachment, XmATTACH_POSITION); arg_n++;
    XtSetArg(id_args[arg_n], XmNleftPosition, 60); arg_n++;
    XtSetArg(id_args[arg_n], XmNrightAttachment, XmATTACH_POSITION); arg_n++;
    XtSetArg(id_args[arg_n], XmNrightPosition, 80); arg_n++;
    XtSetArg(id_args[arg_n], XmNlabelString, 
            XmStringCreateLocalized("Cancel")); arg_n++;
    
    id_cancel = XtCreateManagedWidget("id_cancel", 
                                     xmPushButtonWidgetClass,
                                     id_form, 
                                     id_args, arg_n);
    
    /* Prepare callback data */
    id_data = (IdDialogData *)XtMalloc(sizeof(IdDialogData));
    id_data->dialog = id_dialog;
    id_data->text = id_text;
    id_data->filenames = filenames;
    id_data->file_count = selected_count;
    
    /* Add callbacks */
    XtAddCallback(id_ok, XmNactivateCallback, id_ok_callback, (XtPointer)id_data);
    XtAddCallback(id_cancel, XmNactivateCallback, id_cancel_callback, (XtPointer)id_data);
    
    /* Manage and show the dialog */
    XtManageChild(id_form);
    XtPopup(id_dialog, XtGrabNone);
    
    /* Free our file dialog callback data */
    XtFree((char *)data);
}


void send_multiple_aif_callback(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Widget file_dialog;
    Widget file_list;
    XmString filter;
    XmString title;
    MultiFileCallbackData *callback_data;
    
    /* Check if connected */
    if (!app_data.connected) {
        show_message_dialog(app_data.mainWindow, "Not Connected", 
                           "Please connect to a SMDI device first.",
                           XmDIALOG_WARNING);
        return;
    }
    
    /* Create a standard file selection dialog */
    file_dialog = XmCreateFileSelectionDialog(
        app_data.mainWindow,       /* Parent widget */
        "multi_aif_dialog",        /* Dialog name */
        NULL, 0);                  /* No arguments */
    
    /* Set dialog title */
    title = XmStringCreateLocalized("Select AIF Files");
    XtVaSetValues(
        XtParent(file_dialog),     /* Parent shell */
        XmNtitle, "Select AIF Files", /* Dialog title */
        NULL);                     /* Terminate list */
    
    /* Set file filter pattern */
    filter = XmStringCreateLocalized("*.aif");
    XtVaSetValues(
        file_dialog,
        XmNpattern, filter,
        XmNdialogTitle, title,
        NULL);
    
    XmStringFree(filter);
    XmStringFree(title);
    
    /* Get the file list widget */
    file_list = (Widget)XmSelectionBoxGetChild(file_dialog, XmDIALOG_LIST);
    
    /* Allow multiple selections in the file list */
    XtVaSetValues(
        file_list,
        XmNselectionPolicy, XmEXTENDED_SELECT,
        NULL);
    
    /* Change the OK button label to "Upload Selected" */
    XtVaSetValues(
        (Widget)XmSelectionBoxGetChild(file_dialog, XmDIALOG_OK_BUTTON),
        XmNlabelString, XmStringCreateLocalized("Upload Selected"),
        NULL);
    
    /* Prepare the callback data */
    callback_data = (MultiFileCallbackData *)XtMalloc(sizeof(MultiFileCallbackData));
    callback_data->dialog = file_dialog;
    callback_data->file_list = file_list;
    
    /* Add callbacks for the dialog buttons */
    XtAddCallback(file_dialog, XmNokCallback, multi_file_ok_callback, (XtPointer)callback_data);
    XtAddCallback(file_dialog, XmNcancelCallback, 
                 (XtCallbackProc)XtUnmanageChild, NULL);
    
    /* Show the dialog */
    XtManageChild(file_dialog);
}


/* Function to upload multiple AIF files */
void upload_multiple_aif_files(char **filenames, int file_count, int start_sample_id)
{
    int i;
    int success_count = 0;
    int failure_count = 0;
    char message[256];
    
    update_status("Starting upload of %d files from sample ID %d...", 
                 file_count, start_sample_id);
    
    /* Upload each file */
    for (i = 0; i < file_count; i++) {
        int current_id = start_sample_id + i;
        
        update_status("Uploading file %d of %d to sample ID %d...", 
                     i + 1, file_count, current_id);
        
        if (send_aif_file(filenames[i], current_id)) {
            success_count++;
        } else {
            failure_count++;
        }
        
        /* Free the filename */
        XtFree(filenames[i]);
    }
    
    /* Free the filenames array */
    XtFree((char *)filenames);
    
    /* Show results */
    sprintf(message, "Upload complete: %d successful, %d failed", 
           success_count, failure_count);
    update_status("%s", message);
    
    /* Show dialog with results */
    show_message_dialog(app_data.mainWindow, "Upload Results", 
                       message, XmDIALOG_INFORMATION);
    
    /* Refresh the sample list */
    clear_sample_list();
    refresh_sample_list();
}
