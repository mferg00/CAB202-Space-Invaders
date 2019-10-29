# Topic 11 Makefile (Preview)
# Lawrence Buckingham, October  2017.
# Queensland University of Technology.


# Add modules here!
OBJ=main.o sprites.o serial_io.o hardware.o
# OBJ= Assignment2.o

# Set the name of the folder containing libcab202_teensy.a
CAB202_TEENSY_FOLDER=../cab202_teensy

# Set the name of the folder containing usb_serial.o
USB_SERIAL_FOLDER =../usb_serial
USB_SERIAL_OBJ =../usb_serial/usb_serial.o

# ---------------------------------------------------------------------------
#	Leave the rest of the file alone.
# ---------------------------------------------------------------------------

all: game.hex

TEENSY_LIBS = $(USB_SERIAL_OBJ)  -lcab202_teensy -lprintf_flt -lm

TEENSY_DIRS =-I$(CAB202_TEENSY_FOLDER) -L$(CAB202_TEENSY_FOLDER) \
	-I$(USB_SERIAL_FOLDER)

TEENSY_FLAGS = \
	-std=gnu99 \
	-mmcu=atmega32u4 \
	-DF_CPU=8000000UL \
	-funsigned-char \
	-funsigned-bitfields \
	-ffunction-sections \
	-fpack-struct \
	-fshort-enums \
	-Wall \
	-Werror \
	-Wl,-u,vfprintf \
	-Os

clean:
	for f in $(TARGETS); do \
		if [ -f $$f ]; then rm $$f; fi; \
		if [ -f $$f.elf ]; then rm $$f.elf; fi; \
		if [ -f $$f.obj ]; then rm $$f.obj; fi; \
	done

rebuild: clean all

%.o : %.c
	avr-gcc $< -c $(TEENSY_FLAGS) $(TEENSY_DIRS)

game.hex : $(OBJ)
	avr-gcc $(OBJ) $(TEENSY_FLAGS) $(TEENSY_DIRS) $(TEENSY_LIBS) -o $@.obj
	avr-objcopy -O ihex $@.obj $@
