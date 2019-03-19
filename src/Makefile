TARGET := ZBox_Housekeeper
CC := gcc
 
INCLUDE:= -I ../include
 
LIBS:= -L ../lib/linux -lmosquitto -lcurl -lssl -lcrypto -lpthread -lm
 
CSRCS := $(wildcard *.c)
OBJS := $(patsubst %.c, %.o, $(CSRCS))
 
$(TARGET): $(OBJS) 
	$(CC) $+ -o $@  $(LIBS)
	cp $(TARGET) ../bin/$(TARGET) -rf
%.o:%.c
	$(CC) -c -g $< -o $@ $(INCLUDE)
clean:
	rm -rf $(OBJS) $(TARGET) ../bin/$(TARGET)
	#rm *.o
