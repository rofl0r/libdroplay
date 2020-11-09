OBJS = \
	dro2play.o \
	dbopl.o \
	libdroplay.o

PROGS = dro2play

LIBS = -lao
CXXFLAGS = -fpermissive

-include config.mak

all: $(PROGS)

dro2play: $(OBJS)
	$(CXX) -o $@ $^ $(LIBS) $(LDFLAGS)
clean:
	rm -f *.o $(PROGS)

.PHONY: all clean
