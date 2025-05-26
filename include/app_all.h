/* include/app_all.h - Modified for SMDI Sampler Manager */
#ifndef APP_ALL_H
#define APP_ALL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>  
#include <sys/time.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/RowColumn.h>
#include <Xm/MessageB.h>
#include <Xm/CascadeB.h>
#include <Xm/Protocols.h>
#include <Xm/DrawingA.h>
#include <Xm/FileSB.h>
#include <Xm/List.h>
#include <Xm/Frame.h>
#include <Xm/ToggleB.h>
#include <Xm/ScrolledW.h>
#include <Xm/PanedW.h>
#include <Xm/TextF.h>
#include <Xm/Separator.h>
#include <Xm/SeparatoG.h>
/* Remove ComboBox include */
/* #include <Xm/ComboBox.h> */

/* Include SMDI headers */
#include "smdi.h"
#include "smdi_sample.h"
#include "smdi_aif.h"

/* Include custom grid widget header */
#include "grid_widget.h"

#ifndef _NO_PROTO
extern Widget XmSelectionBoxGetChild(Widget widget, unsigned char type);
#else
extern Widget XmSelectionBoxGetChild();
#endif


/* Maximum number of samples to display */
#define MAX_SAMPLES 128
#define MAX_SCSI_DEVICES 16

typedef enum {
    MODE_NOT_CONNECTED,  /* Not connected to any device */
    MODE_CONNECTED       /* Connected to a SMDI device */
} AppMode;

/* Sample Data Structure for display */
typedef struct {
    int id;                  /* Sample ID */
    char name[256];          /* Sample name */
    int rate;                /* Sample rate in Hz */
    int length;              /* Sample length in frames */
    int bits;                /* Bits per sample */
    int channels;            /* Number of channels */
    int exists;              /* Whether sample exists */
} SampleInfo;


typedef struct {
    Widget mainWindow;       /* Main window widget */
    Widget sampleGrid;       /* Grid widget for samples (replacing sampleList) */
    Widget haOption;         /* Host adapter option menu */
    Widget idOption;         /* Target ID option menu */
    Widget connectButton;    /* Connect button */
    Widget deviceInfoLabel;  /* Device info label */
    Widget progressBackground; /* Background for progress bar */
    Widget progressIndicator;  /* Colored part of progress bar */
    int progressVisible;       /* Whether progress bar is visible */
    Widget statusLabel; 


    /* Connection and sample data */
    int connected;           /* Connection state */
    int currentHA;           /* Current host adapter */
    int currentID;           /* Current SCSI target ID */
    char deviceName[32];     /* Connected device name */
    char deviceVendor[16];   /* Connected device vendor */
    SampleInfo samples[MAX_SAMPLES];  /* Sample information array */
    int numSamples;          /* Number of samples in array */
    
    /* Progress tracking */
    int operationInProgress; /* Flag for ongoing operation */
    char statusMessage[256]; /* Current status message */
} AppData;

/* Structure for multiple file selections */
typedef struct {
    Widget dialog;
    Widget file_list;
} MultiFileCallbackData;

/* Structure for ID dialog data */
typedef struct {
    Widget dialog;
    Widget text;
    char **filenames;
    int file_count;
} IdDialogData;


/* Global application data */
extern AppData app_data;
extern Widget toplevel;
extern XtAppContext app_context;

/* Function declarations */
void create_main_window(Widget parent);
void create_menu_bar(Widget parent);
void create_connection_panel(Widget parent);
void create_sample_list(Widget parent);
void create_status_bar(Widget parent);

/* SMDI operations */
int scan_scsi_devices(int ha_id, char *device_names[], int *device_types);
int connect_to_device(int ha_id, int id);
int refresh_sample_list(void);
int receive_sample_as_aif(int sample_id, const char *filename);
int delete_sample(int sample_id);
int send_aif_file(const char *filename, int sample_id);

/* UI callbacks */
void exit_callback(Widget widget, XtPointer client_data, XtPointer call_data);
void help_callback(Widget widget, XtPointer client_data, XtPointer call_data);
void connect_callback(Widget widget, XtPointer client_data, XtPointer call_data);
void scan_callback(Widget widget, XtPointer client_data, XtPointer call_data);
void refresh_callback(Widget widget, XtPointer client_data, XtPointer call_data);
void send_aif_callback(Widget widget, XtPointer client_data, XtPointer call_data);
void file_selected_callback(Widget widget, XtPointer client_data, XtPointer call_data);
void sample_selected_callback(Widget widget, XtPointer client_data, XtPointer call_data);
void confirm_delete_callback(Widget widget, XtPointer client_data, XtPointer call_data);
void ha_option_callback(Widget widget, XtPointer client_data, XtPointer call_data);
void id_option_callback(Widget widget, XtPointer client_data, XtPointer call_data);

/* Context menu callbacks */
void sample_create_popup_menu(Widget widget, XtPointer client_data, XEvent *event, Boolean *continue_to_dispatch);
void receive_aif_callback(Widget widget, XtPointer client_data, XtPointer call_data);
void delete_sample_callback(Widget widget, XtPointer client_data, XtPointer call_data);

/* Utility functions */
void update_status(const char *format, ...);
void update_device_info(const char *name, const char *vendor);
void show_message_dialog(Widget parent, const char *title, const char *message, int type);
void show_progress(int percent, const char *message);
void clear_sample_list(void);
void add_sample_to_list(SampleInfo *sample);
void main_log(const char *format, ...);  /* Debug logging function */
void ha_button_callback(Widget widget, XtPointer client_data, XtPointer call_data);
void id_button_callback(Widget widget, XtPointer client_data, XtPointer call_data);
void hide_progress(void);  
void dialog_close_callback(Widget widget, XtPointer client_data, XtPointer call_data);
void show_sample_details(SampleInfo *sample);
void dialog_close_callback(Widget widget, XtPointer client_data, XtPointer call_data);
void select_sample_id_dialog(Widget parent, const char *filename);
void send_multiple_aif_callback(Widget widget, XtPointer client_data, XtPointer call_data);
void upload_multiple_aif_files(char **filenames, int file_count, int start_sample_id);
void send_multiple_aif_callback(Widget widget, XtPointer client_data, XtPointer call_data);
void upload_multiple_aif_files(char **filenames, int file_count, int start_sample_id);

#endif /* APP_ALL_H */
