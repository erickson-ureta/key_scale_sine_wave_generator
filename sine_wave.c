#define _USE_MATH_DEFINES

#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <regex.h>
#include <unistd.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define NUM_NOTES 8
#define A4_FREQ_HZ 440.0

typedef struct AudioData {
    double frequency;
    double phase;
} AudioData;

typedef struct NoteData {
    char name[3];
    char interval; // i.e. distance from A4. A4 == 0.
    double frequency;
} NoteData;

void audio_callback(void *userdata, Uint8 *stream, int len) {
    AudioData *audiodata = (AudioData *) userdata;

    double volume_mult = 0.5; // amplitude

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
    if (!spec || !audiodata) return 0;

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

void note_loop(AudioData *audiodata, NoteData note_data[NUM_NOTES]) {
    for (int i = 0; i < NUM_NOTES; i++) {
        audiodata->frequency = note_data[i].frequency;
        printf("%-2s (%.2f Hz)\n", note_data[i].name, audiodata->frequency);
        SDL_PauseAudio(0);
        SDL_Delay(200);
        SDL_PauseAudio(1);
        SDL_Delay(85);
    }
}

int get_dist_from_a4(char *note) {
    if (!strcmp(note, "B#") || !strcmp(note, "C")) {
        return -9;
    }
    if (!strcmp(note, "C#") || !strcmp(note, "Db")) {
        return -8;
    }
    if (!strcmp(note, "D")) {
        return -7;
    }
    if (!strcmp(note, "D#") || !strcmp(note, "Eb")) {
        return -6;
    }
    if (!strcmp(note, "E") || !strcmp(note, "Fb")) {
        return -5;
    }
    if (!strcmp(note, "E#") || !strcmp(note, "F")) {
        return -4;
    }
    if (!strcmp(note, "F#") || !strcmp(note, "Gb")) {
        return -3;
    }
    if (!strcmp(note, "G")) {
        return -2;
    }
    if (!strcmp(note, "G#") || !strcmp(note, "Ab")) {
        return -1;
    }
    if (!strcmp(note, "A")) {
        return 0;
    }
    if (!strcmp(note, "A#") || !strcmp(note, "Bb")) {
        return 1;
    }
    if (!strcmp(note, "B") || !strcmp(note, "Cb")) {
        return 2;
    }

    // Return A4 by default
    return 0;
}

int _get_pitch_class_idx(char *key, char *pitch_classes[12][2]) {
    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 2; j++) {
            if (!strcmp(key, pitch_classes[i][j])) {
                return i;
            }
        }
    }

    return -1;
}

void _initialize_note_data_names(NoteData note_data[NUM_NOTES], char *key, int major_scale) {
    char *pitch_classes[12][2] = {
        { "B#", "C"  }, // 0
        { "C#", "Db" }, // 1
        { "D" , "Cx" }, // 2
        { "D#", "Eb" }, // 3
        { "E" , "Fb" }, // 4
        { "E#", "F"  }, // 5
        { "F#", "Gb" }, // 6
        { "G" , "Fx" }, // 7
        { "G#", "Ab" }, // 8
        { "A" , "Gx" }, // 9
        { "A#", "Bb" }, // 10
        { "B" , "Cb" }, // 11
    };

    int pc_idx = _get_pitch_class_idx(key, pitch_classes);
    char cur_note_name = key[0];
    for (int i = 0; i < NUM_NOTES; i++) {
        //printf("---\n");
        //printf("i = %d\n", i);
        //printf("pc_idx = %d\n", pc_idx);
        //printf("cur_note_name = %c\n", cur_note_name);
        for (int j = 0; j < 2; j++) {
            if (i == 0 && strcmp(key, pitch_classes[pc_idx][j])) continue;
            if (cur_note_name == pitch_classes[pc_idx][j][0]) {
                strncpy(note_data[i].name, pitch_classes[pc_idx][j], 2);
                cur_note_name++;
                if (cur_note_name > 'G') cur_note_name = 'A';
                break;
            }
        }

        if (major_scale) {
            pc_idx += (i == 2 || i == 6) ? 1 : 2;
        }
        else {  // Minor scale
            pc_idx += (i == 1 || i == 4) ? 1 : 2;
        }
        pc_idx = pc_idx % 12;
    }
}

void initialize_note_data(NoteData note_data[NUM_NOTES], char *key, int major_scale) {
    _initialize_note_data_names(note_data, key, major_scale);

    int interval = get_dist_from_a4(key);  // Distance from A4, in semitones
    for (int i = 0; i < NUM_NOTES; i++) {
        note_data[i].interval = interval;

        // Generate frequency based on interval
        double factor = pow(2.0, 1.0/12.0);
        note_data[i].frequency = A4_FREQ_HZ * pow(factor, interval);
        if (major_scale) {
            interval += (i == 2 || i == 6) ? 1 : 2;
        }
        else {  // Minor scale
            interval += (i == 1 || i == 4) ? 1 : 2;
        }
    }
}

void print_usage() {
    printf("usage: sine_wave <key> <\"major\"|\"minor\">\n");
    printf("       For sharps, use # (e.g. \"C#\" for C sharp)\n");
    printf("       For flats, use b (e.g. \"Bb\" for B flat)\n");
}

int parse_args(int argc, char *argv[]) {
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
    int major_scale = !strcmp(argv[2], "major");
    NoteData note_data[NUM_NOTES];
    initialize_note_data(note_data, key, major_scale);

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

    note_loop(&audiodata, note_data);

    SDL_CloseAudio();
    SDL_Quit();

    return 0;
}
