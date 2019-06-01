PRGNAME     = ngp.elf
CC          = gcc
CXX 		= g++

#### Configuration

# Possible values : retrostone, rs97, rs90
PORT = retrostone
# Possible values : alsa, oss, portaudio
SOUND_ENGINE = alsa
# Possible values : generic, rs90
MENU = generic

#### End of Configuration

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"

INCLUDES	= -Imednafen/include -I./ -I./mednafen -I./mednafen/hw_cpu/z80-fuse -I./mednafen/ngp -I./mednafen/ngp/TLCS-900h -I./mednafen/sound
INCLUDES	+= -Ishell/headers -Ishell/video/$(PORT) -Ishell/audio -Ishell/scalers -Ishell/input/sdl -Ishell/fonts -Ishell/menu/$(MENU)

DEFINES		= -DLSB_FIRST -DINLINE="inline" -DWANT_16BPP -DFRONTEND_SUPPORTS_RGB565 -DINLINE="inline" -DNDEBUG -DWANT_STEREO_SOUND
DEFINES		+= -DSIZEOF_DOUBLE=8 $(WARNINGS) -DMEDNAFEN_VERSION=\"0.9.31\" -DPACKAGE=\"mednafen\" -DMEDNAFEN_VERSION_NUMERIC=931 -DPSS_STYLE=1 -DMPC_FIXED_POINT -DSTDC_HEADERS -D__STDC_LIMIT_MACROS -D_LOW_ACCURACY_
DEFINES		+= -DGIT_VERSION=\"$(GIT_VERSION)\"

CFLAGS		= -Ofast -march=native -g -fno-common -Wall $(INCLUDES) $(DEFINES)
CXXFLAGS	= $(CFLAGS) -nostdinc++ -fno-rtti -fno-exceptions -std=gnu++11
LDFLAGS     = -lc -lgcc -lm -lSDL -lz

ifeq ($(SOUND_ENGINE), alsa)
LDFLAGS 		+= -lasound
endif
ifeq ($(SOUND_ENGINE), portaudio)
LDFLAGS 		+= -lasound -lportaudio
endif

# Files to be compiled
SRCDIR 		=  ./mednafen ./mednafen/hw_cpu/z80-fuse ./mednafen/ngp ./mednafen/ngp/TLCS-900h ./mednafen/sound
SRCDIR		+= ./shell/input/sdl ./shell/audio/$(SOUND_ENGINE) ./shell/emu ./shell/scalers ./shell/video/$(PORT) ./shell/menu/$(MENU) ./shell/fonts

VPATH		= $(SRCDIR)
SRC_C		= $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.c))
SRC_CPP		= $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.cpp))
OBJ_C		= $(notdir $(patsubst %.c, %.o, $(SRC_C)))
OBJ_CPP		= $(notdir $(patsubst %.cpp, %.o, $(SRC_CPP)))
OBJS		= $(OBJ_C) $(OBJ_CPP)

# Rules to make executable
$(PRGNAME): $(OBJS)  
	$(CC) $(CFLAGS) -std=gnu99 -o $(PRGNAME) $^ $(LDFLAGS)
	
$(OBJ_CPP) : %.o : %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(PRGNAME)$(EXESUFFIX) *.o
