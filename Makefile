PRGNAME     = ngp.elf
CC          = clang
CXX 		= clang

# change compilation / linking flag options
GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"


INCLUDES	= -Imednafen/include -I./ -I./mednafen -I./mednafen/hw_cpu/z80-fuse -I./mednafen/ngp -I./mednafen/ngp/TLCS-900h -I./mednafen/sound -Ilibretro-common/include
DEFINES		= -DLSB_FIRST -DINLINE="inline" -DWANT_16BPP -DFRONTEND_SUPPORTS_RGB565 -DINLINE="inline" -DDEBUG
DEFINES		+= -DSIZEOF_DOUBLE=8 $(WARNINGS) -DMEDNAFEN_VERSION=\"0.9.31\" -DPACKAGE=\"mednafen\" -DMEDNAFEN_VERSION_NUMERIC=931 -DPSS_STYLE=1 -DMPC_FIXED_POINT -DSTDC_HEADERS -D__STDC_LIMIT_MACROS -D_LOW_ACCURACY_
DEFINES		+= -DGIT_VERSION=\"$(GIT_VERSION)\"
   
CFLAGS		= -Ofast -march=native -g3 $(INCLUDES) $(DEFINES)
CXXFLAGS	= $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11
LDFLAGS     = -lSDL -lm -lasound -lportaudio -lstdc++ -lz

# Files to be compiled
SRCDIR 		= ./ ./mednafen ./mednafen/hw_cpu/z80-fuse ./mednafen/ngp ./mednafen/ngp/TLCS-900h ./mednafen/sound ./libretro-common/streams ./libretro-common/vfs
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
