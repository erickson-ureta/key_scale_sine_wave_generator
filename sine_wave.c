#define _USE_MATH_DEFINES

#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <regex.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define NUM_NOTES 8
#define A4_FREQ_HZ 440.0

typedef struct AudioData {
    double frequency;
    double phase;
} AudioData;

void audio_callback(void *userdata, Uint8 *stream, int len) {
    AudioData *audiodata = (AudioData *) userdata;

    double volume_mult = 0.5;

    int sample_rate = 44100;
    double phase_increment = (2.0*M_PI * audiodata->frequency) / sample_rate;
    Sint16 *buffer = (Sint16 *) stream;
    int samples = len / sizeof(Sint16);

    for (int i = 0; i < samples; i++) {
        buffer[i] = (Sint16) (32767 * sin(audiodata->phase) * volume_mult);
        audiodata->phase += phase_increment;
        if (audiodata->phase > 2.0*M_PI) audiodata->phase -= 2.0*M_PI;
    }
}

void signal_handler(int signum) {
    printf("\nReceived QUIT signal: %d\n", signum);
    SDL_CloseAudio();
    SDL_Quit();
    exit(0);
}

int initialize_audio_spec_data(SDL_AudioSpec *spec, AudioData *audiodata) {
    if (!spec || !audiodata) {
        return 0;
    }

    audiodata->frequency = A4_FREQ_HZ;
    audiodata->phase = 0.0;

    spec->freq = 44100;
    spec->format = AUDIO_S16SYS;
    spec->channels = 1;
    spec->samples = 4096;
    spec->callback = audio_callback;
    spec->userdata = audiodata;

    return 1;
}

void note_loop(AudioData *audiodata, double note_frequencies[NUM_NOTES]) {
    int i = 0;
    audiodata->frequency = note_frequencies[i];

    Uint32 last_time = SDL_GetTicks();
    SDL_PauseAudio(0);
    printf("frequency = %.2fHz\n", audiodata->frequency);

    while (1) {
        Uint32 current_time = SDL_GetTicks();
        if (current_time - last_time >= 290) { // Change frequency every 500 ms

            // Pause in between notes
            SDL_PauseAudio(1);
            Uint32 delay_time = SDL_GetTicks() - current_time;
            SDL_Delay(200 - delay_time);

            i++;
            if (i >= NUM_NOTES) break;

            audiodata->frequency = note_frequencies[i];
            printf("frequency = %.2fHz\n", audiodata->frequency);
            SDL_PauseAudio(0);
            last_time = SDL_GetTicks();
        }
        SDL_Delay(100); // Small delay to prevent 100% CPU usage
    }
}

int get_interval_from_key(char *key) {
    if (!strcmp(key, "B#") || !strcmp(key, "C")) {
        return -9;
    }
    if (!strcmp(key, "C#") || !strcmp(key, "Db")) {
        return -8;
    }
    if (!strcmp(key, "D")) {
        return -7;
    }
    if (!strcmp(key, "D#") || !strcmp(key, "Eb")) {
        return -6;
    }
    if (!strcmp(key, "E") || !strcmp(key, "Fb")) {
        return -5;
    }
    if (!strcmp(key, "E#") || !strcmp(key, "F")) {
        return -4;
    }
    if (!strcmp(key, "F#") || !strcmp(key, "Gb")) {
        return -3;
    }
    if (!strcmp(key, "G")) {
        return -2;
    }
    if (!strcmp(key, "G#") || !strcmp(key, "Ab")) {
        return -1;
    }
    if (!strcmp(key, "A")) {
        return 0;
    }
    if (!strcmp(key, "A#") || !strcmp(key, "Bb")) {
        return 1;
    }
    if (!strcmp(key, "B") || !strcmp(key, "Cb")) {
        return 2;
    }

    // Return A4 by default
    return 0;
}

void initialize_note_freqs(double note_frequencies[NUM_NOTES], char *key, int major_scale) {
    int interval = get_interval_from_key(key); // Start at C4, which is -9 semis away from A4
    for (int i = 0; i < NUM_NOTES; i++) {
        double factor = pow(2.0, 1.0/12.0);
        note_frequencies[i] = A4_FREQ_HZ * pow(factor, interval);
        if (major_scale) {
            interval += (i == 2 || i == 6) ? 1 : 2;
        }
        else {  // Minor scale
            interval += (i == 1 || i == 5) ? 1 : 2;
        }
    }
}

void print_usage() {
    printf("usage: sine_wave <key> <\"major\"|\"minor\">\n");
    printf("       For sharps, use # (e.g. \"C#\" for C sharp\n");
    printf("       For flats, use b (e.g. \"Bb\" for B flat\n");
}

int parse_args(int argc, char *argv[])
{
    if (argc != 3) return 0;

    // Check key
    char *key = argv[1];
    regex_t regex;
    if (regcomp(&regex, "^[A-Ga-g](#|b)?$", REG_EXTENDED)) {
        printf("Unable to compile regex\n");
        return 0;
    }
    int regex_ret = regexec(&regex, key, 0, NULL, 0);
    if (regex_ret == REG_NOMATCH) {
        printf("Invalid key = %s \n", key);
        return 0;
    }

    // Check mode
    char *mode = argv[2];
    if (strcmp(mode, "major") && strcmp(mode, "minor")) {
        printf("Invalid mode = %s \n", mode);
        return 0;
    }

    return 1;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, signal_handler);

    if (!parse_args(argc, argv)) {
        print_usage();
        return 1;
    }

    char *key = argv[1];
    double note_frequencies[NUM_NOTES];
    int major_scale = !strcmp(argv[2], "major");
    initialize_note_freqs(note_frequencies, key, major_scale);

    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        printf("Error initializing SDL; %s\n", SDL_GetError());
        return 1;
    }

    AudioData audiodata = {0};
    SDL_AudioSpec spec = {0};
    if (!initialize_audio_spec_data(&spec, &audiodata)) {
        printf("Unable to initialize audio specs and data\n");
        return 1;
    }

    if (SDL_OpenAudio(&spec, NULL) < 0) {
        printf("SDL unable to open audio; %s\n", SDL_GetError());
        return 1;
    }

    note_loop(&audiodata, note_frequencies);

    return 0;
}
