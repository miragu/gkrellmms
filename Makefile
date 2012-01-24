# Sample Makefile for a GKrellM plugin, edited for GKrellMMS

# You may want to rename the binary-file.
BIN_FILENAME = gkrellmms

GTK_INCLUDE ?= `pkg-config gtk+-2.0 --cflags`
GTK_LIB ?= `pkg-config gtk+-2.0 --libs`
IMLIB_INCLUDE ?= 
IMLIB_LIB ?= 

XMMS_INCLUDE ?= `pkg-config audacious --cflags`
XMMS_LIB ?= `pkg-config audacious --libs` -laudclient

DBUS_INCLUDE ?= `pkg-config dbus-1 --cflags`
DBUS_LIB ?= `pkg-config dbus-1 --libs`

PLUGIN_DIR ?= /usr/local/lib/gkrellm2/plugins

FLAGS = -O2 -Wall -fPIC $(GTK_INCLUDE) $(IMLIB_INCLUDE) $(DBUS_INCLUDE) $(XMMS_INCLUDE)
LIBS = $(GTK_LIB) $(IMLIB_LIB) $(DBUS_LIB) $(XMMS_LIB)
LFLAGS = -shared -lpthread

LOCALEDIR ?= /usr/share/locale
ifeq ($(enable_nls),1)
   FLAGS += -DENABLE_NLS -DLOCALEDIR=\"$(LOCALEDIR)\"
   export enable_nls
endif
PACKAGE ?= gkrellmms
FLAGS += -DPACKAGE="\"$(PACKAGE)\"" 
export PACKAGE LOCALEDIR

CC ?= gcc 
CC += $(CFLAGS) $(FLAGS)

INSTALL = install -c
INSTALL_PROGRAM = $(INSTALL) -s

OBJS = gkrellmms.o options.o playlist.o

all:	$(BIN_FILENAME).so
	(cd po && ${MAKE} all )

$(BIN_FILENAME).so: $(OBJS)
	$(CC) $(OBJS) -o $(BIN_FILENAME).so $(LFLAGS) $(LIBS)

clean:
	rm -f *.o core *.so* *.bak *~
	(cd po && ${MAKE} clean)

install:
	(cd po && ${MAKE} INSTALL_PREFIX=$(DESTDIR) install)
	$(INSTALL_PROGRAM) -m 755 $(BIN_FILENAME).so \
		$(DESTDIR)/$(PLUGIN_DIR)/$(BIN_FILENAME).so

gkrellmms.c.o: gkrellmms.c
options.c.o: options.c
playlist.c.o: playlist.c
