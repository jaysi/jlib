DEBFLAGS = -O -g
#DEBFLAGS = -O2 -DNDEBUG

J_INCLUDE=../include
J_DOC=../doc
CFLAGS=-Winline -Wall -c -fPIC -I$(J_INCLUDE)

#objects
IF_OBJ=j_if.o
JDBIF_OBJ=jdbif.c jdbif_debug.c jdbif_table.c jdbif_type.o jdbif_cell.o jdbif_data.o
#JIF_OBJ=jacif_hlp.o jacif_usr.o jacif_grp.o jacif_obj.o jacif_mg.o jacif_dbg.o jacif_tree.o
DB_OBJ=jdb.o jdb_blk_calc.o jdb_block.o jdb_cell.o jdb_data.o jdb_data_ptr.o jdb_fav.o jdb_hash.o jdb_hdr.o jdb_index.o jdb_io.o jdb_chlog.o jdb_list.o  jdb_map.o jdb_mem.o jdb_pack.o jdb_table.o jdb_type.o jdb_undo.o jdb_wr_thread.o
IO_OBJ=hardio.o
STR_OBJ=jcs.o jpack.o
CRYPTO_OBJ=crc.o sha256.o des.o aes.o rc4.o ecc.o jhash.o jrand.o
#AC_OBJ=jac_cache.o jac.o jac_inx.o jac_group.o jac_user_dep.o jac_user.o jac_obj_inx.o jac_oex_inx.o jac_obj_ex.o jac_object.o jac_obj_dep.o jac_tree.o jac_mgmt.o jac_filter.o
LOG_OBJ=jer.o debug.o
NET_OBJ=jn_base.o jn.o jn_conn.o jn_assm.o jn_fifo.o
SVR_OBJ=jn_svr_login.o jn_svr_com.o jn_svr_poll.o jn_svr_mod.o jn_svr.o jn_svr_send.o
CLT_OBJ=jn_clt_com.o jn_clt.o
SINGLE_CLT_OBJ=jn_base.o jn_assm.o
#CORE_OBJ=$(IO_OBJ) $(STR_OBJ) $(CRYPTO_OBJ) $(AC_OBJ) $(LOG_OBJ)
CORE_OBJ=$(IO_OBJ) $(STR_OBJ) $(CRYPTO_OBJ) $(DB_OBJ) $(LOG_OBJ)
JSS_OBJ=jss.o
TABLE_OBJ=jtable.o

#hdr, src dir
HDR=$(J_INCLUDE)/*.h
SRC=*.c

#dirs
BAK=$(HOME)/backup
JLIB_DIR=../lib
TMP=../tmp

#interfaces, libs
#IF=jif jnif jncif jssif
#J_LIBS=libjac.so libjnsvr.so libjnclt.so libjnclt_s.so libjss.so
IF=jdbif jnif jncif jssif
J_LIBS=libjdb.so libjnsvr.so libjnclt.so libjnclt_s.so libjss.so

CFLAGS += $(DEBFLAGS)

all: $(J_LIBS) $(IF) jnmod

release: all
	@strip *.so
	@strip $(IF)
	@cp -f *.so ../
	@cp -f $(IF) ../

#libjac.so:$(IO_OBJ) $(STR_OBJ) $(CRYPTO_OBJ) $(AC_OBJ) $(LOG_OBJ)
#	ld -shared -olibjac.so $(CORE_OBJ) -lpthread

libjdb.so: $(IO_OBJ) $(STR_OBJ) $(CRYPTO_OBJ) $(LOG_OBJ) $(DB_OBJ)
	ld -shared -olibjdb.so $(DB_OBJ) $(IO_OBJ) $(STR_OBJ) $(CRYPTO_OBJ) $(LOG_OBJ) -lpthread

libjnsvr.so: libjdb.so $(NET_OBJ) $(SVR_OBJ) $(LOG_OBJ) $(STR_OBJ) $(CRYPTO_OBJ)
	ld -shared -olibjnsvr.so $(NET_OBJ) $(SVR_OBJ) $(LOG_OBJ) $(STR_OBJ) $(CRYPTO_OBJ) -L. libjdb.so -lpthread -ldl

libjnclt.so: $(NET_OBJ) $(CLT_OBJ) $(LOG_OBJ) $(STR_OBJ) $(CRYPTO_OBJ) $(IO_OBJ)
	ld -shared -olibjnclt.so $(NET_OBJ) $(CLT_OBJ) $(LOG_OBJ) $(STR_OBJ) $(CRYPTO_OBJ) $(IO_OBJ) -L. -lpthread

libjnclt_s.so: $(SINGLE_CLT_OBJ) $(LOG_OBJ) $(STR_OBJ) $(CRYPTO_OBJ)
	$(CC) $(CFLAGS) -I$(J_INCLUDE) -DJN_SINGLE_CLT -ojn_conn_s.o jn_conn.c
	$(CC) $(CFLAGS) -I$(J_INCLUDE) -DJN_SINGLE_CLT -ojn_s.o jn.c
	$(CC) $(CFLAGS) -I$(J_INCLUDE) -DJN_SINGLE_CLT -ojn_fifo_s.o jn_fifo.c
	$(CC) $(CFLAGS) -I$(J_INCLUDE) -DJN_SINGLE_CLT -ojn_clt_s.o jn_clt.c
	$(CC) $(CFLAGS) -I$(J_INCLUDE) -DJN_SINGLE_CLT -ojn_clt_com_s.o jn_clt_com.c
	ld -shared -olibjnclt_s.so $(SINGLE_CLT_OBJ) $(LOG_OBJ) $(STR_OBJ) $(CRYPTO_OBJ) jn_conn_s.o jn_s.o jn_fifo_s.o jn_clt_s.o jn_clt_com_s.o

libjss.so: $(JSS_OBJ) $(IO_OBJ) $(STR_OBJ) $(CRYPTO_OBJ) $(TABLE_OBJ)
	ld -shared -olibjss.so $(JSS_OBJ) $(IO_OBJ) $(STR_OBJ) $(CRYPTO_OBJ) $(LOG_OBJ)

jncif: libjnclt.so $(IF_OBJ)
	$(CC) -Wall -I$(J_INCLUDE) -ojncif jn_cltif.c $(IF_OBJ) -L. libjnclt.so -Wl,-rpath,./

#jif: libjac.so $(LOG_OBJ) $(IF_OBJ) $(JIF_OBJ)
#	$(CC) -Wall -I$(J_INCLUDE) -ojif jacif.c $(IF_OBJ) $(JIF_OBJ) -L. libjac.so -Wl,-rpath,./

jdbif: libjdb.so $(LOG_OBJ) $(IF_OBJ) $(JDBIF_OBJ)
	$(CC) -Wall -I$(J_INCLUDE) -ojdbif $(JDBIF_OBJ) $(IF_OBJ) $(JIF_OBJ) -L. libjdb.so -Wl,-rpath,./

jnif: libjnsvr.so $(IF_OBJ)
	$(CC) -Wall -I$(J_INCLUDE) -ojnif jn_svrif.c $(IF_OBJ) -L. libjnsvr.so -Wl,-rpath,./

jssif: libjss.so $(IF_OBJ) jssif_table.o $(TABLE_OBJ)
	$(CC) -Wall -I$(J_INCLUDE) -ojssif jssif.c jssif_table.o $(TABLE_OBJ) $(IF_OBJ) -L. libjss.so -Wl,-rpath,./

size:
	$(CC) -Wall -I$(J_INCLUDE) -osize sizet.c

jnmod:
	@cd jnmod; make; cd ..

clean: cleanbak cleanobj noexec
	@rm -f ../jdbif ../libjdb.so ../libjnsvr.so ../libjnclt.so
	@rm -f jdbif jncif jnif jssif

cleanobj:
	@rm -f *.o *.so

cleanbak:
	@rm -f $(J_INCLUDE)/*.h~
	@rm -f *.c~
	@rm -f Makefile~

noexec:
	@chmod -x *.c
	@chmod -x $(J_INCLUDE)/*.h
	@chmod -x Makefile

backup:
	@mkdir $(BAK)/jlib
	@mkdir $(BAK)/jlib/src
	@mkdir $(BAK)/jlib/src/jnmod
	@mkdir $(BAK)/jlib/include
	@mkdir $(BAK)/jlib/doc	
	@cp *.c Makefile $(BAK)/jlib/src
	@cp jnmod/*.c jnmod/Makefile $(BAK)/jlib/src/jnmod
	@cp $(J_INCLUDE)/*.h $(BAK)/jlib/include
	@cp $(J_DOC)/*.txt $(BAK)/jlib/doc
	@tar -cf $(BAK)/jlib$(VER).tar $(BAK)/jlib
	@gzip $(BAK)/jlib$(VER).tar
	@rm -rf $(BAK)/jlib
	
