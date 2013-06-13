CC            = g++ 
INCLUDEDIR    = -I/usr/include -I/usr/include/X11 -I/usr/X11R6/include

#Linux C Flags
#CFLAGS	      =  -g -O0 -fomit-frame-pointer -ffast-math -finline-functions -march=i486 -DNDEBUG -DNONANSI_INCLUDES -DSH_MEM -DSIG_ONE_PARAM $(INCLUDEDIR)

CFLAGS	      =  --static -g $(INCLUDEDIR)
#CFLAGS	      =  --static -g -arch=i386 -arch=ppc

LIBS	      =  -L/usr/X11/lib -L/usr/lib/X11 -L/usr/X11R6/lib -lX11

DEST	      = ~/bin
EXTHDRS	      =
HDRS	      = missile.h
INSTALL	      = /etc/install
LD            = $(CC) -arch=i386 -arch=ppc
LDFLAGS       =
MAKEFILE      = Makefile
OBJS          = missile.o
PRINT	      = pr
PROGRAM       = missile
SHELL	      = /bin/sh
SRCS	      = missile.cc

SYSHDRS	      = 

all:		$(PROGRAM)
$(PROGRAM):	$(OBJS)
		$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $(PROGRAM)
		strip $(PROGRAM)

clean:;		rm -f $(OBJS) core
		@make depend

clobber:;	rm -f $(OBJS) $(PROGRAM) core tags

depend:;	makedepend -- $(CFLAGS) -- $(SRCS)

echo:;		@echo $(HDRS) $(SRCS)

index:;		@ctags -wx $(HDRS) $(SRCS)

install:	$(PROGRAM)
		@echo Installing $(PROGRAM) in $(DEST)
		@-strip $(PROGRAM)
		@if [ $(DEST) != . ]; then \
		(rm -f $(DEST)/$(PROGRAM); $(INSTALL) -f $(DEST) $(PROGRAM)); fi

print:;		@$(PRINT) $(HDRS) $(SRCS)

tags:           $(HDRS) $(SRCS); @ctags $(HDRS) $(SRCS)

update:		$(DEST)/$(PROGRAM)

# DO NOT DELETE THIS LINE -- make depend depends on it.

missile.o: /usr/X11R6/include/X11/X.h /usr/X11R6/include/X11/Xlib.h
missile.o: /usr/include/sys/types.h /usr/include/sys/cdefs.h
missile.o: /usr/include/machine/endian.h /usr/include/sys/_types.h
#missile.o: /usr/include/machine/_types.h /usr/include/sys/_pthreadtypes.h
#missile.o: /usr/include/sys/select.h /usr/include/sys/_sigset.h
#missile.o: /usr/include/sys/_timeval.h /usr/include/sys/timespec.h
missile.o: /usr/X11R6/include/X11/Xfuncproto.h
missile.o: /usr/X11R6/include/X11/Xosdefs.h /usr/include/stddef.h
#missile.o: /usr/include/sys/_null.h /usr/X11R6/include/X11/Xutil.h
missile.o: /usr/include/stdio.h /usr/include/stdlib.h /usr/include/string.h
missile.o: /usr/include/strings.h /usr/include/unistd.h
missile.o: /usr/include/sys/unistd.h /usr/include/time.h
