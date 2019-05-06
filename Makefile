
PIDGIN_TREE_TOP ?= ../pidgin-2.10.11
PIDGIN3_TREE_TOP ?= ../pidgin-main
LIBPURPLE_DIR ?= $(PIDGIN_TREE_TOP)/libpurple
WIN32_DEV_TOP ?= $(PIDGIN_TREE_TOP)/../win32-dev

WIN32_CC ?= $(WIN32_DEV_TOP)/mingw-4.7.2/bin/gcc

PROTOC_C ?= protoc-c
PKG_CONFIG ?= pkg-config

DIR_PERM = 0755
LIB_PERM = 0755
FILE_PERM = 0644

CFLAGS	?= -O2 -g -pipe -Wall
LDFLAGS ?= -Wl,-z,relro

CFLAGS  += -std=c99 

# Comment out to disable localisation
CFLAGS += -DENABLE_NLS

# Do some nasty OS and purple version detection
ifeq ($(OS),Windows_NT)
  #only defined on 64-bit windows
  PROGFILES32 = ${ProgramFiles(x86)}
  ifndef PROGFILES32
    PROGFILES32 = $(PROGRAMFILES)
  endif
  TARGET = libprpl.dll
  DEST = "$(PROGFILES32)/Pidgin/plugins"
  ICONS_DEST = "$(PROGFILES32)/Pidgin/pixmaps/pidgin/protocols"
  LOCALEDIR = "$(PROGFILES32)/Pidgin/locale"
else
  UNAME_S := $(shell uname -s)

  #.. There are special flags we need for OSX
  ifeq ($(UNAME_S), Darwin)
    #
    #.. /opt/local/include and subdirs are included here to ensure this compiles
    #   for folks using Macports.  I believe Homebrew uses /usr/local/include
    #   so things should "just work".  You *must* make sure your packages are
    #   all up to date or you will most likely get compilation errors.
    #
    INCLUDES = -I/opt/local/include -lz $(OS)

    CC = gcc
  else
    CC ?= gcc
  endif

  ifeq ($(shell $(PKG_CONFIG) --exists purple-3 2>/dev/null && echo "true"),)
    ifeq ($(shell $(PKG_CONFIG) --exists purple 2>/dev/null && echo "true"),)
      TARGET = FAILNOPURPLE
      DEST =
      ICONS_DEST =
    else
      TARGET = libprpl.so
      DEST = $(DESTDIR)`$(PKG_CONFIG) --variable=plugindir purple`
      ICONS_DEST = $(DESTDIR)`$(PKG_CONFIG) --variable=datadir purple`/pixmaps/pidgin/protocols
      LOCALEDIR = $(DESTDIR)$(shell $(PKG_CONFIG) --variable=datadir purple)/locale
    endif
  else
    TARGET = libprpl.so
    DEST = $(DESTDIR)`$(PKG_CONFIG) --variable=plugindir purple-3`
    ICONS_DEST = $(DESTDIR)`$(PKG_CONFIG) --variable=datadir purple-3`/pixmaps/pidgin/protocols
    LOCALEDIR = $(DESTDIR)$(shell $(PKG_CONFIG) --variable=datadir purple-3)/locale
  endif
endif

WIN32_CFLAGS = -std=c99 -I$(WIN32_DEV_TOP)/glib-2.28.8/include -I$(WIN32_DEV_TOP)/glib-2.28.8/include/glib-2.0 -I$(WIN32_DEV_TOP)/glib-2.28.8/lib/glib-2.0/include -DPLUGIN_VERSION='"$(PLUGIN_VERSION)"' -Wall -Wextra -Werror -Wno-deprecated-declarations -Wno-unused-parameter -fno-strict-aliasing -Wformat
WIN32_LDFLAGS = -L$(WIN32_DEV_TOP)/glib-2.28.8/lib  -lpurple -lintl -lglib-2.0  -ggdb -static-libgcc -lz
WIN32_PIDGIN2_CFLAGS = -I$(PIDGIN_TREE_TOP)/libpurple -I$(PIDGIN_TREE_TOP) $(WIN32_CFLAGS)
WIN32_PIDGIN3_CFLAGS = -I$(PIDGIN3_TREE_TOP)/libpurple -I$(PIDGIN3_TREE_TOP) -I$(WIN32_DEV_TOP)/gplugin-dev/gplugin $(WIN32_CFLAGS)
WIN32_PIDGIN2_LDFLAGS = -L$(PIDGIN_TREE_TOP)/libpurple $(WIN32_LDFLAGS)
WIN32_PIDGIN3_LDFLAGS = -L$(PIDGIN3_TREE_TOP)/libpurple -L$(WIN32_DEV_TOP)/gplugin-dev/gplugin $(WIN32_LDFLAGS) -lgplugin

CFLAGS += -DLOCALEDIR=\"$(LOCALEDIR)\"

C_FILES := 
PURPLE_COMPAT_FILES :=
PURPLE_C_FILES := libprpl.c $(C_FILES)

.PHONY:	all install FAILNOPURPLE clean install-icons install-locales %-locale-install

LOCALES = $(patsubst %.po, %.mo, $(wildcard po/*.po))

all: $(TARGET)

libprpl.so: $(PURPLE_C_FILES) $(PURPLE_COMPAT_FILES)
	$(CC) -fPIC $(CFLAGS) $(CPPFLAGS) -shared -o $@ $^ $(LDFLAGS) `$(PKG_CONFIG) purple glib-2.0 --libs --cflags`  $(INCLUDES) -Ipurple2compat -g -ggdb

libprpl3.so: $(PURPLE_C_FILES)
	$(CC) -fPIC $(CFLAGS) $(CPPFLAGS) -shared -o $@ $^ $(LDFLAGS) `$(PKG_CONFIG) purple-3 glib-2.0 --libs --cflags` $(INCLUDES)  -g -ggdb

libprpl.dll: $(PURPLE_C_FILES) $(PURPLE_COMPAT_FILES)
	$(WIN32_CC) -O0 -g -ggdb -shared -o $@ $^ $(WIN32_PIDGIN2_CFLAGS) $(WIN32_PIDGIN2_LDFLAGS) -Ipurple2compat

libprpl3.dll: $(PURPLE_C_FILES) $(PURPLE_COMPAT_FILES)
	$(WIN32_CC) -O0 -g -ggdb -shared -o $@ $^ $(WIN32_PIDGIN3_CFLAGS) $(WIN32_PIDGIN3_LDFLAGS)

install: $(TARGET)
	mkdir -m $(DIR_PERM) -p $(DEST)
	install -m $(LIB_PERM) -p $(TARGET) $(DEST)

FAILNOPURPLE:
	echo "You need libpurple development headers installed to be able to compile this plugin"

clean:
	rm -f $(TARGET)

gdb:
	gdb --args pidgin -c ~/.fake_purple -n -m

