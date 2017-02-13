


PRU_SUPPORT_INCLUDE_PATH = /home/debian/TestCode/pru-software-support-package/include
PRU_LIB_PATH = /usr/share/ti/cgt-pru/lib
PRU_INC_PATH = /usr/share/ti/cgt-pru/include

C_FLAGS = -i $(PRU_LIB_PATH) -i $(PRU_INC_PATH) -i $(PRU_SUPPORT_INCLUDE_PATH) -i . --endian=little --define am3359 --define pru0 --silicon_version=3 -o1 -k

C_SRCS = main.c spi.c
C_OBJS = $(C_SRCS:.c=.obj)

TARGET = firmware.out



all: makefile pru_loader $(TARGET) text.bin

pru_loader: pru_loader.c 
	gcc -o pru_loader pru_loader.c -lprussdrv

$(TARGET): $(C_OBJS)
	clpru  $(C_FLAGS) -z lnk.cmd -o $(TARGET) $(C_OBJS)

%.obj: %.c
	clpru $(C_FLAGS) -c $<   

text.bin: $(TARGET)
	hexpru bin.cmd $(TARGET)

clean:
	rm -f $(C_OBJS)
	rm -f $(TARGET)
	rm -f pru_loader
	rm -f text.bin data.bin
	rm -f $(C_OBJS:.obj=.asm)
