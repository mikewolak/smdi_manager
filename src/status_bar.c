/* src/status_bar.c - Creates the status bar */
#include "app_all.h"

void create_status_bar(Widget parent)
{
    Widget status_frame;
    Widget progress_form;
    Widget progress_frame;
    Widget progress_background;
    Widget progress_indicator;
    XmString str;
    Display *display;
    Colormap colormap;
    XColor blue_color, exact_color;
    Pixel blue_pixel;
    
    /* Get display and colormap */
    display = XtDisplay(parent);
    colormap = DefaultColormapOfScreen(XtScreen(parent));
    
    /* Allocate a blue color */
    if (XAllocNamedColor(display, colormap, "blue", &blue_color, &exact_color)) {
        blue_pixel = blue_color.pixel;
    } else {
        /* Fallback to a standard color if 'blue' isn't available */
        blue_pixel = BlackPixelOfScreen(XtScreen(parent));
    }
    
    /* Create a frame for the status bar */
    status_frame = XtVaCreateManagedWidget(
        "status_frame",            /* Widget name */
        xmFrameWidgetClass,        /* Widget class */
        parent,                    /* Parent widget */
        XmNshadowType, XmSHADOW_ETCHED_IN,
        XmNpaneMinimum, 60,        /* Minimum height - increased for progress area */
        XmNpaneMaximum, 80,        /* Maximum height */
        NULL);                     /* Terminate list */
    
    /* Remove the "Status" title label */
    
    /* Create the status label */
    str = XmStringCreateLocalized("Ready");
    app_data.statusLabel = XtVaCreateManagedWidget(
        "status_label",            /* Widget name */
        xmLabelWidgetClass,        /* Widget class */
        status_frame,              /* Parent widget */
        XmNlabelString, str,       /* Label text */
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNtopAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNtopOffset, 5,
        XmNleftOffset, 5,
        XmNrightOffset, 5,
        NULL);                     /* Terminate list */
    XmStringFree(str);
    
    /* Create a form for the progress indicator */
    progress_form = XtVaCreateManagedWidget(
        "progress_form",           /* Widget name */
        xmFormWidgetClass,         /* Widget class */
        status_frame,              /* Parent widget */
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, app_data.statusLabel,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNbottomAttachment, XmATTACH_FORM,
        XmNtopOffset, 5,
        XmNleftOffset, 5,
        XmNrightOffset, 5,
        XmNbottomOffset, 5,
        NULL);                     /* Terminate list */
    
    /* Create a frame for the progress bar */
    progress_frame = XtVaCreateManagedWidget(
        "progress_frame",          /* Widget name */
        xmFrameWidgetClass,        /* Widget class */
        progress_form,             /* Parent widget */
        XmNshadowType, XmSHADOW_ETCHED_IN,
        XmNtopAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNbottomAttachment, XmATTACH_FORM,
        NULL);                     /* Terminate list */
    
    /* Create the progress background (white area) */
    progress_background = XtVaCreateManagedWidget(
        "progress_background",     /* Widget name */
        xmDrawingAreaWidgetClass,  /* Widget class */
        progress_frame,            /* Parent widget */
        XmNheight, 20,             /* Fixed height */
        XmNbackground, WhitePixelOfScreen(XtScreen(parent)),
        NULL);                     /* Terminate list */
    
    /* Create the progress indicator (colored portion) */
    progress_indicator = XtVaCreateManagedWidget(
        "progress_indicator",      /* Widget name */
        xmDrawingAreaWidgetClass,  /* Widget class */
        progress_background,       /* Parent widget */
        XmNheight, 20,             /* Same height as background */
        XmNwidth, 1,               /* Start with minimal width */
        XmNbackground, blue_pixel, /* Use the allocated blue color */
        NULL);                     /* Terminate list */
    
    /* Store the progress widgets in app_data */
    app_data.progressBackground = progress_background;
    app_data.progressIndicator = progress_indicator;
    app_data.progressVisible = 0;  /* Initially not visible */
    
    /* Make the progress bar initially invisible */
    XtUnmanageChild(progress_form);
}


/* Update the status message */
void update_status(const char *format, ...)
{
    va_list args;
    XmString str;
    char buffer[256];
    
    /* Format the message */
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    
    /* Store the message */
    strncpy(app_data.statusMessage, buffer, sizeof(app_data.statusMessage) - 1);
    app_data.statusMessage[sizeof(app_data.statusMessage) - 1] = '\0';
    
    /* Update the status label */
    str = XmStringCreateLocalized(buffer);
    XtVaSetValues(app_data.statusLabel, XmNlabelString, str, NULL);
    XmStringFree(str);
    
    /* Force a UI update */
    XmUpdateDisplay(app_data.mainWindow);
}

void update_device_info(const char *name, const char *vendor)
{
    XmString str;
    char buffer[256];
    
    /* Store the information */
    strncpy(app_data.deviceName, name, sizeof(app_data.deviceName) - 1);
    app_data.deviceName[sizeof(app_data.deviceName) - 1] = '\0';
    
    strncpy(app_data.deviceVendor, vendor, sizeof(app_data.deviceVendor) - 1);
    app_data.deviceVendor[sizeof(app_data.deviceVendor) - 1] = '\0';
    
    /* Format the display string - space added at the beginning for better visibility */
    sprintf(buffer, "%s %s", vendor, name);
    
    /* Update the device info label */
    str = XmStringCreateLocalized(buffer);
    XtVaSetValues(app_data.deviceInfoLabel, XmNlabelString, str, NULL);
    XmStringFree(str);
}

/* Show a message dialog */
void show_message_dialog(Widget parent, const char *title, const char *message, int type)
{
    Widget dialog;
    XmString t_str;
    XmString m_str;
    
    /* Create message strings */
    t_str = XmStringCreateLocalized((char *)title);
    m_str = XmStringCreateLocalized((char *)message);
    
    /* Create the dialog based on type */
    if (type == XmDIALOG_ERROR) {
        dialog = XmCreateErrorDialog(parent, "error_dialog", NULL, 0);
    } else if (type == XmDIALOG_WARNING) {
        dialog = XmCreateWarningDialog(parent, "warning_dialog", NULL, 0);
    } else {
        dialog = XmCreateInformationDialog(parent, "info_dialog", NULL, 0);
    }
    
    /* Set dialog properties */
    XtVaSetValues(
        dialog,
        XmNdialogTitle, t_str,
        XmNmessageString, m_str,
        NULL);
    
    /* Hide Cancel and Help buttons */
    XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
    XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
    
    /* Free strings */
    XmStringFree(t_str);
    XmStringFree(m_str);
    
    /* Show the dialog */
    XtManageChild(dialog);
}

void show_progress(int percent, const char *message)
{
    Widget parent;
    Dimension total_width;
    int indicator_width;
    
    /* Store the message but don't display it in status label during progress */
    strncpy(app_data.statusMessage, message, sizeof(app_data.statusMessage) - 1);
    app_data.statusMessage[sizeof(app_data.statusMessage) - 1] = '\0';
    
    /* If progress bar not yet visible, show it */
    if (!app_data.progressVisible) {
        /* Get the parent of the progress background */
        parent = XtParent(XtParent(app_data.progressBackground));
        XtManageChild(parent);
        app_data.progressVisible = 1;
    }
    
    /* Get the width of the background widget */
    XtVaGetValues(app_data.progressBackground, XmNwidth, &total_width, NULL);
    
    /* Calculate the width of the indicator based on percentage */
    indicator_width = (total_width * percent) / 100;
    if (indicator_width < 1) indicator_width = 1;
    
    /* Update the width of the progress indicator */
    XtVaSetValues(app_data.progressIndicator, XmNwidth, indicator_width, NULL);
    
    /* Force a UI update */
    XmUpdateDisplay(app_data.mainWindow);
}

void hide_progress(void)
{
    if (app_data.progressVisible) {
        /* Get the parent of the progress background */
        Widget parent;
        
        parent = XtParent(XtParent(app_data.progressBackground));
        XtUnmanageChild(parent);
        app_data.progressVisible = 0;
        
        /* Restore the status message now that the progress bar is gone */
        update_status("%s", app_data.statusMessage);
    }
}
