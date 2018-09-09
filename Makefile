CC			= gcc
CXX			= g++
LINKC		= gcc
LINKCXX		= g++
CFLAGS		= -Wall -O2
ADDLIB		=  -lpthread -lmysqlclient
TOP_DIR		:= $(shell pwd)
SRC_DIRS	:= $(shell find $(TOP_DIR) -maxdepth 1 -type d)
TARGET		:= $(TOP_DIR)/bin/ttt
INC_PATH	= -I$(TOP_DIR)/inc/ -I$(TOP_DIR)/def/
C_SRCS		= $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))
C_OBJS		= $(patsubst %.c, %.o, $(C_SRCS))

all : $(TARGET)

$(TARGET) : $(C_OBJS)
	$(shell if [ ! -d $(TOP_DIR)/bin ]; then mkdir $(TOP_DIR)/bin; fi)
	$(LINKC) $(CFLAGS) -o $@ $^ $(ADDLIB)
%.o : %.c
	$(CC) $(INC_PATH) $(CFLAGS) -c -o $@ $< $(ADDLIB)

.PHONY : clean
clean :
	rm -rf $(TARGET) $(CXX_OBJS) $(C_OBJS)