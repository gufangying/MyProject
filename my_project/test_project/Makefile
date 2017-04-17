BIN	 = video
CC	 = gcc
CPP	 = g++
INCS	 = -I./
CSRCS	 = $(wildcard ./*.c ./ueventmonitor/*.c)

define all-cpp-files-under
$(shell find $(1) -name "*."$(2) -and -not -name ".*" )
endef

define all-subdir-cpp-files  
$(call all-cpp-files-under,.,"cpp")
endef

CPPSRCS	 = $(call all-subdir-cpp-files)
COBJS	:= $(CSRCS:.c=.o)
CPPOBJS	:= $(CPPSRCS:.cpp=.o)

DEBUGFLAGS = -funwind-tables -g -rdynamic -O0 -DDEBUG
OPTIMIZEFLAGS = -O2
CFLAGS	 =
CPPFLAGS = -DSUPPORT_ION -std=c++11 -DLINUX  -DENABLE_ASSERT  -Wno-multichar -Wno-write-strings

CFLAGS += $(DEBUGFLAGS)
CPPFLAGS += $(DEBUGFLAGS)
#CFLAGS += $(OPTIMIZEFLAGS)
#CPPFLAGS += $(OPTIMIZEFLAGS)

CPPFLAGS += -D USE_RK_V4L2_HEAD_FILES

all : $(BIN)

$(COBJS) : %.o : %.c
	@echo " CC    $<"
	@$(CC) -c $< -o $@ $(INCS) $(CFLAGS)
$(CPPOBJS) : %.o : %.cpp
	@echo "CPP    $<"
	@$(CPP) -c $< -o $@ $(INCS) $(CPPFLAGS)

$(BIN) : $(COBJS) $(CPPOBJS)
	@echo "LD     video "
	@$(CPP) -o $(BIN) $(COBJS) $(CPPOBJS)
	@-rm $(COBJS) $(CPPOBJS) 
	@echo "video build success!"


clean:
	@rm -f $(BIN) $(COBJS) $(CPPOBJS)
	@echo "video clean success!"
