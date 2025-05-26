# Makefile for IRIX 5.3 SMDI Sample Manager with Motif 1.2
# Uses ANSI C90 standard and targets MIPS architecture

# Compiler and flags
CC = cc
CFLAGS = -ansi -O -I./include -I/usr/include/X11 -I/usr/include/Motif1.2 -fullwarn -32 -mips2 -D_SGIMOTIF
LDFLAGS = -L/usr/lib -lXm -lXt -lX11 -lds -laudiofile -laudioutil -lm

# Directories
SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin

# Target executable
TARGET = $(BINDIR)/smdi_manager

# GUI Object files
GUI_OBJS = $(OBJDIR)/main.o $(OBJDIR)/window.o $(OBJDIR)/menu.o $(OBJDIR)/callbacks.o \
       $(OBJDIR)/sample_list.o $(OBJDIR)/status_bar.o $(OBJDIR)/smdi_operations.o \
       $(OBJDIR)/grid_widget.o

# SMDI Object files
SMDI_OBJS = $(OBJDIR)/smdi_util.o $(OBJDIR)/smdi_core.o $(OBJDIR)/smdi_sample.o \
            $(OBJDIR)/smdi_aif.o $(OBJDIR)/aspi_irix.o $(OBJDIR)/scsi_debug.o

# Default target
all: directories $(TARGET)

# Create directories if they don't exist
directories:
	test -d $(OBJDIR) || mkdir -p $(OBJDIR)
	test -d $(BINDIR) || mkdir -p $(BINDIR)

# Link
$(TARGET): $(GUI_OBJS) $(SMDI_OBJS)
	$(CC) -o $@ $(GUI_OBJS) $(SMDI_OBJS) $(LDFLAGS)

# Compile rules for GUI
$(OBJDIR)/main.o: $(SRCDIR)/main.c $(INCDIR)/app_all.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/main.c -o $(OBJDIR)/main.o

$(OBJDIR)/window.o: $(SRCDIR)/window.c $(INCDIR)/app_all.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/window.c -o $(OBJDIR)/window.o

$(OBJDIR)/menu.o: $(SRCDIR)/menu.c $(INCDIR)/app_all.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/menu.c -o $(OBJDIR)/menu.o

$(OBJDIR)/callbacks.o: $(SRCDIR)/callbacks.c $(INCDIR)/app_all.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/callbacks.c -o $(OBJDIR)/callbacks.o

$(OBJDIR)/sample_list.o: $(SRCDIR)/sample_list.c $(INCDIR)/app_all.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/sample_list.c -o $(OBJDIR)/sample_list.o

$(OBJDIR)/status_bar.o: $(SRCDIR)/status_bar.c $(INCDIR)/app_all.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/status_bar.c -o $(OBJDIR)/status_bar.o

$(OBJDIR)/smdi_operations.o: $(SRCDIR)/smdi_operations.c $(INCDIR)/app_all.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/smdi_operations.c -o $(OBJDIR)/smdi_operations.o

$(OBJDIR)/grid_widget.o: $(SRCDIR)/grid_widget.c $(INCDIR)/grid_widget.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/grid_widget.c -o $(OBJDIR)/grid_widget.o

# Compile rules for SMDI
$(OBJDIR)/smdi_util.o: $(SRCDIR)/smdi_util.c $(INCDIR)/smdi.h $(INCDIR)/aspi_irix.h $(INCDIR)/scsi_debug.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/smdi_util.c -o $(OBJDIR)/smdi_util.o

$(OBJDIR)/smdi_core.o: $(SRCDIR)/smdi_core.c $(INCDIR)/smdi.h $(INCDIR)/aspi_irix.h $(INCDIR)/scsi_debug.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/smdi_core.c -o $(OBJDIR)/smdi_core.o

$(OBJDIR)/smdi_sample.o: $(SRCDIR)/smdi_sample.c $(INCDIR)/smdi.h $(INCDIR)/smdi_sample.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/smdi_sample.c -o $(OBJDIR)/smdi_sample.o

$(OBJDIR)/smdi_aif.o: $(SRCDIR)/smdi_aif.c $(INCDIR)/smdi.h $(INCDIR)/smdi_sample.h $(INCDIR)/smdi_aif.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/smdi_aif.c -o $(OBJDIR)/smdi_aif.o

$(OBJDIR)/aspi_irix.o: $(SRCDIR)/aspi_irix.c $(INCDIR)/aspi_irix.h $(INCDIR)/scsi_debug.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/aspi_irix.c -o $(OBJDIR)/aspi_irix.o

$(OBJDIR)/scsi_debug.o: $(SRCDIR)/scsi_debug.c $(INCDIR)/scsi_debug.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/scsi_debug.c -o $(OBJDIR)/scsi_debug.o

# Clean
clean:
	rm -f $(OBJDIR)/*.o $(TARGET)

# Run with output redirection (helpful for debugging)
run: all
	$(TARGET) > $(BINDIR)/out 2>&1

# Install to /usr/local/bin
install: all
	cp $(TARGET) /usr/local/bin/

.PHONY: all clean directories run install
