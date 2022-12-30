
all: bin/serial

serial: bin/serial

bin/serial: src/draw_hexgrid_offsetarray.c src/serial.c
	g++ $^ -O3 -o $@ -lsfml-graphics -lsfml-window -lsfml-system -lm


clean:
	rm -rf bin/*