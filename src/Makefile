TARGET := dev_remote_upgrade
CC := gcc
 
INCLUDE:= -I ../include
 
LIBS:= -L ../lib/linux -lmosquitto -lcurl -lssl -lcrypto -lpthread -lm
 
CSRCS := $(wildcard *.c)
OBJS := $(patsubst %.c, %.o, $(CSRCS))
 
$(TARGET): $(OBJS) 
	$(CC) $+ -o $@  $(LIBS)
	cp $(TARGET) ../bin -rf
%.o:%.c
	$(CC) -c -g $< -o $@ $(INCLUDE)
clean:
	rm -rf $(OBJS) $(TARGET)
	#rm *.o
