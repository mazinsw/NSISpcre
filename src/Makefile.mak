CPP    = g++
RM     = rm -f
OBJS   = NSISpcre.o \
         pluginapi.o

LIBS   = -shared -Wl,--kill-at -L"..\..\..\Projects\FalconCpp\trunk\pkg\pcre\pcre 8.39\pcre-8.39\dist\lib" -lpcrecpp -lpcre -m32
CFLAGS = -DUNICODE -D_UNICODE -I"..\..\..\Projects\FalconCpp\trunk\pkg\pcre\pcre 8.39\pcre-8.39\dist\include" -DPCRE_STATIC -m32 -fno-diagnostics-show-option

.PHONY: ../bin/x86-unicode/NSISpcre.dll clean clean-after

all: ../bin/x86-unicode/NSISpcre.dll

clean:
	$(RM) $(OBJS) ../bin/x86-unicode/NSISpcre.dll

clean-after:
	$(RM) $(OBJS)

../bin/x86-unicode/NSISpcre.dll: $(OBJS)
	$(CPP) -Wall -s -O2 -o $@ $(OBJS) $(LIBS)

NSISpcre.o: NSISpcre.cpp exdll.h
	$(CPP) -Wall -s -O2 -c $< -o $@ $(CFLAGS)

pluginapi.o: pluginapi.c pluginapi.h
	$(CPP) -Wall -s -O2 -c $< -o $@ $(CFLAGS)

