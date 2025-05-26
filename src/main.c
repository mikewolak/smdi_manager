/* src/main.c - Add more debugging */
#include "app_all.h"
#include <stdarg.h>

/* Global variables */
Widget toplevel;
XtAppContext app_context;
AppData app_data;

/* Debug logging function with line markers */
void main_log(const char *format, ...)
{
    FILE *log_file;
    va_list args;
    
    log_file = fopen("/tmp/smdi_manager.log", "a");
    if (log_file) {
        va_start(args, format);
        fprintf(log_file, "DEBUG: ");
        vfprintf(log_file, format, args);
        va_end(args);
        fprintf(log_file, "\n");
        fclose(log_file);
    }
}

/* Complete IRIX 4DWM Motif styling */
static String fallback_resources[] = {
    "*useSchemes: all",
    "*sgiMode: true",
    "*background: #7c8b9e",
    "*foreground: Black",
    "*XmText.background: #ffcece", 
    "*XmTextField.background: #ffcece",
    "*XmScrolledText.background: #ffcece",
    "*XmList.background: White",
    
    /* Button styling */
    "*XmPushButton.shadowThickness: 2",
    "*XmPushButton.arm3dColor: #c0c0c0",
    "*XmPushButton.background: #d4d0c8",
    "*XmPushButton.highlightThickness: 1",
    "*XmPushButton.highlightColor: #ffffff",
    "*XmPushButton.marginLeft: 4",
    "*XmPushButton.marginRight: 4",
    "*XmPushButton.marginTop: 2",
    "*XmPushButton.marginBottom: 2",
    
    /* Frame styling */
    "*XmFrame.shadowThickness: 2",
    "*XmFrame.shadowType: SHADOW_ETCHED_IN",
    
    /* Form styling */
    "*XmForm.horizontalSpacing: 4",
    "*XmForm.verticalSpacing: 4",
    "*XmForm.fractionBase: 100",
    
    /* Scroll bars */
    "*XmScrollBar*width: 20",
    "*XmScrollBar*height: 20",
    "*XmScrollBar*background: #d4d0c8",
    
    /* Menu styling */
    "*XmMenuShell*background: #d4d0c8",
    "*XmCascadeButton*background: #d4d0c8",
    "*XmCascadeButtonGadget*background: #d4d0c8",
    
    /* Separator styling */
    "*XmSeparator*shadowThickness: 1",
    
    /* Default styling */
    "*highlightThickness: 1",
    "*shadowThickness: 2",
    "*fontList: -adobe-helvetica-medium-r-normal--14-*-*-*-*-*-iso8859-1",
    NULL
};

int main(int argc, char *argv[])
{
    main_log("SMDI Manager starting - DEBUG BUILD");
    
    /* Initialize the application data */
    main_log("Initializing app_data");
    memset(&app_data, 0, sizeof(AppData));
    app_data.connected = 0;
    app_data.currentHA = 0;
    app_data.currentID = 0;
    app_data.numSamples = 0;
    app_data.operationInProgress = 0;
    strcpy(app_data.statusMessage, "Ready");
    
    /* Initialize the X Toolkit with IRIX specific resources */
    main_log("Starting XtVaAppInitialize");
    toplevel = XtVaAppInitialize(
        &app_context,        /* Application context */
        "Irix53_SMDI_Manager", /* Application class name */
        NULL, 0,             /* Command line options */
        &argc, argv,         /* Command line args */
        fallback_resources,  /* Fallback resources */
        XmNdeleteResponse, XmDO_NOTHING,  /* Handle WM_DELETE_WINDOW */
        XmNmappedWhenManaged, False,      /* For proper window placement */
        NULL);               /* Terminate list */
    
    main_log("X Toolkit initialized");
    
    /* Add callback for WM_DELETE_WINDOW */
    main_log("Adding WM_DELETE_WINDOW callback");
    XmAddWMProtocolCallback(
        toplevel,
        XmInternAtom(XtDisplay(toplevel), "WM_DELETE_WINDOW", False),
        exit_callback,
        NULL);
    
    main_log("WM_DELETE_WINDOW callback added");
    
    /* Create the main window */
    main_log("About to create main window");
    create_main_window(toplevel);
    main_log("Main window creation returned");
    
    /* Realize and enter the main loop */
    main_log("About to realize widgets");
    XtRealizeWidget(toplevel);
    main_log("Widgets realized");
    
    XtMapWidget(toplevel);  /* Map explicitly since mappedWhenManaged is False */
    main_log("Toplevel mapped");
    
    /* Initialize SMDI */
    main_log("Initializing SMDI");
    if (SMDI_Init()) {
        main_log("SMDI initialized successfully");
        update_status("SMDI initialized. Ready to connect to a device.");
    } else {
        main_log("SMDI initialization failed");
        update_status("SMDI initialization failed!");
        show_message_dialog(toplevel, "Error", 
                           "Failed to initialize SMDI. SCSI subsystem may not be available.", 
                           XmDIALOG_ERROR);
    }
    
    main_log("Entering main loop");
    XtAppMainLoop(app_context);
    
    return 0;  /* Never reached */
}
