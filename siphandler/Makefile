
PROJECT	  := siphandler
VERSION   := 1.0.0
DESCR     := "siphandler is the villa lowlevel"

# Verbose and silent build modes
ifeq ($(V),)
HIDE=@
endif

CFLAGS    += -I. -Iinclude -I../third_party/re/include -I../third_party/rm/include -I../third_party/baresip/include

CXXFLAGS  += -I. -Iinclude -I../third_party/re/include -I../third_party/rm/include -I../third_party/baresip/include
CXXFLAGS  += $(EXTRA_CXXFLAGS)


# XXX: common for C/C++
CPPFLAGS += -DHAVE_INTTYPES_H

INSTALL := install
ifeq ($(DESTDIR),)
PREFIX  := /usr/local
else
PREFIX  := /usr
endif
BINDIR	:= $(PREFIX)/bin
INCDIR  := $(PREFIX)/include
BIN	:= $(PROJECT)$(BIN_SUFFIX)

all: $(BIN)

APP_SRCS := main.cpp

APP_OBJS  := $(OBJS) $(patsubst %.cpp,%.o,$(APP_SRCS))

LFLAGS	  = -L../third_party/re -L../third_party/rem -L../third_party/baresip -lre -lrem -lbaresip -fobjc-link-runtime \
	-framework AudioToolbox -framework AVFoundation -framework CoreVideo -framework CoreAudio -framework CoreMedia

-include $(APP_OBJS:.o=.d)

$(BIN):	$(APP_OBJS)
	@echo "  LD      $@"
	$(CXX) $(LFLAGS) $(APP_LFLAGS) $^ -o $@

$(BUILD)/%.o: %.c $(BUILD) Makefile $(APP_MK)
	@echo "  CC      $@"
	$(HIDE)$(CC) $(CFLAGS) -c $< -o $@ $(DFLAGS)

$(BUILD)/%.o: %.cpp $(BUILD) Makefile $(APP_MK)
	@echo "  CXX     $@"
	$(HIDE)$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@ $(DFLAGS)

clean:
	rm -f $(BIN) $(APP_OBJS)