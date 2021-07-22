
CC=gcc
#CC= tcc
INSTALL_DIR=/usr/bin
MAN_INSTALL_DIR=/usr/share/man/man1
CCC= g++
GIT_HASH= $(shell git rev-parse > /dev/null 2>&1 && git rev-parse --short HEAD || echo no)
OPTLEVEL= -O3 -s -march=native -DSISA_GIT_HASH=\"$(GIT_HASH)\"
#"
MORECFLAGS=-DUSE_COMPUTED_GOTO -DUSE_TERMIOS -DUSE_UNSIGNED_INT -DATTRIB_NOINLINE
SDL2CFLAGS=-DUSE_COMPUTED_GOTO -DUSE_SDL2 -DUSE_UNSIGNED_INT
CFLAGS= $(MORECFLAGS) $(OPTLEVEL)
CFLAGS_SDL2= $(SDL2CFLAGS) $(OPTLEVEL)
CPPFLAGS= $(MORECFLAGS) $(OPTLEVEL) -lm -Wno-unused-function -Wno-absolute-value -std=c++17 -finline-limit=64000 -fno-math-errno


all: main asm_programs

q:
	admin $(MAKE) -B install
	admin $(MAKE) libc
	admin $(MAKE) clean
	git add .
	git commit -m "Developer time" || echo "nothing to commit"
	git push || echo "nothing to push"
	./asmbuild.sh

d:
	admin $(MAKE) -B install
	admin $(MAKE) libc
	admin $(MAKE) clean
	./asmbuild.sh

sisa16_emu:
	$(CC) $(CFLAGS) isa.c -o sisa16_emu  || $(CC) $(CFLAGS) isa.c -o sisa16_emu 
	@echo "~~Built emulator."

sisa16_asm:
	$(CC) $(CFLAGS) assembler.c -o sisa16_asm    || $(CC) $(CFLAGS) assembler.c -o sisa16_asm 
	@echo "~~Built assembler (Which has an emulator built into it.)"

sisa16_dbg:
	$(CC) $(CFLAGS) debugger.c -o sisa16_dbg    || $(CC) $(CFLAGS) debugger.c -o sisa16_dbg  
	@echo "~~Built debugger."

# SDL2 versions, if you want to mess with graphics and audio

sisa16_sdl2_emu:
	$(CC) $(CFLAGS_SDL2) isa.c -o sisa16_sdl2_emu -lSDL2  -DUSE_SDL2 || echo "Cannot build the sdl2 version of the emulator."
	@echo "~~Built SDL2 emulator."

sisa16_sdl2_asm:
	$(CC) $(CFLAGS_SDL2) assembler.c -o sisa16_sdl2_asm -lSDL2  -DUSE_SDL2 || echo "Cannot build the sdl2 version of assembler."
	@echo "~~Built SDL2 assembler (Which has an emulator built into it.)"

sisa16_sdl2_dbg:
	$(CC) $(CFLAGS_SDL2) debugger.c -o sisa16_sdl2_dbg  -DUSE_SDL2 -lSDL2 || echo "Cannot build the sdl2 version of the debugger."
	@echo "~~Built SDL2 debugger."





main: sisa16_asm sisa16_emu sisa16_dbg

asm: sisa16_asm
	./asm_compile.sh

isa_constexpr:
	$(CCC) $(CPPFLAGS) *.cpp -o isa_constexpr

#used by github actions.
check:
#effectively, a check.
	sudo $(MAKE) -B install
	sudo $(MAKE) libc
	sudo $(MAKE) clean
	./asmbuild.sh
	sisa16_asm -C
	sisa16_asm -v
	sisa16_asm -dis clock.bin 0x10000
	sisa16_asm -dis clock.bin 0x20000
	sisa16_asm -fdis echo.bin 0
	sisa16_asm -dis echo2.bin 0x20000
	sisa16_asm -fdis switchcase.bin 0
	sisa16_asm -fdis controlflow_1.bin 0
	
install: sisa16_asm sisa16_emu sisa16_dbg
	@cp ./sisa16_emu $(INSTALL_DIR)/ || cp ./sisa16_emu.exe $(INSTALL_DIR)/ || echo "ERROR!!! Cannot install sisa16_emu"
	@cp ./sisa16_dbg $(INSTALL_DIR)/ || cp ./sisa16_dbg.exe $(INSTALL_DIR)/ || echo "ERROR!!! Cannot install sisa16_dbg"
	@cp ./sisa16_asm $(INSTALL_DIR)/ || cp ./sisa16_asm.exe $(INSTALL_DIR)/ || echo "ERROR!!! Cannot install sisa16_asm"
	@mkdir /usr/include/sisa16/ || echo "sisa16 include directory either already exists or cannot be created."
	@cp *.hasm /usr/include/sisa16/ || echo "ERROR!!! Cannot install libc.hasm. It is the main utility library for sisa16"
	@cp ./*.1 $(MAN_INSTALL_DIR)/ || echo "Could not install manpages."

install_sdl2: sisa16_asm sisa16_emu sisa16_dbg sisa16_sdl2_asm sisa16_sdl2_emu sisa16_sdl2_dbg
	@cp ./sisa16_emu $(INSTALL_DIR)/ || cp ./sisa16_emu.exe $(INSTALL_DIR)/ || echo "ERROR!!! Cannot install sisa16_emu"
	@cp ./sisa16_dbg $(INSTALL_DIR)/ || cp ./sisa16_dbg.exe $(INSTALL_DIR)/ || echo "ERROR!!! Cannot install sisa16_dbg"
	@cp ./sisa16_asm $(INSTALL_DIR)/ || cp ./sisa16_asm.exe $(INSTALL_DIR)/ || echo "ERROR!!! Cannot install sisa16_asm"
	@cp ./sisa16_sdl2_emu $(INSTALL_DIR)/ || cp ./sisa16_sdl2_emu.exe $(INSTALL_DIR)/ || echo "ERROR!!! Cannot install sisa16_sdl2_emu"
	@cp ./sisa16_sdl2_asm $(INSTALL_DIR)/ || cp ./sisa16_sdl2_asm.exe $(INSTALL_DIR)/ || echo "ERROR!!! Cannot install sisa16_sdl2_asm"
	@cp ./sisa16_sdl2_dbg $(INSTALL_DIR)/ || cp ./sisa16_sdl2_dbg.exe $(INSTALL_DIR)/ || echo "ERROR!!! Cannot install sisa16_sdl2_dbg"
	@mkdir /usr/include/sisa16/ || echo "sisa16 include directory either already exists or cannot be created."
	@cp ./libc.hasm /usr/include/sisa16/ || echo "ERROR!!! Cannot install libc.hasm. It is the main utility library for sisa16"
	@cp ./*.1 $(MAN_INSTALL_DIR)/ || echo "Could not install manpages."

libc:
	sisa16_asm -i libc.asm -o libc.bin
	@mkdir /usr/include/sisa16/ || echo "sisa16 include directory either already exists or cannot be created."
	@cp *.hasm /usr/include/sisa16/ && echo "Installed libraries." || echo "ERROR!!! Cannot install headers"
	@cp libc.bin.hasm.tmp /usr/include/sisa16/libc_pre.hasm && echo "Installed libraries." || echo "ERROR!!! Cannot install libc_pre.hasm. It is the main utility library for sisa16"
	@cp libc.bin /usr/include/sisa16/libc_pre.bin && echo "Installed libraries." || echo "ERROR!!! Cannot install libc_pre.bin It is the main utility library for sisa16"

uninstall:
	rm -f $(INSTALL_DIR)/sisa16*
	rm -f $(MAN_INSTALL_DIR)/sisa16_emu.1
	rm -f $(MAN_INSTALL_DIR)/sisa16_dbg.1
	rm -f $(MAN_INSTALL_DIR)/sisa16_asm.1
	@echo "Uninstalled."
	@echo "Note that if you have libraries under /usr/include/sisa16/, they were *not* removed."

clean:
	rm -f *.exe *.dsk *.out *.o *.bin *.tmp sisa16_emu sisa16_asm sisa16_dbg sisa16_sdl2_emu sisa16_sdl2_asm sisa16_sdl2_dbg isa_constexpr rbytes
	clear || echo "cannot clear?"
