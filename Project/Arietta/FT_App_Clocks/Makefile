#############################################################
# Project name
PROJECT = EVEtest
PLATFORM = ARIETTA

# Source files
ROOTDIR = ../../..
SRCDIR = $(ROOTDIR)/Src
CSRCS = \
  $(SRCDIR)/FT_App_Clocks.c \
  $(SRCDIR)/FT_Gpu_Hal.c \
  $(SRCDIR)/FT_CoPro_Cmds.c \

#  ./spidev-test.c \

# all directories to search for include files
INCLUDES = -I$(ROOTDIR)/Hdr \
           -I$(ROOTDIR)/Hdr/Arietta \

#############################################################
# generate OBJ files here
OUTPUT = obj

# compiler to use
CC = gcc

# Options common to compile, link and assembly rules
COMMON = 

# Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -D$(PLATFORM)
#CFLAGS += -Wall 

# Linker flags
LDFLAGS = $(COMMON)
LDFLAGS += -lm

# executable to generate
TARGET = $(PROJECT).exe

## Build
.PHONY: all
all: $(TARGET)

## Clean target
.PHONY: clean
clean:
	@echo "Clean project"
	@-rm -rf $(OUTPUT)

## Rebuild the project.
.PHONY: rebuild
rebuild: clean all

# Create objects files list with sources files
OBJECTS = $(CSRCS:.c=.o)

.PHONY: objfiles
objfiles: $(addprefix $(OUTPUT)/,$(notdir $(OBJECTS)))

# create object files from C source files.
.c.o:
	@echo 'Compiling $<'
	@$(shell mkdir $(OUTPUT) 2>/dev/null)
	@$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $(OUTPUT)/$(@F)

## Link
$(TARGET): $(OBJECTS)
	@echo "Linking $(addprefix $(OUTPUT)/,$(notdir $(OBJECTS)))..."
	@$(CC) $(LDFLAGS) $(addprefix $(OUTPUT)/,$(notdir $(OBJECTS))) $(LIBDIRS) $(LIBS) -o $(TARGET)

size: ${TARGET}
	@echo "============================================================================"
#	@echo "Sourcefiles: $(CSRCS)"
#	@echo "Objfiles: $(addprefix $(OUTPUT)/,$(notdir $(OBJECTS)))"
	@ls -hal ${TARGET}
