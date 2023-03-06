
TARGET = filesystem.elf
OBJS = romdisk.o main.o

all: clean $(TARGET)

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET) $(OBJS)
	-rm -f romdisk.o romdisk.img
	-rm -f stage1.img
	-rm -f stage2.img

$(TARGET): $(OBJS) 
	kos-cc -o $(TARGET) $(OBJS) -lkosfat
	$(KOS_GENROMFS) -f stage1.img -d stage1 -v
	$(KOS_GENROMFS) -f stage2.img -d stage2 -v

romdisk.img:
	$(KOS_GENROMFS) -f romdisk.img -d romdisk -v

romdisk.o: romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o romdisk.img romdisk romdisk.o

run: $(TARGET)
	$(KOS_IP_LOADER) $(TARGET)

rei: dist
	$(REICAST) GAME.CDI

lxd: dist
	$(LXDREAM) GAME.CDI

dist: $(TARGET)
	$(KOS_STRIP) $(TARGET)
	$(KOS_OBJCOPY) -O binary $(TARGET) prog.bin
	$(KOS_SCRAMBLE) prog.bin 1ST_READ.BIN
	mkdir -p ISO
	cp 1ST_READ.BIN ISO
	cp stage1.img ISO
	cp stage2.img ISO
	mkisofs -C 0,11702 -V DCTEST -G ${KOS_BASE}/IP.BIN -J -R -l -o GAME.ISO ISO
	$(CREATE_CDI) GAME.ISO GAME.CDI
	rm GAME.ISO
	rm 1ST_READ.BIN
	rm prog.bin
	rm -r ISO


