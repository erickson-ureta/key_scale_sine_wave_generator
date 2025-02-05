# Key scale sine wave generator

## About
This is a simple terminal-based program that generates sine wave noises that ascend in a given musical key.
This program is written in C, requires [installing the SDL2 library](https://wiki.libsdl.org/SDL2/Installation), and only supports being built in a Linux system (Ubuntu 24.04).

This will display the notes in the provided scale, alongside each notes' sine wave frequency,
This uses a pitch standard of A4 == 440Hz.
This program can play notes within a range of C4 (261.63 Hz) up to B5 (987.77 Hz).

```
 $ ./sine_wave A major
A  (440.00 Hz)
B  (493.88 Hz)
C# (554.37 Hz)
D  (587.33 Hz)
E  (659.26 Hz)
F# (739.99 Hz)
G# (830.61 Hz)
A  (880.00 Hz)
```

## Building
Install the SDL2 library (linked) above. Then simply run `make` with the provided Makefile.

## Usage

Run the program with the desired key (must be in capitals) and the mode (major/minor) for the given key.
```
usage: sine_wave <key> <"major"|"minor">
       For sharps, use # (e.g. "C#" for C sharp)
       For flats, use b (e.g. "Bb" for B flat)
```

Examples:

To generate sine wave noises that ascend in the key of C major:
```
./sine_wave C major
```

To generate sine wave noises that ascend in the key of A# minor:
```
./sine_wave A# minor
```

To generate sine wave noises that ascend in the key of Eb minor:
```
./sine_wave Eb minor
```

