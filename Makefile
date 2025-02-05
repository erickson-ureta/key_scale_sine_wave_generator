sine_wave: sine_wave.c
	gcc -Wall -Wextra $^ -o sine_wave -lSDL2 -lm

clean:
	rm -rfv sine_wave
