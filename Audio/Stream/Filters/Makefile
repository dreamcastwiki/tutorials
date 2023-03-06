TARGET = wav.elf
OBJS = romdisk.o main.o

all: clean $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET) $(OBJS)
	-rm -f romdisk.o romdisk.img

$(TARGET): $(OBJS) 
	kos-cc -o $(TARGET) $(OBJS) $(DATAOBJS) $(OBJEXTRA) -lwav

romdisk.img:
	$(KOS_GENROMFS) -f romdisk.img -d romdisk -v

romdisk.o: romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o romdisk.img romdisk romdisk.o

rei: dist1
	$(REICAST) GAME.CDI

lxd: dist1
	$(LXDREAM) GAME.CDI

run: 
	$(KOS_LOADER) $(TARGET)
#	dc-tool -p -b 115200 -x $(TARGET)

dist:
	rm -f $(OBJS) romdisk.o romdisk.img
	$(KOS_STRIP) $(TARGET)

dist1: $(TARGET)
	$(KOS_STRIP) $(TARGET)
	$(KOS_OBJCOPY) -O binary $(TARGET) prog.bin
	$(KOS_SCRAMBLE) prog.bin 1ST_READ.BIN
	mkdir -p ISO
	cp 1ST_READ.BIN ISO
	mkisofs -C 0,11702 -V DCTEST -G ${KOS_BASE}/IP.BIN -J -R -l -o GAME.ISO ISO
	$(CREATE_CDI) GAME.ISO GAME.CDI
	rm GAME.ISO
	rm 1ST_READ.BIN
	rm $(TARGET)
