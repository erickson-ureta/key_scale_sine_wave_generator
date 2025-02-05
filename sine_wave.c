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

#define A4_FREQ_HZ 440.0
#define AMP_VOL_HI 0.5
#define AMP_VOL_LO 0.0

#define NUM_NOTES 8
#define PITCH_CLASS_LEN 3

typedef struct AudioData {
    double frequency;  // Pitch
    double phase;
    double amplitude;  // Volume
} AudioData;

typedef struct NoteData {
    char name[2];
    int interval; // i.e. distance from A4. A4 == 0.
    double frequency;
} NoteData;

char *g_pitch_classes[12][PITCH_CLASS_LEN] = {
    { "B#", "C" , ""   },   // 0
    { "C#", "Db", ""   },   // 1
    { "D" , "Cx", ""   },   // 2
    { "D#", "Eb", ""   },   // 3
    { "E" , "Fb", "Dx" },   // 4
    { "E#", "F" , ""   },   // 5
    { "F#", "Gb", ""   },   // 6
    { "G" , "Fx", ""   },   // 7
    { "G#", "Ab", ""   },   // 8
    { "A" , "Gx", ""   },   // 9
    { "A#", "Bb", ""   },   // 10
    { "B" , "Cb", "Ax" },   // 11
};

void audio_callback(void *userdata, Uint8 *stream, int len) {
    AudioData *audiodata = (AudioData *) userdata;

    int sample_rate = 44100;
    double phase_increment = (2.0*M_PI * audiodata->frequency) / sample_rate;
    Sint16 *buffer = (Sint16 *) stream;
    int samples = len / sizeof(Sint16);

    for (int i = 0; i < samples; i++) {
        buffer[i] = (Sint16) (32767 * sin(audiodata->phase) * audiodata->amplitude);
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
    audiodata->amplitude = AMP_VOL_HI;

    spec->freq = 44100;
    spec->format = AUDIO_S16SYS;
    spec->channels = 1;
    spec->samples = 4096;
    spec->callback = audio_callback;
    spec->userdata = audiodata;

    return 1;
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

int _get_pitch_class_idx(char *key) {
    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < PITCH_CLASS_LEN; j++) {
            if (!strcmp(key, g_pitch_classes[i][j])) {
                return i;
            }
        }
    }

    return -1;
}

void _initialize_note_names(NoteData note_data[NUM_NOTES], char *key, int major_scale) {
    int pc_idx = _get_pitch_class_idx(key);
    char cur_note_name = key[0];
    for (int i = 0; i < NUM_NOTES; i++) {
        for (int j = 0; j < PITCH_CLASS_LEN; j++) {
            if (i == 0 && strcmp(key, g_pitch_classes[pc_idx][j])) continue;
            if (cur_note_name == g_pitch_classes[pc_idx][j][0]) {
                strncpy(note_data[i].name, g_pitch_classes[pc_idx][j], 2);
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

void _initialize_note_frequencies(NoteData notes_in_scale[NUM_NOTES], char *key, int major_scale) {
    int interval = get_dist_from_a4(key);  // Distance from A4, in semitones
                                           //
    for (int i = 0; i < NUM_NOTES; i++) {
        notes_in_scale[i].interval = interval;

        // Generate frequency based on interval
        double factor = pow(2.0, 1.0/12.0);
        notes_in_scale[i].frequency = A4_FREQ_HZ * pow(factor, interval);
        if (major_scale) {
            interval += (i == 2 || i == 6) ? 1 : 2;
        }
        else {  // Minor scale
            interval += (i == 1 || i == 4) ? 1 : 2;
        }
    }
}

void initialize_notes_in_scale(NoteData notes_in_scale[NUM_NOTES], char *key, int major_scale) {
    _initialize_note_names(notes_in_scale, key, major_scale);
    _initialize_note_frequencies(notes_in_scale, key, major_scale);
}

void loop_through_scale(AudioData *audiodata, NoteData notes_in_scale[NUM_NOTES]) {
    SDL_PauseAudio(0);
    for (int i = 0; i < NUM_NOTES; i++) {
        audiodata->frequency = notes_in_scale[i].frequency;
        audiodata->amplitude = AMP_VOL_HI;
        printf("%-2s (%.2f Hz)\n", notes_in_scale[i].name, audiodata->frequency);
        SDL_Delay(225);
        audiodata->amplitude = AMP_VOL_LO;
        SDL_Delay(90);
    }
    SDL_PauseAudio(1);
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
    NoteData notes_in_scale[NUM_NOTES];
    initialize_notes_in_scale(notes_in_scale, key, major_scale);

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

    loop_through_scale(&audiodata, notes_in_scale);

    SDL_CloseAudio();
    SDL_Quit();

    return 0;
}
