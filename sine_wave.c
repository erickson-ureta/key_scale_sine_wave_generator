#define _USE_MATH_DEFINES
#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

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
            SDL_Delay(250 - delay_time);

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

void initialize_note_freqs(double note_frequencies[NUM_NOTES]) {
    int interval = -9; // Start at C4, which is -9 semis away from A4
    for (int i = 0; i < NUM_NOTES; i++) {
        double factor = pow(2.0, 1.0/12.0);
        note_frequencies[i] = A4_FREQ_HZ * pow(factor, interval);
        // Major scale
        interval += (i == 2 || i == 6) ? 1 : 2;
    }
}

int main() {
    signal(SIGINT, signal_handler);

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

    double note_frequencies[NUM_NOTES];
    initialize_note_freqs(note_frequencies);

    note_loop(&audiodata, note_frequencies);

    return 0;
}
