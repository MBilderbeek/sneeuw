CC=htc
CFLAGS=-O

TARGET=snow
LIBS=-lg
EXTRAS=gs2.o
DOSFILEPATH=~/Aurora/
OTHERFILES=snowbg2.sr5 tvs5.sr5
EMULATOR=openmsx -ext debugdevice 
EMUDISK=-diska

OBJECTS=snow.o setpal.o debug.o

#----------------------------------------------------------------------------
# Pattern rule to create object files from .as files

%.o: %.as
	$(CC) -c $(CFLAGS) -o $@ $<

# ===========================================================================
# The real stuff

all: emutest

emutest: $(TARGET) dsk
	$(EMULATOR) $(EMUDISK) $(TARGET).dsk

dsk: $(TARGET)
	rm -f $(TARGET).dsk autoexec.bat
	echo $(TARGET) > autoexec.bat
	wrdsk $(TARGET).dsk $(DOSFILEPATH)/msxdos.sys  \
	                    $(DOSFILEPATH)/command.com \
			    autoexec.bat               \
			    $(TARGET).com              \
			    $(OTHERFILES)


$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET).com $(OBJECTS) $(EXTRAS) $(LIBS) 

clean:
	rm -f palette.h $(TARGET).as $(TARGET).com $(OBJECTS) $(TARGET).dsk autoexec.bat

setpal.o: setpal.as
debug.o: debug.as

$(TARGET).o: $(TARGET).c palette.h

palette.h: tvs5.pl5
	gcc -o hulp/plX2h hulp/plX2h.c
	hulp/plX2h $< $@
