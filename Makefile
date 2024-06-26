PRJNAME = RL_cardGame

INCPATH = ./include
SRCPATH = ./src
BINPATH = ./bin
OBJPATH = ./obj
LIBPATH = ./lib

EXT_INC_FLAGS= -I${HOME}/include/ann -I${HOME}/include/lin_alg -I{HOME}/include
EXT_LIB_FLAGS= -L/usr/lib/x86_64-linux-gnu -L${HOME}/lib ${HOME}/lib/pcg.o ${HOME}/lib/log.o

RLS_LIB = $(PRJNAME)
DBG_LIB = $(PRJNAME)_dbg

CC = gcc
LD = gcc
AR = ar

COM_CFLAGS = -std=c11 -Wall -Wextra -I$(INCPATH) $(EXT_INC_FLAGS) -DFLD_FLT32 -DIND_INT64
OPT_CFLAGS = -flto -O3

MKL_LD_LIBS = -lmkl_rt -lm
# -Wl,--no-as-needed -lmkl_intel_lp64 -lmkl_intel_thread -lmkl_core -liomp5 -lpthread -lm -ldl
RLS_CFLAGS = -DNDEBUG $(COM_CFLAGS) $(OPT_CFLAGS)
RLS_LDFLAGS = $(OPT_CFLAGS) -L$(LIBPATH) $(EXT_LIB_FLAGS)
RLS_LD_LIBS = -lann -llin_alg_flt32 $(MKL_LD_LIBS)
DBG_CFLAGS = -DDEBUG -g $(COM_CFLAGS) 
DBG_LDFLAGS = -L$(LIBPATH) $(EXT_LIB_FLAGS) -g -no-pie -ggdb3
DBG_LD_LIBS = -lann_dbg -llin_alg_flt32_dbg $(MKL_LD_LIBS)


CFILES = $(filter-out $(SRCPATH)/main%.c, $(wildcard $(SRCPATH)/*.c))
TST_CFILES = $(filter %_test.c, $(CFILES))
NTST_CFILES = $(filter-out %_test.c, $(CFILES))
NTST_HFILES = $(filter-out %test.h, $(wildcard $(INCPATH)/*.h))
RLS_OBJS = $(patsubst $(SRCPATH)/%.c, $(OBJPATH)/%.o, $(NTST_CFILES))
DBG_OBJS = $(patsubst $(SRCPATH)/%.c, $(OBJPATH)/%_dbg.o, $(NTST_CFILES))
TST_OBJS = $(patsubst $(SRCPATH)/%.c, $(OBJPATH)/%_dbg.o, $(TST_CFILES))

.PHONY: all clean release debug test main run_test show_vars

all: debug release test main
	@echo "====== make all ======"

show_vars:
	@echo "CFILES: " $(CFILES)
	@echo "TST_CFILES: " $(TST_CFILES)
	@echo "NTST_CFILES: " $(NTST_CFILES)
	@echo "NTST_HFILES: " $(NTST_HFILES)
	@echo "DBG_OBJS: " $(DBG_OBJS)
	@echo "===== show vars ======"

$(OBJPATH)/main_dbg.o: $(SRCPATH)/main.c $(NTST_HFILES)
	@mkdir -p $(OBJPATH)
	$(CC) $(DBG_CFLAGS) -o $@ -c $<

$(OBJPATH)/main.o: $(SRCPATH)/main.c $(NTST_HFILES)
	@mkdir -p $(OBJPATH)
	$(CC) $(RLS_CFLAGS) -o $@ -c $<

$(OBJPATH)/%_test_dbg.o: $(SRCPATH)/%_test.c $(INCPATH)/test.h
	@mkdir -p $(OBJPATH)
	$(CC) $(DBG_CFLAGS) -o $@ -c $<

$(OBJPATH)/%.o: $(SRCPATH)/%.c $(INCPATH)/%.h
	@mkdir -p $(OBJPATH)
	$(CC) $(RLS_CFLAGS) -o $@ -c $<

$(OBJPATH)/%_dbg.o: $(SRCPATH)/%.c $(INCPATH)/%.h
	@mkdir -p $(OBJPATH)
	$(CC) $(DBG_CFLAGS) -o $@ -c $<

$(BINPATH)/test.out: $(OBJPATH)/main_test_dbg.o $(DBG_OBJS) $(TST_OBJS)
	@mkdir -p $(BINPATH)
	$(LD) $(DBG_LDFLAGS) -o $@ $^ $(DBG_LD_LIBS)

$(BINPATH)/main.out: $(OBJPATH)/main.o $(RLS_OBJS)
	@mkdir -p $(BINPATH)
	$(LD) $(RLS_LDFLAGS) -o $@ $^ $(RLS_LD_LIBS)

$(BINPATH)/main_dbg.out: $(OBJPATH)/main_dbg.o $(DBG_OBJS)
	@mkdir -p $(BINPATH)
	$(LD) $(DBG_LDFLAGS) -o $@ $^ $(DBG_LD_LIBS)


release: $(RLS_OBJS)
	@echo "====== make release objs ======"

debug: $(DBG_OBJS) $(TST_OBJS)
	@echo "====== make debug objs ======"

test: $(BINPATH)/test.out
	@echo "====== make test ======"

main: $(BINPATH)/main.out $(BINPATH)/main_dbg.out
	@echo "====== make main ======"

run_test: test
	$(BINPATH)/test.out
	@echo "****** test finished ******"
	@echo "====== make run_test ======"

run_main: main
	$(BINPATH)/main_dbg.out
	@echo "====== make run_main (dbg) ======"	

clean:
	rm -rf $(OBJPATH) $(BINPATH) $(LIBPATH)
	@echo "====== make clean ======"

