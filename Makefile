OBJS = \
	dro2play.o \
	emu8950.o \
	emuadpcm.o \
	libdroplay.o

PROGS = dro2play

LIBS = -lao

-include config.mak

all: $(PROGS)

dro2play: $(OBJS)
	$(CC) -o $@ $^ $(LIBS) $(LDFLAGS)

