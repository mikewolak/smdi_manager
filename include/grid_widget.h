/* grid_widget.h - Custom Grid Widget for IRIX 5.3 Motif 1.2 */
#ifndef GRID_WIDGET_H
#define GRID_WIDGET_H

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/DrawingA.h>
#include <Xm/ScrolledW.h>

/* Maximum dimensions */
#define GRID_MAX_ROWS 1000
#define GRID_MAX_COLS 10
#define GRID_TEXT_LEN 256

/* Default column widths */
#define NAME_COLUMN_WIDTH 180
#define RATE_COLUMN_WIDTH 115 
#define LENGTH_COLUMN_WIDTH 110
#define BITS_COLUMN_WIDTH 60
#define CHANNELS_COLUMN_WIDTH 105

/* Column definition struct */
typedef struct {
    char title[GRID_TEXT_LEN];  /* Column title */
    int width;                  /* Column width in pixels */
    unsigned char alignment;    /* 'L'=left, 'C'=center, 'R'=right */
} GridColumn;

/* Cell data struct */
typedef struct {
    char text[GRID_TEXT_LEN];   /* Cell text */
} GridCell;

/* Grid Widget struct */
typedef struct {
    Widget parent;              /* Parent widget */
    Widget scrolled;            /* Scrolled window container */
    Widget drawing;             /* Drawing area for grid */
    
    /* Data storage */
    GridColumn columns[GRID_MAX_COLS];  /* Column definitions */
    GridCell cells[GRID_MAX_ROWS][GRID_MAX_COLS]; /* Cell data */
    
    /* Grid properties */
    int num_columns;            /* Number of columns */
    int num_rows;               /* Number of rows (data) */
    int header_height;          /* Height of header in pixels */
    int row_height;             /* Height of rows in pixels */
    int selected_row;           /* Currently selected row (-1 = none) */
    
    /* Graphic contexts */
    GC gc_text;                 /* For drawing text */
    GC gc_lines;                /* For drawing grid lines */
    GC gc_header;               /* For drawing header background */
    GC gc_selection;            /* For drawing selection background */
    
    /* Callbacks */
    XtCallbackList select_callback;       /* Called when row is selected */
    XtCallbackList double_click_callback; /* Called when row is double-clicked */
    XtCallbackList right_click_callback;  /* Called when row is right-clicked */
} GridWidget;

/* Function prototypes */
Widget create_grid_widget(Widget parent, int x, int y, int width, int height);
void grid_set_column(Widget grid, int col_index, const char *title, int width, char alignment);
void grid_set_num_rows(Widget grid, int num_rows);
void grid_set_cell(Widget grid, int row, int col, const char *text);
void grid_add_select_callback(Widget grid, XtCallbackProc callback, XtPointer client_data);
void grid_add_double_click_callback(Widget grid, XtCallbackProc callback, XtPointer client_data);
void grid_add_right_click_callback(Widget grid, XtCallbackProc callback, XtPointer client_data);
int grid_get_selected_row(Widget grid);
void grid_select_row(Widget grid, int row);

/* Get grid data from widget */
GridWidget *get_grid_data(Widget grid);

/* Draw grid function made public to allow forced redraws */
void draw_grid(Widget widget, XtPointer client_data, XtPointer call_data);

#endif /* GRID_WIDGET_H */
