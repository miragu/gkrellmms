MSGFMT = msgfmt
SUB_DIRS = 
FILES_PO:=$(wildcard *.po)
FILES_MO:=$(FILES_PO:.po=.mo)

LOCALEDIR ?= /usr/share/locale
PACKAGE ?= gkrellm-gkrellmms

ifeq ($(enable_nls),1)
all: mo-files

mo-files: $(FILES_MO)

install: 
	$(MAKE) all
	for f in *.mo ; do mkdir -p \
		$(INSTALL_PREFIX)$(LOCALEDIR)/`basename $$f .mo`/LC_MESSAGES ; \
		cp $$f $(INSTALL_PREFIX)$(LOCALEDIR)/`basename $$f .mo`/LC_MESSAGES/$(PACKAGE).mo ; done

%.mo: %.po
	$(MSGFMT) -f -v -o $@ $<

else

all:

install:

endif

clean:
	$(RM) $(FILES_MO)

