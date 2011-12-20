DEBFLAGS = -O -g
#DEBFLAGS = -O2 -DNDEBUG

J_INCLUDE=../../include
CFLAGS=-Winline -Wall -c -fPIC -I$(J_INCLUDE)

JNMOD_OBJ=jnmod.o
MOD_OBJ=schat.so echo.so fileshare.so

CFLAGS += $(DEBFLAGS)

all: $(MOD_OBJ)

schat.so: $(JNMOD_OBJ) schat.o
	ld -shared -oschat.so $(JNMOD_OBJ) schat.o
	
echo.so: $(JNMOD_OBJ) echo.o
	ld -shared -oecho.so $(JNMOD_OBJ) echo.o
	
fileshare.so: $(JNMOD_OBJ) fileshare.o
	ld -shared -ofileshare.so $(JNMOD_OBJ) fileshare.o
	
clean: cleanbak cleanobj noexec
	@rm -f ../jif ../libjac.so ../libjnsvr.so ../libjnclt.so
	@rm -f jif jncif jnif

cleanobj:
	@rm -f *.o *.so

cleanbak:	
	@rm -f *.c~
	@rm -f Makefile~
	
noexec:
	@chmod -x *.c	
	@chmod -x Makefile
		
