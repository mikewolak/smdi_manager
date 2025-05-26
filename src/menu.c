/* src/menu.c - Creates the menu bar */
#include "app_all.h"

void create_menu_bar(Widget parent)
{
    Widget menu_bar;
    Widget file_menu;
    Widget operations_menu;
    Widget help_menu;
    Widget file_cascade;
    Widget operations_cascade;
    Widget help_cascade;
    Widget connect_button;
    Widget refresh_button;
    Widget separator;
    Widget exit_button;
    Widget send_aif_button;
    Widget send_multi_aif_button;
    Widget help_button;
    XmString str;
    
    /* Create the menu bar */
    menu_bar = XmCreateMenuBar(parent, "menu_bar", NULL, 0);
    XtManageChild(menu_bar);
    
    /* Create the File menu */
    file_menu = XmCreatePulldownMenu(menu_bar, "file_menu", NULL, 0);
    
    /* Create the File cascade button */
    file_cascade = XtVaCreateManagedWidget(
        "File",                    /* Button label */
        xmCascadeButtonWidgetClass, /* Widget class */
        menu_bar,                  /* Parent widget */
        XmNsubMenuId, file_menu,   /* Connect to menu */
        NULL);                     /* Terminate list */
    
/* Create the Connect button */
    str = XmStringCreateLocalized("Connect...");
    connect_button = XtVaCreateManagedWidget(
        "connect_menu",            /* Widget name */
        xmPushButtonWidgetClass,   /* Widget class */
        file_menu,                 /* Parent widget */
        XmNlabelString, str,       /* Button label */
        NULL);                     /* Terminate list */
    XmStringFree(str);
    
    /* Add the callback for the Connect button */
    XtAddCallback(connect_button, XmNactivateCallback, connect_callback, NULL);
    
    /* Create the Refresh List button */
    str = XmStringCreateLocalized("Refresh Sample List");
    refresh_button = XtVaCreateManagedWidget(
        "refresh_list",           /* Widget name */
        xmPushButtonWidgetClass,   /* Widget class */
        file_menu,                 /* Parent widget */
        XmNlabelString, str,       /* Button label */
        NULL);                     /* Terminate list */
    XmStringFree(str);
    
    /* Add the callback for the Refresh List button */
    XtAddCallback(refresh_button, XmNactivateCallback, refresh_callback, NULL);
    
    /* Create a separator */
    separator = XtVaCreateManagedWidget(
        "separator",               /* Widget name */
        xmSeparatorGadgetClass,    /* Widget class */
        file_menu,                 /* Parent widget */
        NULL);                     /* Terminate list */
    
    /* Create the Exit button */
    str = XmStringCreateLocalized("Exit");
    exit_button = XtVaCreateManagedWidget(
        "exit",                    /* Widget name */
        xmPushButtonWidgetClass,   /* Widget class */
        file_menu,                 /* Parent widget */
        XmNlabelString, str,       /* Button label */
        NULL);                     /* Terminate list */
    XmStringFree(str);
    
    /* Add the callback for the Exit button */
    XtAddCallback(exit_button, XmNactivateCallback, exit_callback, NULL);
    
    /* Create the Operations menu */
    operations_menu = XmCreatePulldownMenu(menu_bar, "operations_menu", NULL, 0);
    
    /* Create the Operations cascade button */
    str = XmStringCreateLocalized("Operations");
    operations_cascade = XtVaCreateManagedWidget(
        "operations",              /* Widget name */
        xmCascadeButtonWidgetClass, /* Widget class */
        menu_bar,                  /* Parent widget */
        XmNlabelString, str,       /* Button label */
        XmNsubMenuId, operations_menu, /* Connect to menu */
        NULL);                     /* Terminate list */
    XmStringFree(str);
    
    /* Create the Send AIF button */
    str = XmStringCreateLocalized("Send AIF File...");
    send_aif_button = XtVaCreateManagedWidget(
        "send_aif",                /* Widget name */
        xmPushButtonWidgetClass,   /* Widget class */
        operations_menu,           /* Parent widget */
        XmNlabelString, str,       /* Button label */
        NULL);                     /* Terminate list */
    XmStringFree(str);
    
    /* Add the callback for the Send AIF button */
    XtAddCallback(send_aif_button, XmNactivateCallback, send_aif_callback, NULL);

   /* Create the Send Multiple AIF button */
   str = XmStringCreateLocalized("Send Multiple AIF Files...");
   send_multi_aif_button = XtVaCreateManagedWidget(
    "send_multi_aif",          /* Widget name */
    xmPushButtonWidgetClass,   /* Widget class */
    operations_menu,           /* Parent widget */
    XmNlabelString, str,       /* Button label */
    NULL);                     /* Terminate list */
    XmStringFree(str);

    /* Add the callback for the Send Multiple AIF button */
    XtAddCallback(send_multi_aif_button, XmNactivateCallback, send_multiple_aif_callback, NULL);
    
    /* Create the Help menu */
    help_menu = XmCreatePulldownMenu(menu_bar, "help_menu", NULL, 0);
    
    /* Create the Help cascade button */
    str = XmStringCreateLocalized("Help");
    help_cascade = XtVaCreateManagedWidget(
        "help",                    /* Widget name */
        xmCascadeButtonWidgetClass, /* Widget class */
        menu_bar,                  /* Parent widget */
        XmNlabelString, str,       /* Button label */
        XmNsubMenuId, help_menu,   /* Connect to menu */
        NULL);                     /* Terminate list */
    XmStringFree(str);
    
    /* Set the Help menu as the help menu for the menu bar */
    XtVaSetValues(
        menu_bar,                  /* Widget to set */
        XmNmenuHelpWidget, help_cascade, /* Help menu widget */
        NULL);                     /* Terminate list */
    
    /* Create the Help button */
    str = XmStringCreateLocalized("About");
    help_button = XtVaCreateManagedWidget(
        "about",                   /* Widget name */
        xmPushButtonWidgetClass,   /* Widget class */
        help_menu,                 /* Parent widget */
        XmNlabelString, str,       /* Button label */
        NULL);                     /* Terminate list */
    XmStringFree(str);
    
    /* Add the callback for the Help button */
    XtAddCallback(help_button, XmNactivateCallback, help_callback, NULL);
}
