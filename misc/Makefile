CFLAGS+=`pkg-config --cflags cairo pango pangocairo` -Wall
LOADLIBES+=`pkg-config --libs cairo pango pangocairo` -lm

all: 16seg bpmwheel boxmaker

16seg: 16seg.c

bpmwheel: bpmwheel.c

boxmaker: boxmaker.c

clean:
	rm -f 16seg bpmwheel boxmaker

.PHONY: clean
