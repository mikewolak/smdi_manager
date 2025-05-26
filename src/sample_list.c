/* src/sample_list.c - Creates the sample list view using the grid widget */
#include "app_all.h"

void create_sample_list(Widget parent)
{
    Widget sample_frame;
    
    /* Create a frame for the sample list */
    sample_frame = XtVaCreateManagedWidget(
        "sample_frame",            /* Widget name */
        xmFrameWidgetClass,        /* Widget class */
        parent,                    /* Parent widget */
        XmNshadowType, XmSHADOW_ETCHED_IN,
        XmNpaneMinimum, 200,       /* Minimum height */
        NULL);                     /* Terminate list */
    
    /* Remove the "Sample List" title label */
    
    /* Create the grid widget for samples */
    app_data.sampleGrid = create_grid_widget(
        sample_frame,               /* Parent widget */
        10, 10,                     /* Position (x, y) - adjusted y position */
        620, 400);                  /* Size (width, height) */
    
    /* Define grid columns with adjusted widths */
    grid_set_column(app_data.sampleGrid, 0, "ID", 50, 'C');
    grid_set_column(app_data.sampleGrid, 1, "Name", 180, 'L');
    grid_set_column(app_data.sampleGrid, 2, "Rate (Hz)", 115, 'R');
    grid_set_column(app_data.sampleGrid, 3, "Length", 110, 'R');
    grid_set_column(app_data.sampleGrid, 4, "Bits", 60, 'C');
    grid_set_column(app_data.sampleGrid, 5, "Channels", 105, 'C');
    
    /* Add callback for selection */
    grid_add_select_callback(app_data.sampleGrid, sample_selected_callback, NULL);
    
    /* Add event handler for right-click popup menu */
    XtAddEventHandler(app_data.sampleGrid, ButtonPressMask, False, 
                     (XtEventHandler)sample_create_popup_menu, NULL);
}

/* Clear the sample list */
void clear_sample_list(void)
{
    /* Reset sample count */
    app_data.numSamples = 0;
    
    /* Set grid rows to 0 */
    grid_set_num_rows(app_data.sampleGrid, 0);
}

/* Add a sample to the list */
void add_sample_to_list(SampleInfo *sample)
{
    char buffer[256];
    int row;
    Dimension height;
    int total_rows;
    GridWidget *grid_data;
    
    /* Don't add if we're at max capacity */
    if (app_data.numSamples >= MAX_SAMPLES) {
        return;
    }
    
    /* Store the sample info in our array */
    app_data.samples[app_data.numSamples] = *sample;
    
    /* Add a row to the grid */
    row = app_data.numSamples;
    grid_set_num_rows(app_data.sampleGrid, row + 1);
    
    /* Set cell values */
    sprintf(buffer, "%d", sample->id);
    grid_set_cell(app_data.sampleGrid, row, 0, buffer);
    
    grid_set_cell(app_data.sampleGrid, row, 1, sample->name);
    
    sprintf(buffer, "%d", sample->rate);
    grid_set_cell(app_data.sampleGrid, row, 2, buffer);
    
    sprintf(buffer, "%d", sample->length);
    grid_set_cell(app_data.sampleGrid, row, 3, buffer);
    
    sprintf(buffer, "%d", sample->bits);
    grid_set_cell(app_data.sampleGrid, row, 4, buffer);
    
    sprintf(buffer, "%d", sample->channels);
    grid_set_cell(app_data.sampleGrid, row, 5, buffer);
    
    /* Increment counter */
    app_data.numSamples++;
    
    /* Ensure the grid's drawing area is large enough */
    grid_data = get_grid_data(app_data.sampleGrid);
    if (grid_data) {
        total_rows = app_data.numSamples;
        height = grid_data->header_height + (grid_data->row_height * total_rows);
        
        /* Force the drawing area to have enough height */
        XtVaSetValues(app_data.sampleGrid, 
                     XmNheight, height,
                     NULL);
        
        /* Update the scrolled window */
        if (grid_data->scrolled) {
            XmScrolledWindowSetAreas(grid_data->scrolled, NULL, NULL, app_data.sampleGrid);
        }
        
    }
    
    /* Force a redraw if the window exists */
    if (XtWindow(app_data.sampleGrid)) {
        XClearWindow(XtDisplay(app_data.sampleGrid), XtWindow(app_data.sampleGrid));
        draw_grid(app_data.sampleGrid, NULL, NULL);
    }
}
