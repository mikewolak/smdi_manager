/* grid_widget.c - Custom Grid Widget for IRIX 5.3 Motif 1.2 */
#include "grid_widget.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Internal function prototypes */
static void handle_expose(Widget widget, XtPointer client_data, XtPointer call_data);
static void handle_resize(Widget widget, XtPointer client_data, XtPointer call_data);
static void handle_button_press(Widget widget, XtPointer client_data, XEvent *event, Boolean *continue_to_dispatch);
static void handle_button_release(Widget widget, XtPointer client_data, XEvent *event, Boolean *continue_to_dispatch);

Widget create_grid_widget(Widget parent, int x, int y, int width, int height)
{
    Widget scrolled;
    Widget drawing;
    GridWidget *grid_data;
    Arg args[20];
    int n;
    XGCValues gc_values;
    Display *display;
    Colormap colormap;
    XColor color, exact;
    Pixel white_pixel, black_pixel, header_pixel, select_pixel;
    
    /* Create scrolled window container */
    n = 0;
    XtSetArg(args[n], XmNx, x); n++;
    XtSetArg(args[n], XmNy, y); n++;
    XtSetArg(args[n], XmNwidth, width); n++;
    XtSetArg(args[n], XmNheight, height); n++;
    XtSetArg(args[n], XmNscrollingPolicy, XmAUTOMATIC); n++;
    XtSetArg(args[n], XmNscrollBarDisplayPolicy, XmAS_NEEDED); n++;
    
    scrolled = XmCreateScrolledWindow(parent, "grid_scrolled", args, n);
    XtManageChild(scrolled);
    
    /* Create drawing area for grid */
    n = 0;
    XtSetArg(args[n], XmNwidth, width); n++;
    XtSetArg(args[n], XmNheight, height); n++;
    
    drawing = XmCreateDrawingArea(scrolled, "grid_drawing", args, n);
    
    /* Allocate grid data structure */
    grid_data = (GridWidget *)XtMalloc(sizeof(GridWidget));
    
    if (grid_data == NULL) {
        fprintf(stderr, "Failed to allocate grid data structure\n");
        return NULL;
    }
    
    /* Initialize grid data */
    memset(grid_data, 0, sizeof(GridWidget));
    grid_data->parent = parent;
    grid_data->scrolled = scrolled;
    grid_data->drawing = drawing;
    grid_data->num_columns = 0;
    grid_data->num_rows = 0;
    grid_data->header_height = 25;
    grid_data->row_height = 20;
    grid_data->selected_row = -1;
    
    /* Get display and colormap */
    display = XtDisplay(parent);
    colormap = DefaultColormapOfScreen(XtScreen(parent));
    
    /* Get standard colors */
    white_pixel = WhitePixelOfScreen(XtScreen(parent));
    black_pixel = BlackPixelOfScreen(XtScreen(parent));
    
    /* Allocate header background color (IRIX blue-grey) */
    if (XAllocNamedColor(display, colormap, "#7c8b9e", &color, &exact)) {
        header_pixel = color.pixel;
    } else {
        /* Fallback to light gray if IRIX blue not available */
        if (XAllocNamedColor(display, colormap, "light gray", &color, &exact)) {
            header_pixel = color.pixel;
        } else {
            header_pixel = white_pixel;
        }
    }
    
    /* Allocate selection background color - IRIX light blue selection */
    if (XAllocNamedColor(display, colormap, "#a0c8ff", &color, &exact)) {
        select_pixel = color.pixel;
    } else {
        /* Fallback to light blue if color not available */
        if (XAllocNamedColor(display, colormap, "light blue", &color, &exact)) {
            select_pixel = color.pixel;
        } else {
            select_pixel = white_pixel;
        }
    }
    
    /* Create GC for drawing text */
    gc_values.foreground = black_pixel;
    gc_values.background = white_pixel;
    grid_data->gc_text = XCreateGC(display, 
                                   RootWindowOfScreen(XtScreen(parent)),
                                   GCForeground | GCBackground,
                                   &gc_values);
    
    /* Create GC for drawing grid lines */
    gc_values.foreground = black_pixel;
    gc_values.line_width = 1;
    grid_data->gc_lines = XCreateGC(display,
                                   RootWindowOfScreen(XtScreen(parent)),
                                   GCForeground | GCLineWidth,
                                   &gc_values);
    
    /* Create GC for header background */
    gc_values.foreground = header_pixel;
    gc_values.background = white_pixel;
    grid_data->gc_header = XCreateGC(display,
                                    RootWindowOfScreen(XtScreen(parent)),
                                    GCForeground | GCBackground,
                                    &gc_values);
    
    /* Create GC for selection background */
    gc_values.foreground = select_pixel;
    gc_values.background = white_pixel;
    grid_data->gc_selection = XCreateGC(display,
                                       RootWindowOfScreen(XtScreen(parent)),
                                       GCForeground | GCBackground,
                                       &gc_values);
    
    /* Store grid data pointer in drawing area */
    XtVaSetValues(drawing, XmNuserData, (XtPointer)grid_data, NULL);
    
    /* Add event handlers */
    XtAddCallback(drawing, XmNexposeCallback, handle_expose, NULL);
    XtAddCallback(drawing, XmNresizeCallback, handle_resize, NULL);
    XtAddEventHandler(drawing, ButtonPressMask, False, 
                     (XtEventHandler)handle_button_press, NULL);
    XtAddEventHandler(drawing, ButtonReleaseMask, False, 
                     (XtEventHandler)handle_button_release, NULL);
    
    /* Finally, manage the drawing area */
    XtManageChild(drawing);
    
    return drawing;
}

/* Get grid data from widget */
GridWidget *get_grid_data(Widget grid)
{
    GridWidget *grid_data;
    
    XtVaGetValues(grid, XmNuserData, &grid_data, NULL);
    return grid_data;
}

/* Set column properties */
void grid_set_column(Widget grid, int col_index, const char *title, int width, char alignment)
{
    GridWidget *grid_data;
    
    grid_data = get_grid_data(grid);
    
    if (grid_data == NULL || col_index < 0 || col_index >= GRID_MAX_COLS) {
        return;
    }
    
    /* Update column information */
    strncpy(grid_data->columns[col_index].title, title, GRID_TEXT_LEN - 1);
    grid_data->columns[col_index].title[GRID_TEXT_LEN - 1] = '\0';
    grid_data->columns[col_index].width = width;
    grid_data->columns[col_index].alignment = alignment;
    
    /* Update number of columns if needed */
    if (col_index >= grid_data->num_columns) {
        grid_data->num_columns = col_index + 1;
    }
}

/* Set number of data rows */
void grid_set_num_rows(Widget grid, int num_rows)
{
    GridWidget *grid_data;
    Dimension width, height;
    int total_height;
    
    grid_data = get_grid_data(grid);
    
    if (grid_data == NULL || num_rows < 0 || num_rows > GRID_MAX_ROWS) {
        return;
    }
    
    grid_data->num_rows = num_rows;
    
    /* Adjust the drawing area size */
    XtVaGetValues(grid, XmNwidth, &width, NULL);
    
    total_height = grid_data->header_height + (grid_data->row_height * num_rows);
    
    XtVaSetValues(grid, XmNheight, total_height, NULL);
    
    /* Clear the selection if it's outside the new range */
    if (grid_data->selected_row >= num_rows) {
        grid_data->selected_row = -1;
    }
}

/* Set cell content */
void grid_set_cell(Widget grid, int row, int col, const char *text)
{
    GridWidget *grid_data;
    
    grid_data = get_grid_data(grid);
    
    if (grid_data == NULL || row < 0 || row >= GRID_MAX_ROWS ||
        col < 0 || col >= GRID_MAX_COLS) {
        return;
    }
    
    /* Update cell text */
    strncpy(grid_data->cells[row][col].text, text, GRID_TEXT_LEN - 1);
    grid_data->cells[row][col].text[GRID_TEXT_LEN - 1] = '\0';
}

/* Add select callback */
void grid_add_select_callback(Widget grid, XtCallbackProc callback, XtPointer client_data)
{
    XtAddCallback(grid, XmNinputCallback, callback, client_data);
}

/* Add double click callback */
void grid_add_double_click_callback(Widget grid, XtCallbackProc callback, XtPointer client_data)
{
    /* We'll implement this in handle_button_press */
    XtVaSetValues(grid, XmNuserData, (XtPointer)get_grid_data(grid), NULL);
}

/* Add right click callback */
void grid_add_right_click_callback(Widget grid, XtCallbackProc callback, XtPointer client_data)
{
    /* We'll implement this in handle_button_press */
    XtVaSetValues(grid, XmNuserData, (XtPointer)get_grid_data(grid), NULL);
}

/* Get currently selected row */
int grid_get_selected_row(Widget grid)
{
    GridWidget *grid_data;
    
    grid_data = get_grid_data(grid);
    
    if (grid_data == NULL) {
        return -1;
    }
    
    return grid_data->selected_row;
}

/* Select a row programmatically */
void grid_select_row(Widget grid, int row)
{
    GridWidget *grid_data;
    XEvent event;
    XmDrawingAreaCallbackStruct cb;
    
    grid_data = get_grid_data(grid);
    
    if (grid_data == NULL || row < 0 || row >= grid_data->num_rows) {
        return;
    }
    
    /* Update selection */
    grid_data->selected_row = row;
    
    /* Only redraw if window exists */
    if (XtWindow(grid)) {
        XClearWindow(XtDisplay(grid), XtWindow(grid));
        draw_grid(grid, NULL, NULL);
    }
    
    /* Call the selection callback */
    memset(&event, 0, sizeof(XEvent));
    memset(&cb, 0, sizeof(XmDrawingAreaCallbackStruct));
    
    cb.reason = XmCR_INPUT;
    cb.event = &event;
    
    XtCallCallbacks(grid, XmNinputCallback, (XtPointer)&cb);
}

/* ---- Internal functions ---- */

/* Handle expose events */
static void handle_expose(Widget widget, XtPointer client_data, XtPointer call_data)
{
    draw_grid(widget, client_data, call_data);
}

/* Handle resize events */
static void handle_resize(Widget widget, XtPointer client_data, XtPointer call_data)
{
    /* Just redraw the grid */
    draw_grid(widget, client_data, call_data);
}

/* Handle button press events */
static void handle_button_press(Widget widget, XtPointer client_data, XEvent *event, Boolean *continue_to_dispatch)
{
    GridWidget *grid_data;
    XButtonEvent *btn_event;
    int row, y_pos;
    static Time last_click_time = 0;
    static int last_click_row = -1;
    XmDrawingAreaCallbackStruct cb;
    
    grid_data = get_grid_data(widget);
    
    if (grid_data == NULL) {
        return;
    }
    
    btn_event = (XButtonEvent*)event;
    
    /* Calculate which row was clicked */
    y_pos = btn_event->y;
    
    /* Ignore clicks in the header */
    if (y_pos < grid_data->header_height) {
        return;
    }
    
    /* Calculate row index */
    row = (y_pos - grid_data->header_height) / grid_data->row_height;
    
    /* Make sure it's within range */
    if (row < 0 || row >= grid_data->num_rows) {
        return;
    }
    
    /* Update selection */
    grid_data->selected_row = row;
    
    /* Only redraw if window exists */
    if (XtWindow(widget)) {
        XClearWindow(XtDisplay(widget), XtWindow(widget));
        draw_grid(widget, NULL, NULL);
    }
    
    /* Handle different button types */
    if (btn_event->button == Button1) {  /* Left button */
        /* Check for double click */
        if (btn_event->time - last_click_time < 400 && row == last_click_row) {
            /* Double-click detected - set up a callback with special reason */
            memset(&cb, 0, sizeof(XmDrawingAreaCallbackStruct));
            cb.reason = XmCR_DEFAULT_ACTION;  /* Special reason for double-click */
            cb.event = event;
            
            XtCallCallbacks(widget, XmNinputCallback, (XtPointer)&cb);
        } else {
            /* Single click - just normal selection */
            memset(&cb, 0, sizeof(XmDrawingAreaCallbackStruct));
            cb.reason = XmCR_INPUT;
            cb.event = event;
            
            XtCallCallbacks(widget, XmNinputCallback, (XtPointer)&cb);
        }
        
        /* Update for next click */
        last_click_time = btn_event->time;
        last_click_row = row;
    }
    else if (btn_event->button == Button3) {  /* Right button */
        /* Pass to callback for right-click menu */
        memset(&cb, 0, sizeof(XmDrawingAreaCallbackStruct));
        cb.reason = XmCR_INPUT;
        cb.event = event;
        
        XtCallCallbacks(widget, XmNinputCallback, (XtPointer)&cb);
    }
}

/* Handle button release events */
static void handle_button_release(Widget widget, XtPointer client_data, XEvent *event, Boolean *continue_to_dispatch)
{
    /* Nothing special to do on release */
}

/* Draw the entire grid */
void draw_grid(Widget widget, XtPointer client_data, XtPointer call_data)
{
    GridWidget *grid_data;
    Display *display;
    Window window;
    Dimension width, height;
    int i, j;
    int x, y;
    int col_x;
    XFontStruct *font;
    int font_height, font_ascent;
    int text_width;
    int text_x;
    
    grid_data = get_grid_data(widget);
    
    if (grid_data == NULL) {
        return;
    }
    
    display = XtDisplay(widget);
    window = XtWindow(widget);
    
    /* CRITICAL: Make sure the window exists before attempting to draw */
    if (!window) {
        return;
    }
    
    /* Get widget dimensions */
    XtVaGetValues(widget, XmNwidth, &width, XmNheight, &height, NULL);
    
    /* Load a fixed font */
    font = XLoadQueryFont(display, "fixed");
    
    if (font == NULL) {
        fprintf(stderr, "Failed to load font\n");
        return;
    }
    
    font_height = font->ascent + font->descent;
    font_ascent = font->ascent;
    
    /* Draw header background */
    XFillRectangle(display, window, grid_data->gc_header,
                  0, 0, width, grid_data->header_height);
    
    /* Draw column headers */
    for (i = 0, col_x = 0; i < grid_data->num_columns; i++) {
        /* Draw header text */
        text_width = XTextWidth(font, grid_data->columns[i].title, 
                               strlen(grid_data->columns[i].title));
        
        /* Calculate text position based on alignment */
        if (grid_data->columns[i].alignment == 'C') {
            text_x = col_x + (grid_data->columns[i].width - text_width) / 2;
        } else if (grid_data->columns[i].alignment == 'R') {
            text_x = col_x + grid_data->columns[i].width - text_width - 5;
        } else {
            text_x = col_x + 5;  /* Left alignment or default */
        }
        
        /* Draw the header text */
        XDrawString(display, window, grid_data->gc_text,
                   text_x, grid_data->header_height / 2 + font_ascent / 2,
                   grid_data->columns[i].title,
                   strlen(grid_data->columns[i].title));
        
        /* Move to the next column */
        col_x += grid_data->columns[i].width;
    }
    
    /* Draw rows */
    for (i = 0; i < grid_data->num_rows; i++) {
        /* Calculate row position */
        y = grid_data->header_height + (i * grid_data->row_height);
        
        /* Draw selection background if this row is selected */
        if (i == grid_data->selected_row) {
            XFillRectangle(display, window, grid_data->gc_selection,
                          0, y, width, grid_data->row_height);
        }
        
        /* Draw each cell in the row */
        for (j = 0, col_x = 0; j < grid_data->num_columns; j++) {
            /* Calculate text position */
            text_width = XTextWidth(font, grid_data->cells[i][j].text, 
                                   strlen(grid_data->cells[i][j].text));
            
            /* Calculate text position based on alignment */
            if (grid_data->columns[j].alignment == 'C') {
                text_x = col_x + (grid_data->columns[j].width - text_width) / 2;
            } else if (grid_data->columns[j].alignment == 'R') {
                text_x = col_x + grid_data->columns[j].width - text_width - 5;
            } else {
                text_x = col_x + 5;  /* Left alignment or default */
            }
            
            /* Draw the cell text */
            XDrawString(display, window, grid_data->gc_text,
                       text_x, y + grid_data->row_height / 2 + font_ascent / 2,
                       grid_data->cells[i][j].text,
                       strlen(grid_data->cells[i][j].text));
            
            /* Move to the next column */
            col_x += grid_data->columns[j].width;
        }
    }
    
    /* Draw horizontal grid lines */
    for (i = 0; i <= grid_data->num_rows; i++) {
        y = grid_data->header_height + (i * grid_data->row_height);
        XDrawLine(display, window, grid_data->gc_lines,
                 0, y, width, y);
    }
    
    /* Draw vertical grid lines */
    for (i = 0, col_x = 0; i <= grid_data->num_columns; i++) {
        XDrawLine(display, window, grid_data->gc_lines,
                 col_x, 0, col_x, height);
        
        if (i < grid_data->num_columns) {
            col_x += grid_data->columns[i].width;
        }
    }
    
    /* Additional header separator line */
    XDrawLine(display, window, grid_data->gc_lines,
             0, grid_data->header_height, width, grid_data->header_height);
    
    /* Free font */
    XFreeFont(display, font);
}
