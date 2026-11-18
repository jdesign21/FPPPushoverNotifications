SRCDIR ?= /opt/fpp/src
include $(SRCDIR)/makefiles/common/setup.mk
include $(SRCDIR)/makefiles/platform/*.mk

all: libfpp-FPPPushoverNotifications.$(SHLIB_EXT)

OBJECTS_fpp_FPPPushoverNotifications_so += FPPPushoverNotifications.o
LIBS_fpp_FPPPushoverNotifications_so += -L$(SRCDIR) -lfpp -ljsoncpp
CXXFLAGS_FPPPushoverNotifications.o += -I$(SRCDIR)

libfpp-FPPPushoverNotifications.$(SHLIB_EXT): $(OBJECTS_fpp_FPPPushoverNotifications_so) $(SRCDIR)/libfpp.$(SHLIB_EXT)
	$(CCACHE) $(CC) -shared $(CFLAGS_$@) $(OBJECTS_fpp_FPPPushoverNotifications_so) $(LIBS_fpp_FPPPushoverNotifications_so) $(LDFLAGS) -o $@

%.o: %.cpp Makefile
	$(CCACHE) $(CC) $(CFLAGS) $(CXXFLAGS) $(CXXFLAGS_$@) -c $< -o $@

clean:
	rm -f libfpp-FPPPushoverNotifications.$(SHLIB_EXT) $(OBJECTS_fpp_FPPPushoverNotifications_so)

.PHONY: all clean
