/* src/window.c - Modified for SMDI Sampler Manager */
#include "app_all.h"
#include <stdarg.h>

/* Debug logging function */
void window_log(const char *format, ...)
{
    FILE *log_file;
    va_list args;
    
    log_file = fopen("/tmp/smdi_manager.log", "a");
    if (log_file) {
        va_start(args, format);
        vfprintf(log_file, format, args);
        va_end(args);
        fprintf(log_file, "\n");
        fclose(log_file);
    }
}

void create_main_window(Widget parent)
{
    Widget main_window;
    Widget main_form;
    Widget paned_window;
    
    main_log("create_main_window: Starting");
    
    /* Create a main window widget as a child of the shell */
    main_window = XtVaCreateManagedWidget(
        "main_window",             /* Widget name */
        xmMainWindowWidgetClass,   /* Widget class */
        parent,                    /* Parent widget */
        XmNshowSeparator, True,    /* Show separator between menu and work area */
        NULL);                     /* Terminate list */
    
    main_log("create_main_window: Created main_window widget");
    
    /* Store the main window widget in app_data */
    app_data.mainWindow = main_window;
    
    /* Create the menu bar */
    main_log("create_main_window: Creating menu bar");
    create_menu_bar(main_window);
    main_log("create_main_window: Menu bar created");
    
    /* Create a form widget to contain the application */
    main_form = XtVaCreateManagedWidget(
        "main_form",               /* Widget name */
        xmFormWidgetClass,         /* Widget class */
        main_window,               /* Parent widget */
        XmNfractionBase, 100,      /* Divide form into 100 units */
        XmNshadowThickness, 0,     /* No shadow around form */
        XmNwidth, 768,             /* Initial width */
        XmNheight, 480,            /* Initial height */
        NULL);                     /* Terminate list */
    
    main_log("create_main_window: Created main_form widget");
    
    /* Create a paned window for better layout control */
    paned_window = XtVaCreateManagedWidget(
        "paned_window",            /* Widget name */
        xmPanedWindowWidgetClass,  /* Widget class */
        main_form,                 /* Parent widget */
        XmNtopAttachment, XmATTACH_FORM,
        XmNbottomAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNsashWidth, 8,           /* Width of sash between panes */
        XmNsashHeight, 8,          /* Height of sash between panes */
        NULL);                     /* Terminate list */
    
    main_log("create_main_window: Created paned_window widget");
    
    /* Create connection panel in the top pane */
    main_log("create_main_window: Creating connection panel");
    create_connection_panel(paned_window);
    main_log("create_main_window: Connection panel created");
    
    /* Create sample list in the middle pane */
    main_log("create_main_window: Creating sample list");
    create_sample_list(paned_window);
    main_log("create_main_window: Sample list created");
    
    /* Create status bar in the bottom pane */
    main_log("create_main_window: Creating status bar");
    create_status_bar(paned_window);
    main_log("create_main_window: Status bar created");
    
    /* Set the work area */
    XtVaSetValues(
        main_window,               /* Widget to set */
        XmNworkWindow, main_form,  /* Work area widget */
        NULL);                     /* Terminate list */
    
    main_log("create_main_window: Set work area, completed");
}

void create_connection_panel(Widget parent)
{
    Widget connection_frame;
    Widget connection_form;
    Widget ha_label;
    Widget id_label;
    Widget scan_button;
    Widget device_frame;
    Widget title_label;
    Widget device_title;
    XmString str;
    int i;
    char buffer[8];
    Widget ha_option_menu;
    Widget id_option_menu;
    Widget ha_pulldown;
    Widget id_pulldown;
    Widget ha_button;
    Widget id_button;
    Arg args[10];
    int n;
    
    main_log("create_connection_panel: Starting");
    
    /* Create a frame for the connection panel */
    connection_frame = XtVaCreateManagedWidget(
        "connection_frame",        /* Widget name */
        xmFrameWidgetClass,        /* Widget class */
        parent,                    /* Parent widget */
        XmNshadowType, XmSHADOW_ETCHED_IN,
        XmNpaneMinimum, 90,        /* Minimum height */
        XmNpaneMaximum, 120,       /* Maximum height */
        NULL);                     /* Terminate list */
    
    main_log("create_connection_panel: Created connection_frame");
    
    /* Create a form inside the frame */
    connection_form = XtVaCreateManagedWidget(
        "connection_form",         /* Widget name */
        xmFormWidgetClass,         /* Widget class */
        connection_frame,          /* Parent widget */
        XmNfractionBase, 100,      /* Divide form into 100 units */
        NULL);                     /* Terminate list */
    
    main_log("create_connection_panel: Created connection_form");
    
    /* Create title label for connection panel */
    main_log("create_connection_panel: Creating title label");
    str = XmStringCreateLocalized("Device Connection");
    
    title_label = XtVaCreateManagedWidget(
        "title_label",             /* Widget name */
        xmLabelWidgetClass,        /* Widget class */
        connection_form,           /* Parent widget */
        XmNlabelString, str,       /* Label text */
        XmNtopAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNtopOffset, 2,
        XmNalignment, XmALIGNMENT_CENTER,
        NULL);                     /* Terminate list */
    
    XmStringFree(str);
    main_log("create_connection_panel: Created title_label");
    
    /* Create Host Adapter (HA) label */
    str = XmStringCreateLocalized("Host Adapter:");
    ha_label = XtVaCreateManagedWidget(
        "ha_label",                /* Widget name */
        xmLabelWidgetClass,        /* Widget class */
        connection_form,           /* Parent widget */
        XmNlabelString, str,       /* Label text */
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, title_label,
        XmNleftAttachment, XmATTACH_FORM,
        XmNtopOffset, 10,
        XmNleftOffset, 10,
        NULL);                     /* Terminate list */
    
    XmStringFree(str);
    main_log("create_connection_panel: Created HA label");

    /* Create HA option menu with multiple values */
    main_log("create_connection_panel: Creating HA pulldown menu");
    
    /* Create pulldown menu */
    ha_pulldown = XmCreatePulldownMenu(connection_form, "ha_menu", NULL, 0);
    
    /* Add buttons to the pulldown for values 0-7 */

/* Add buttons to the pulldown for values 0-7 */
for (i = 0; i < 8; i++) {
    sprintf(buffer, "%d", i);
    ha_button = XtVaCreateManagedWidget(
        buffer,                    /* Widget name */
        xmPushButtonWidgetClass,   /* Widget class */
        ha_pulldown,               /* Parent widget */
        NULL);                     /* No args needed */
        
    /* Add callback to set the current HA - passing the index as client_data */
    XtAddCallback(ha_button, XmNactivateCallback, ha_option_callback, (XtPointer)(long)i);
}
    
    main_log("create_connection_panel: Creating HA option menu");
    
    /* Create option menu button with initial selection */
    n = 0;
    str = XmStringCreateLocalized("Host Adapter");
    XtSetArg(args[n], XmNlabelString, str); n++;
    XtSetArg(args[n], XmNsubMenuId, ha_pulldown); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, title_label); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, ha_label); n++;
    XtSetArg(args[n], XmNtopOffset, 5); n++;
    XtSetArg(args[n], XmNleftOffset, 5); n++;
    
    ha_option_menu = XmCreateOptionMenu(connection_form, "ha_option", args, n);
    XtManageChild(ha_option_menu);
    XmStringFree(str);
    
    /* Store in app data */
    app_data.haOption = ha_option_menu;
    app_data.currentHA = 0;
    main_log("create_connection_panel: Created HA option menu");
    
    /* Create Target ID label */
    str = XmStringCreateLocalized("Target ID:");
    id_label = XtVaCreateManagedWidget(
        "id_label",                /* Widget name */
        xmLabelWidgetClass,        /* Widget class */
        connection_form,           /* Parent widget */
        XmNlabelString, str,       /* Label text */
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, title_label,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, ha_option_menu,
        XmNtopOffset, 10,
        XmNleftOffset, 15,
        NULL);                     /* Terminate list */
    
    XmStringFree(str);
    main_log("create_connection_panel: Created ID label");
    
    /* Create ID option menu with multiple values */
    main_log("create_connection_panel: Creating ID pulldown menu");
    
    /* Create pulldown menu */
    id_pulldown = XmCreatePulldownMenu(connection_form, "id_menu", NULL, 0);
    
for (i = 0; i < 8; i++) {
    sprintf(buffer, "%d", i);
    id_button = XtVaCreateManagedWidget(
        buffer,                    /* Widget name */
        xmPushButtonWidgetClass,   /* Widget class */
        id_pulldown,               /* Parent widget */
        NULL);                     /* No args needed */
        
    /* Add callback to set the current ID - passing the index as client_data */
    XtAddCallback(id_button, XmNactivateCallback, id_option_callback, (XtPointer)(long)i);
}
    main_log("create_connection_panel: Creating ID option menu");
    
    /* Create option menu button with initial selection */
    n = 0;
    str = XmStringCreateLocalized("Target ID");
    XtSetArg(args[n], XmNlabelString, str); n++;
    XtSetArg(args[n], XmNsubMenuId, id_pulldown); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, title_label); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, id_label); n++;
    XtSetArg(args[n], XmNtopOffset, 5); n++;
    XtSetArg(args[n], XmNleftOffset, 5); n++;
    
    id_option_menu = XmCreateOptionMenu(connection_form, "id_option", args, n);
    XtManageChild(id_option_menu);
    XmStringFree(str);
    
    /* Store in app data */
    app_data.idOption = id_option_menu;
    app_data.currentID = 0;
    main_log("create_connection_panel: Created ID option menu");
    
    /* Create Scan button */
    main_log("create_connection_panel: Creating Scan button");
    str = XmStringCreateLocalized("Scan");
    scan_button = XtVaCreateManagedWidget(
        "scan_button",             /* Widget name */
        xmPushButtonWidgetClass,   /* Widget class */
        connection_form,           /* Parent widget */
        XmNlabelString, str,       /* Button label */
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, title_label,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, id_option_menu,
        XmNtopOffset, 5,
        XmNleftOffset, 15,
        NULL);                     /* Terminate list */
    
    XmStringFree(str);
    main_log("create_connection_panel: Created Scan button");
    
    /* Add callback for the Scan button */
    XtAddCallback(scan_button, XmNactivateCallback, scan_callback, NULL);
    
    /* Create Connect button */
    main_log("create_connection_panel: Creating Connect button");
    str = XmStringCreateLocalized("Connect");
    app_data.connectButton = XtVaCreateManagedWidget(
        "connect_button",          /* Widget name */
        xmPushButtonWidgetClass,   /* Widget class */
        connection_form,           /* Parent widget */
        XmNlabelString, str,       /* Button label */
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, title_label,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, scan_button,
        XmNtopOffset, 5,
        XmNleftOffset, 10,
        NULL);                     /* Terminate list */
    
    XmStringFree(str);
    main_log("create_connection_panel: Created Connect button");
    
    /* Add callback for the Connect button */
    XtAddCallback(app_data.connectButton, XmNactivateCallback, connect_callback, NULL);
    
/* Create a frame for the device info */
device_frame = XtVaCreateManagedWidget(
    "device_frame",            /* Widget name */
    xmFrameWidgetClass,        /* Widget class */
    connection_form,           /* Parent widget */
    XmNshadowType, XmSHADOW_ETCHED_IN,
    XmNtopAttachment, XmATTACH_WIDGET,
    XmNtopWidget, ha_label,
    XmNleftAttachment, XmATTACH_FORM,
    XmNrightAttachment, XmATTACH_FORM,
    XmNbottomAttachment, XmATTACH_FORM,
    XmNtopOffset, 10,
    XmNleftOffset, 10,
    XmNrightOffset, 10,
    XmNbottomOffset, 5,
    NULL);                     /* Terminate list */

/* Create title label for device info frame */
str = XmStringCreateLocalized("Device Information");
device_title = XtVaCreateManagedWidget(
    "device_title",            /* Widget name */
    xmLabelWidgetClass,        /* Widget class */
    device_frame,              /* Parent widget */
    XmNlabelString, str,       /* Label text */
    XmNtopAttachment, XmATTACH_FORM,
    XmNleftAttachment, XmATTACH_FORM,
    XmNrightAttachment, XmATTACH_POSITION,  /* Changed to POSITION instead of FORM */
    XmNrightPosition, 40,                   /* Only use 40% of the width */
    XmNtopOffset, 2,
    XmNalignment, XmALIGNMENT_BEGINNING,    /* Changed to left-align */
    XmNleftOffset, 10,                      /* Add left padding */
    NULL);                     /* Terminate list */
XmStringFree(str);

/* Create device info label */
str = XmStringCreateLocalized("Not connected");
app_data.deviceInfoLabel = XtVaCreateManagedWidget(
    "device_info",             /* Widget name */
    xmLabelWidgetClass,        /* Widget class */
    device_frame,              /* Parent widget */
    XmNlabelString, str,       /* Label text */
    XmNalignment, XmALIGNMENT_BEGINNING,
    XmNtopAttachment, XmATTACH_FORM,        /* Changed to FORM instead of WIDGET */
    XmNleftAttachment, XmATTACH_POSITION,   /* Changed to POSITION instead of FORM */
    XmNleftPosition, 42,                    /* Start at 42% of the width */
    XmNrightAttachment, XmATTACH_FORM,
    XmNbottomAttachment, XmATTACH_FORM,
    XmNtopOffset, 2,                        /* Same top offset as title */
    XmNrightOffset, 5,
    XmNbottomOffset, 5,
    NULL);                     /* Terminate list */
    
    XmStringFree(str);
    main_log("create_connection_panel: Created device info label");
    main_log("create_connection_panel: Completed");
}
