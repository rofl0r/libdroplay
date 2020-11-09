OBJS = \
	dro2play.o \
	opl.o \
	libdroplay.o

PROGS = dro2play

LIBS = -lao

-include config.mak

all: $(PROGS)

dro2play: $(OBJS)
	$(CC) -o $@ $^ $(LIBS) $(LDFLAGS)
clean:
	rm -f *.o $(PROGS)

.PHONY: all clean
