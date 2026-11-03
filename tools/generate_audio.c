/**
 * generate_audio.c - Synthesizes retro-futuristic arcade sound effects
 * and music tracks for SPACE JAM and outputs standard PCM WAV files.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846
#define SAMPLE_RATE 44100

#pragma pack(push, 1)
typedef struct {
    char     riff[4];
    uint32_t fileSize;
    char     wave[4];
    char     fmt[4];
    uint32_t fmtSize;
    uint16_t format;
    uint16_t channels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char     data[4];
    uint32_t dataSize;
} WavHeader;
#pragma pack(pop)

static void SaveWavMono(const char *filename, const int16_t *samples, uint32_t numSamples, uint32_t sampleRate) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Failed to open %s for writing\n", filename);
        return;
    }

    WavHeader h;
    memcpy(h.riff, "RIFF", 4);
    h.fileSize = sizeof(WavHeader) - 8 + numSamples * sizeof(int16_t);
    memcpy(h.wave, "WAVE", 4);
    memcpy(h.fmt, "fmt ", 4);
    h.fmtSize = 16;
    h.format = 1; /* PCM */
    h.channels = 1; /* Mono */
    h.sampleRate = sampleRate;
    h.bitsPerSample = 16;
    h.byteRate = sampleRate * h.channels * (h.bitsPerSample / 8);
    h.blockAlign = h.channels * (h.bitsPerSample / 8);
    memcpy(h.data, "data", 4);
    h.dataSize = numSamples * sizeof(int16_t);

    fwrite(&h, sizeof(h), 1, f);
    fwrite(samples, sizeof(int16_t), numSamples, f);
    fclose(f);
    printf("Generated: %s (%u samples, %.2fs)\n", filename, numSamples, (float)numSamples / sampleRate);
}

/* Pseudo-random white noise [-1.0, 1.0] */
static float RndNoise(void) {
    return ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
}

static inline float ClampFloat(float v, float mn, float mx) {
    if (v < mn) return mn;
    if (v > mx) return mx;
    return v;
}

/*                                                       
   1. SOUND EFFECTS SYNTHESIS
                                                       */

/* Player Laser Pew: Rapid high-to-low laser sweep with harmonic edge */
static void GenSfxShoot(const char *filename) {
    float dur = 0.15f;
    uint32_t n = (uint32_t)(SAMPLE_RATE * dur);
    int16_t *buf = (int16_t *)malloc(n * sizeof(int16_t));
    float phase = 0.0f;

    for (uint32_t i = 0; i < n; i++) {
        float t = (float)i / SAMPLE_RATE;
        float progress = t / dur;

        /* Frequency sweeps from 1350Hz down to 220Hz exponentially */
        float freq = 220.0f + 1130.0f * expf(-progress * 5.0f);
        phase += 2.0f * (float)PI * freq / SAMPLE_RATE;

        /* Waveform: sine with mild square distortion */
        float s = sinf(phase);
        float sq = (s > 0.0f) ? 0.35f : -0.35f;
        float sample = s * 0.7f + sq * 0.3f;

        /* Amplitude envelope: instant attack, exponential decay */
        float env = expf(-progress * 4.2f);
        sample *= env * 0.75f;

        buf[i] = (int16_t)(ClampFloat(sample, -1.0f, 1.0f) * 32767.0f);
    }
    SaveWavMono(filename, buf, n, SAMPLE_RATE);
    free(buf);
}

/* Enemy Blaster Pulse: Deep gritty Imperial turbolaser */
static void GenSfxEnemyShoot(const char *filename) {
    float dur = 0.18f;
    uint32_t n = (uint32_t)(SAMPLE_RATE * dur);
    int16_t *buf = (int16_t *)malloc(n * sizeof(int16_t));
    float phase = 0.0f;

    for (uint32_t i = 0; i < n; i++) {
        float t = (float)i / SAMPLE_RATE;
        float progress = t / dur;

        /* Sweep from 650Hz down to 130Hz */
        float freq = 130.0f + 520.0f * expf(-progress * 4.5f);
        phase += 2.0f * (float)PI * freq / SAMPLE_RATE;

        /* Gritty saw / square blend */
        float saw = (fmodf(phase, 2.0f * (float)PI) / (float)PI) - 1.0f;
        float s = sinf(phase);
        float sample = saw * 0.5f + s * 0.5f;

        float env = expf(-progress * 4.0f);
        sample *= env * 0.70f;

        buf[i] = (int16_t)(ClampFloat(sample, -1.0f, 1.0f) * 32767.0f);
    }
    SaveWavMono(filename, buf, n, SAMPLE_RATE);
    free(buf);
}

/* Small Explosion: Crisp white noise burst + low rumble */
static void GenSfxExplosionSmall(const char *filename) {
    float dur = 0.38f;
    uint32_t n = (uint32_t)(SAMPLE_RATE * dur);
    int16_t *buf = (int16_t *)malloc(n * sizeof(int16_t));
    float lowPhase = 0.0f;
    float lpf = 0.0f;

    for (uint32_t i = 0; i < n; i++) {
        float t = (float)i / SAMPLE_RATE;
        float progress = t / dur;

        /* Noise component with lowpass filter */
        float noise = RndNoise();
        lpf += (noise - lpf) * 0.35f;

        /* Low frequency thud dropping 160Hz -> 40Hz */
        float rumbleFreq = 40.0f + 120.0f * expf(-progress * 5.0f);
        lowPhase += 2.0f * (float)PI * rumbleFreq / SAMPLE_RATE;
        float rumble = sinf(lowPhase);

        float env = expf(-progress * 5.0f);
        float sample = (lpf * 0.65f + rumble * 0.45f) * env;

        buf[i] = (int16_t)(ClampFloat(sample, -1.0f, 1.0f) * 32767.0f);
    }
    SaveWavMono(filename, buf, n, SAMPLE_RATE);
    free(buf);
}

/* Large Explosion: Heavy sub-bass detonation for tanks and large asteroids */
static void GenSfxExplosionLarge(const char *filename) {
    float dur = 0.72f;
    uint32_t n = (uint32_t)(SAMPLE_RATE * dur);
    int16_t *buf = (int16_t *)malloc(n * sizeof(int16_t));
    float lowPhase = 0.0f;
    float lpf = 0.0f;

    for (uint32_t i = 0; i < n; i++) {
        float t = (float)i / SAMPLE_RATE;
        float progress = t / dur;

        float noise = RndNoise();
        lpf += (noise - lpf) * 0.22f;

        float rumbleFreq = 25.0f + 110.0f * expf(-progress * 3.5f);
        lowPhase += 2.0f * (float)PI * rumbleFreq / SAMPLE_RATE;
        float rumble = sinf(lowPhase) + 0.35f * sinf(lowPhase * 0.5f);

        float env = expf(-progress * 3.2f);
        float sample = (lpf * 0.55f + rumble * 0.60f) * env;

        buf[i] = (int16_t)(ClampFloat(sample, -1.0f, 1.0f) * 32767.0f);
    }
    SaveWavMono(filename, buf, n, SAMPLE_RATE);
    free(buf);
}

/* Player Hit: Shield impact & metal resonance */
static void GenSfxPlayerHit(const char *filename) {
    float dur = 0.25f;
    uint32_t n = (uint32_t)(SAMPLE_RATE * dur);
    int16_t *buf = (int16_t *)malloc(n * sizeof(int16_t));
    float phase1 = 0.0f, phase2 = 0.0f;

    for (uint32_t i = 0; i < n; i++) {
        float t = (float)i / SAMPLE_RATE;
        float progress = t / dur;

        phase1 += 2.0f * (float)PI * (160.0f * expf(-progress * 6.0f) + 45.0f) / SAMPLE_RATE;
        phase2 += 2.0f * (float)PI * 440.0f / SAMPLE_RATE;

        float s1 = sinf(phase1);
        float s2 = sinf(phase2) * expf(-progress * 12.0f); /* transient click */
        float noise = RndNoise() * expf(-progress * 18.0f);

        float env = expf(-progress * 4.5f);
        float sample = (s1 * 0.6f + s2 * 0.3f + noise * 0.3f) * env;

        buf[i] = (int16_t)(ClampFloat(sample, -1.0f, 1.0f) * 32767.0f);
    }
    SaveWavMono(filename, buf, n, SAMPLE_RATE);
    free(buf);
}

/* Power-up Chime: Uplifting 4-note ascending crystal arpeggio (C5 - E5 - G5 - C6) */
static void GenSfxPowerup(const char *filename) {
    float dur = 0.42f;
    uint32_t n = (uint32_t)(SAMPLE_RATE * dur);
    int16_t *buf = (int16_t *)malloc(n * sizeof(int16_t));

    float freqs[4] = { 523.25f, 659.25f, 783.99f, 1046.50f }; /* C5, E5, G5, C6 */
    float noteDur = dur / 4.0f;
    float phase = 0.0f;

    for (uint32_t i = 0; i < n; i++) {
        float t = (float)i / SAMPLE_RATE;
        int noteIdx = (int)(t / noteDur);
        if (noteIdx > 3) noteIdx = 3;
        float noteTime = fmodf(t, noteDur);
        float noteEnv = expf(-noteTime * 14.0f);

        phase += 2.0f * (float)PI * freqs[noteIdx] / SAMPLE_RATE;
        /* Bell / chime: pure fundamental + 2nd harmonic */
        float s = sinf(phase) * 0.7f + sinf(phase * 2.0f) * 0.3f;
        float sample = s * noteEnv * 0.70f;

        buf[i] = (int16_t)(ClampFloat(sample, -1.0f, 1.0f) * 32767.0f);
    }
    SaveWavMono(filename, buf, n, SAMPLE_RATE);
    free(buf);
}

/* Boss Appear: Ominous Imperial Dreadnought War Horn / Klaxon */
static void GenSfxBossAppear(const char *filename) {
    float dur = 1.4f;
    uint32_t n = (uint32_t)(SAMPLE_RATE * dur);
    int16_t *buf = (int16_t *)malloc(n * sizeof(int16_t));
    float ph1 = 0.0f, ph2 = 0.0f, ph3 = 0.0f;

    for (uint32_t i = 0; i < n; i++) {
        float t = (float)i / SAMPLE_RATE;
        float progress = t / dur;

        /* Deep D2 (73.4Hz) + A2 (110Hz) + F3 (174.6Hz) chord */
        float f1 = 73.4f + sinf(t * 5.0f) * 2.0f;
        float f2 = 110.0f;
        float f3 = 174.6f;

        ph1 += 2.0f * (float)PI * f1 / SAMPLE_RATE;
        ph2 += 2.0f * (float)PI * f2 / SAMPLE_RATE;
        ph3 += 2.0f * (float)PI * f3 / SAMPLE_RATE;

        float s1 = sinf(ph1);
        float s2 = sinf(ph2);
        float s3 = sinf(ph3);

        /* Brass swell envelope */
        float env = (progress < 0.25f) ? (progress / 0.25f) : expf(-(progress - 0.25f) * 1.8f);
        float sample = (s1 * 0.45f + s2 * 0.35f + s3 * 0.25f) * env * 0.85f;

        buf[i] = (int16_t)(ClampFloat(sample, -1.0f, 1.0f) * 32767.0f);
    }
    SaveWavMono(filename, buf, n, SAMPLE_RATE);
    free(buf);
}

/* Boss Explode: Cataclysmic multi-stage thunder detonation */
static void GenSfxBossExplode(const char *filename) {
    float dur = 1.8f;
    uint32_t n = (uint32_t)(SAMPLE_RATE * dur);
    int16_t *buf = (int16_t *)malloc(n * sizeof(int16_t));
    float lpf = 0.0f;
    float subPhase = 0.0f;

    for (uint32_t i = 0; i < n; i++) {
        float t = (float)i / SAMPLE_RATE;
        float progress = t / dur;

        /* Heavy rumble + multiple secondary shockwaves */
        float noise = RndNoise();
        lpf += (noise - lpf) * (0.15f + 0.15f * expf(-progress * 2.0f));

        float subFreq = 30.0f + 60.0f * expf(-progress * 2.0f);
        subPhase += 2.0f * (float)PI * subFreq / SAMPLE_RATE;
        float sub = sinf(subPhase);

        /* Secondary explosion bursts at t = 0.3s, 0.6s, 0.9s */
        float burst = 0.0f;
        if (t > 0.3f) burst += expf(-(t - 0.3f) * 6.0f) * RndNoise() * 0.4f;
        if (t > 0.65f) burst += expf(-(t - 0.65f) * 7.0f) * RndNoise() * 0.5f;

        float mainEnv = expf(-progress * 1.6f);
        float sample = (lpf * 0.55f + sub * 0.50f + burst) * mainEnv * 0.9f;

        buf[i] = (int16_t)(ClampFloat(sample, -1.0f, 1.0f) * 32767.0f);
    }
    SaveWavMono(filename, buf, n, SAMPLE_RATE);
    free(buf);
}

/* Menu Select: Crisp short tick */
static void GenSfxMenuSelect(const char *filename) {
    float dur = 0.05f;
    uint32_t n = (uint32_t)(SAMPLE_RATE * dur);
    int16_t *buf = (int16_t *)malloc(n * sizeof(int16_t));
    float phase = 0.0f;

    for (uint32_t i = 0; i < n; i++) {
        float t = (float)i / SAMPLE_RATE;
        float progress = t / dur;
        phase += 2.0f * (float)PI * 720.0f / SAMPLE_RATE;
        float sample = sinf(phase) * expf(-progress * 16.0f) * 0.5f;
        buf[i] = (int16_t)(ClampFloat(sample, -1.0f, 1.0f) * 32767.0f);
    }
    SaveWavMono(filename, buf, n, SAMPLE_RATE);
    free(buf);
}

/* Menu Confirm: Affirmative upward chirp */
static void GenSfxMenuConfirm(const char *filename) {
    float dur = 0.12f;
    uint32_t n = (uint32_t)(SAMPLE_RATE * dur);
    int16_t *buf = (int16_t *)malloc(n * sizeof(int16_t));
    float phase = 0.0f;

    for (uint32_t i = 0; i < n; i++) {
        float t = (float)i / SAMPLE_RATE;
        float progress = t / dur;
        float freq = 440.0f + 440.0f * progress;
        phase += 2.0f * (float)PI * freq / SAMPLE_RATE;
        float sample = (sinf(phase) + 0.2f * sinf(phase * 2.0f)) * expf(-progress * 5.0f) * 0.65f;
        buf[i] = (int16_t)(ClampFloat(sample, -1.0f, 1.0f) * 32767.0f);
    }
    SaveWavMono(filename, buf, n, SAMPLE_RATE);
    free(buf);
}

/* Game Over: Melancholy descending minor motif */
static void GenSfxGameOver(const char *filename) {
    float dur = 1.2f;
    uint32_t n = (uint32_t)(SAMPLE_RATE * dur);
    int16_t *buf = (int16_t *)malloc(n * sizeof(int16_t));
    float notes[4] = { 392.00f, 329.63f, 293.66f, 261.63f }; /* G4, E4b, D4, C4 */
    float noteDur = dur / 4.0f;
    float phase = 0.0f;

    for (uint32_t i = 0; i < n; i++) {
        float t = (float)i / SAMPLE_RATE;
        int noteIdx = (int)(t / noteDur);
        if (noteIdx > 3) noteIdx = 3;
        float noteTime = fmodf(t, noteDur);
        float noteEnv = expf(-noteTime * 4.0f);

        phase += 2.0f * (float)PI * notes[noteIdx] / SAMPLE_RATE;
        float s = sinf(phase) + 0.3f * sinf(phase * 2.0f);
        float sample = s * noteEnv * 0.65f;

        buf[i] = (int16_t)(ClampFloat(sample, -1.0f, 1.0f) * 32767.0f);
    }
    SaveWavMono(filename, buf, n, SAMPLE_RATE);
    free(buf);
}

/*                                                       
   2. MUSIC TRACK SYNTHESIS
                                                       */

/* Intro / Prologue Space Opera Fanfare
   Epic, majestic symphonic synth in G minor / Bb major.
   Duration: ~16.0s seamless loop */
static void GenMusicIntro(const char *filename) {
    float dur = 16.0f;
    uint32_t n = (uint32_t)(SAMPLE_RATE * dur);
    int16_t *buf = (int16_t *)malloc(n * sizeof(int16_t));

    /* Star Wars style heroic progression:
       Section 1 (0-4s): Bb Major Brass Fanfare (Bb3, D4, F4) with low Bb1 root
       Section 2 (4-8s): F Major (A3, C4, F4) with low F1 root
       Section 3 (8-12s): Eb Major (G3, Bb3, Eb4) with low Eb1 root
       Section 4 (12-16s): F Sus4 to F Major cadence into loop */

    float rootFreqs[4] = { 58.27f, 43.65f, 38.89f, 43.65f }; /* Bb1, F1, Eb1, F1 */
    float pad1[4] = { 233.08f, 220.00f, 196.00f, 220.00f }; /* Bb3, A3, G3, A3 */
    float pad2[4] = { 293.66f, 261.63f, 233.08f, 293.66f }; /* D4, C4, Bb3, D4 */
    float pad3[4] = { 349.23f, 349.23f, 311.13f, 349.23f }; /* F4, F4, Eb4, F4 */

    /* Heroic Lead Fanfare Notes across the 16 seconds (time, note freq, length) */
    typedef struct { float start; float freq; float len; } FanfareNote;
    static const FanfareNote melody[] = {
        { 0.0f,  233.08f, 0.5f }, /* Bb3 */
        { 0.5f,  349.23f, 1.2f }, /* F4  */
        { 1.8f,  311.13f, 0.35f}, /* Eb4 */
        { 2.2f,  293.66f, 0.35f}, /* D4  */
        { 2.6f,  261.63f, 0.35f}, /* C4  */
        { 3.0f,  466.16f, 1.4f }, /* Bb4 */

        { 4.6f,  349.23f, 0.9f }, /* F4  */
        { 5.6f,  311.13f, 0.35f}, /* Eb4 */
        { 6.0f,  293.66f, 0.35f}, /* D4  */
        { 6.4f,  261.63f, 0.35f}, /* C4  */
        { 6.8f,  466.16f, 1.5f }, /* Bb4 */

        { 8.5f,  349.23f, 0.8f }, /* F4  */
        { 9.4f,  311.13f, 0.4f }, /* Eb4 */
        { 9.9f,  293.66f, 0.4f }, /* D4  */
        { 10.4f, 311.13f, 0.4f }, /* Eb4 */
        { 10.9f, 261.63f, 1.4f }, /* C4  */

        { 12.5f, 293.66f, 0.6f }, /* D4  */
        { 13.2f, 349.23f, 0.6f }, /* F4  */
        { 13.9f, 466.16f, 1.8f }, /* Bb4 climactic resolution */
    };
    int melodyCount = sizeof(melody) / sizeof(melody[0]);

    float phRoot = 0.0f, phP1 = 0.0f, phP2 = 0.0f, phP3 = 0.0f, phMel = 0.0f;

    for (uint32_t i = 0; i < n; i++) {
        float t = (float)i / SAMPLE_RATE;
        int chordIdx = (int)(t / 4.0f);
        if (chordIdx > 3) chordIdx = 3;

        /* Low Bass Pedal */
        phRoot += 2.0f * (float)PI * rootFreqs[chordIdx] / SAMPLE_RATE;
        float bass = sinf(phRoot) * 0.35f + 0.15f * sinf(phRoot * 2.0f);

        /* Lush Brass Pad Chords */
        phP1 += 2.0f * (float)PI * pad1[chordIdx] / SAMPLE_RATE;
        phP2 += 2.0f * (float)PI * pad2[chordIdx] / SAMPLE_RATE;
        phP3 += 2.0f * (float)PI * pad3[chordIdx] / SAMPLE_RATE;

        float pad = (sinf(phP1) + sinf(phP2) + sinf(phP3)) * 0.14f;

        /* Lead Melodic Line */
        float lead = 0.0f;
        for (int m = 0; m < melodyCount; m++) {
            if (t >= melody[m].start && t < melody[m].start + melody[m].len) {
                float dt = t - melody[m].start;
                float env = sinf((dt / melody[m].len) * (float)PI); /* smooth bell arc */
                float vibrato = sinf(dt * 30.0f) * 2.5f;
                phMel += 2.0f * (float)PI * (melody[m].freq + vibrato) / SAMPLE_RATE;
                /* Rich French horn / brass timbre: combination of saw + triangle */
                float saw = (fmodf(phMel, 2.0f * (float)PI) / (float)PI) - 1.0f;
                lead = (sinf(phMel) * 0.6f + saw * 0.4f) * env * 0.45f;
                break;
            }
        }

        /* Seamless loop crossfade near edges */
        float edgeFade = 1.0f;
        if (t < 0.05f) edgeFade = t / 0.05f;
        else if (t > dur - 0.05f) edgeFade = (dur - t) / 0.05f;

        float mix = (bass + pad + lead) * edgeFade * 0.85f;
        buf[i] = (int16_t)(ClampFloat(mix, -1.0f, 1.0f) * 32767.0f);
    }

    SaveWavMono(filename, buf, n, SAMPLE_RATE);
    free(buf);
}

/* Battle Space Combat Theme:
   High energy 130 BPM retro synthwave / chiptune battle with driving bass,
   punchy kick & snare, arpeggios, and melodic space dogfight lead!
   12 bars @ 130 BPM = 22.1538s */
static void GenMusicBattle(const char *filename) {
    float bpm = 130.0f;
    float beatDur = 60.0f / bpm;             /* ~0.4615s */
    float sixteenthDur = beatDur / 4.0f;      /* ~0.1154s */
    int totalBars = 12;
    float dur = totalBars * 4.0f * beatDur;  /* ~22.1538s */

    uint32_t n = (uint32_t)(SAMPLE_RATE * dur);
    int16_t *buf = (int16_t *)malloc(n * sizeof(int16_t));

    /* Chord progression across 12 bars (each chord lasts 2 bars = 8 beats):
       Bars 1-2:   D minor (D2, F3, A3, D4)
       Bars 3-4:   Bb major (Bb1, F3, Bb3, D4)
       Bars 5-6:   C major (C2, E3, G3, C4)
       Bars 7-8:   A minor (A1, E3, A3, C4)
       Bars 9-10:  G minor (G1, D3, G3, Bb3)
       Bars 11-12: A major dominant buildup (A1, E3, A3, C#4) */

    float rootFreqs[6] = { 73.42f, 58.27f, 65.41f, 55.00f, 49.00f, 55.00f }; /* D2, Bb1, C2, A1, G1, A1 */
    float chordArp[6][4] = {
        { 146.83f, 174.61f, 220.00f, 293.66f }, /* D3, F3, A3, D4 */
        { 116.54f, 174.61f, 233.08f, 293.66f }, /* Bb2, F3, Bb3, D4 */
        { 130.81f, 164.81f, 196.00f, 261.63f }, /* C3, E3, G3, C4 */
        { 110.00f, 164.81f, 220.00f, 261.63f }, /* A2, E3, A3, C4 */
        {  98.00f, 146.83f, 196.00f, 233.08f }, /* G2, D3, G3, Bb3 */
        { 110.00f, 164.81f, 220.00f, 277.18f }, /* A2, E3, A3, C#4 */
    };

    /* Battle Lead Solo Melody (starting at Bar 5 through Bar 12) */
    typedef struct { float beat; float freq; float lenBeats; } BattleNote;
    static const BattleNote leadRiff[] = {
        /* Bar 5-6 (C major section) */
        { 16.0f, 587.33f, 0.75f }, /* D5 */
        { 17.0f, 659.25f, 0.75f }, /* E5 */
        { 18.0f, 698.46f, 1.50f }, /* F5 */
        { 20.0f, 659.25f, 0.75f }, /* E5 */
        { 21.0f, 587.33f, 0.75f }, /* D5 */
        { 22.0f, 523.25f, 1.80f }, /* C5 */

        /* Bar 7-8 (A minor section) */
        { 24.0f, 440.00f, 1.00f }, /* A4 */
        { 25.5f, 523.25f, 0.50f }, /* C5 */
        { 26.0f, 587.33f, 1.80f }, /* D5 */
        { 28.0f, 659.25f, 0.75f }, /* E5 */
        { 29.0f, 698.46f, 0.75f }, /* F5 */
        { 30.0f, 783.99f, 1.80f }, /* G5 */

        /* Bar 9-10 (G minor section) */
        { 32.0f, 880.00f, 1.20f }, /* A5 high climax! */
        { 34.0f, 783.99f, 0.75f }, /* G5 */
        { 35.0f, 698.46f, 0.75f }, /* F5 */
        { 36.0f, 659.25f, 1.80f }, /* E5 */
        { 38.0f, 587.33f, 1.80f }, /* D5 */

        /* Bar 11-12 (A dominant climax buildup) */
        { 40.0f, 554.37f, 1.00f }, /* C#5 */
        { 41.5f, 587.33f, 0.50f }, /* D5  */
        { 42.0f, 659.25f, 1.50f }, /* E5  */
        { 44.0f, 739.99f, 0.75f }, /* F#5 */
        { 45.0f, 830.61f, 0.75f }, /* G#5 */
        { 46.0f, 880.00f, 1.80f }, /* A5  */
    };
    int leadCount = sizeof(leadRiff) / sizeof(leadRiff[0]);

    float phBass = 0.0f, phArp = 0.0f, phLead = 0.0f;
    float phKick = 0.0f;

    for (uint32_t i = 0; i < n; i++) {
        float t = (float)i / SAMPLE_RATE;
        float curBeat = t / beatDur;
        int barIdx = (int)(curBeat / 4.0f);
        if (barIdx > 11) barIdx = 11;
        int progIdx = barIdx / 2;
        if (progIdx > 5) progIdx = 5;

        /* -- 1. DRUMS -- */
        float beatFrac = fmodf(curBeat, 1.0f);
        int beatInBar = (int)curBeat % 4;
        float kick = 0.0f;
        float snare = 0.0f;
        float hihat = 0.0f;

        /* Kick on beats 0 and 2 */
        if (beatInBar == 0 || beatInBar == 2) {
            float kickT = beatFrac * beatDur;
            if (kickT < 0.16f) {
                float kFreq = 42.0f + 110.0f * expf(-kickT * 32.0f);
                phKick += 2.0f * (float)PI * kFreq / SAMPLE_RATE;
                kick = sinf(phKick) * expf(-kickT * 18.0f) * 0.85f;
            }
        }

        /* Snare on beats 1 and 3 */
        if (beatInBar == 1 || beatInBar == 3) {
            float snareT = beatFrac * beatDur;
            if (snareT < 0.20f) {
                float tone = sinf(2.0f * (float)PI * 180.0f * snareT) * expf(-snareT * 22.0f);
                float noise = RndNoise() * expf(-snareT * 16.0f);
                snare = (tone * 0.4f + noise * 0.6f) * 0.75f;
            }
        }

        /* Hi-hat on 16th notes */
        float sixteenthFrac = fmodf(t, sixteenthDur);
        if (sixteenthFrac < 0.035f) {
            hihat = RndNoise() * expf(-sixteenthFrac * 120.0f) * 0.18f;
        }

        /* -- 2. DRIVING 16TH-NOTE BASSLINE -- */
        int sixteenthIdx = (int)(t / sixteenthDur);
        float bassNoteFrac = fmodf(t, sixteenthDur);
        float bassFreq = rootFreqs[progIdx];
        /* Bass octaves alternate on upbeat */
        if ((sixteenthIdx % 4) == 2) bassFreq *= 2.0f;
        phBass += 2.0f * (float)PI * bassFreq / SAMPLE_RATE;

        float bassSaw = (fmodf(phBass, 2.0f * (float)PI) / (float)PI) - 1.0f;
        float bassEnv = expf(-bassNoteFrac * 18.0f);
        float bass = (bassSaw * 0.6f + sinf(phBass) * 0.4f) * bassEnv * 0.42f;

        /* -- 3. COSMIC SYNTH ARPEGGIO -- */
        int arpIdx = sixteenthIdx % 4;
        float arpFreq = chordArp[progIdx][arpIdx];
        phArp += 2.0f * (float)PI * arpFreq / SAMPLE_RATE;
        float arpSquare = (sinf(phArp) > 0.0f) ? 0.35f : -0.35f;
        float arpEnv = expf(-bassNoteFrac * 14.0f);
        float arp = arpSquare * arpEnv * 0.16f;

        /* -- 4. DOGFIGHT LEAD SYNTH -- */
        float lead = 0.0f;
        for (int m = 0; m < leadCount; m++) {
            float noteStartSec = leadRiff[m].beat * beatDur;
            float noteLenSec = leadRiff[m].lenBeats * beatDur;
            if (t >= noteStartSec && t < noteStartSec + noteLenSec) {
                float dt = t - noteStartSec;
                float env = sinf((dt / noteLenSec) * (float)PI);
                float vib = sinf(dt * 36.0f) * 3.0f;
                phLead += 2.0f * (float)PI * (leadRiff[m].freq + vib) / SAMPLE_RATE;
                float saw = (fmodf(phLead, 2.0f * (float)PI) / (float)PI) - 1.0f;
                lead = (sinf(phLead) * 0.5f + saw * 0.5f) * env * 0.38f;
                break;
            }
        }

        /* Seamless loop smoothing */
        float edgeFade = 1.0f;
        if (t < 0.04f) edgeFade = t / 0.04f;
        else if (t > dur - 0.04f) edgeFade = (dur - t) / 0.04f;

        float mix = (kick + snare + hihat + bass + arp + lead) * edgeFade * 0.88f;
        buf[i] = (int16_t)(ClampFloat(mix, -1.0f, 1.0f) * 32767.0f);
    }

    SaveWavMono(filename, buf, n, SAMPLE_RATE);
    free(buf);
}

static void GenSfxRadioChirp(const char *filename) {
    float dur = 0.16f;
    uint32_t n = (uint32_t)(dur * SAMPLE_RATE);
    int16_t *buf = (int16_t *)malloc(n * sizeof(int16_t));
    if (!buf) return;
    float phase = 0.0f;
    for (uint32_t i = 0; i < n; i++) {
        float p = (float)i / (float)n;
        float freq = (p < 0.5f) ? 1800.0f : 2400.0f;
        phase += 2.0f * (float)PI * freq / SAMPLE_RATE;
        float subT = (p < 0.5f) ? (p * 2.0f) : ((p - 0.5f) * 2.0f);
        float sample = sinf(phase) * (1.0f - subT) * 0.85f;
        buf[i] = (int16_t)(ClampFloat(sample, -1.0f, 1.0f) * 32767.0f);
    }
    SaveWavMono(filename, buf, n, SAMPLE_RATE);
    free(buf);
}

static void GenSfxRadioStatic(const char *filename) {
    float dur = 0.12f;
    uint32_t n = (uint32_t)(dur * SAMPLE_RATE);
    int16_t *buf = (int16_t *)malloc(n * sizeof(int16_t));
    if (!buf) return;
    float phase = 0.0f;
    for (uint32_t i = 0; i < n; i++) {
        float p = (float)i / (float)n;
        float noise = RndNoise();
        phase += 2.0f * (float)PI * 180.0f / SAMPLE_RATE;
        float sample = (noise * 0.75f + sinf(phase) * 0.25f) * (1.0f - p * 0.8f) * 0.75f;
        buf[i] = (int16_t)(ClampFloat(sample, -1.0f, 1.0f) * 32767.0f);
    }
    SaveWavMono(filename, buf, n, SAMPLE_RATE);
    free(buf);
}

static void GenSfxRadioBeep(const char *filename) {
    float dur = 0.04f;
    uint32_t n = (uint32_t)(dur * SAMPLE_RATE);
    int16_t *buf = (int16_t *)malloc(n * sizeof(int16_t));
    if (!buf) return;
    float phase = 0.0f;
    for (uint32_t i = 0; i < n; i++) {
        float p = (float)i / (float)n;
        phase += 2.0f * (float)PI * 920.0f / SAMPLE_RATE;
        float sample = sinf(phase) * expf(-p * 14.0f) * 0.70f;
        buf[i] = (int16_t)(ClampFloat(sample, -1.0f, 1.0f) * 32767.0f);
    }
    SaveWavMono(filename, buf, n, SAMPLE_RATE);
    free(buf);
}

static void GenSfxWarpJump(const char *filename) {
    float dur = 1.10f;
    uint32_t n = (uint32_t)(dur * SAMPLE_RATE);
    int16_t *buf = (int16_t *)malloc(n * sizeof(int16_t));
    if (!buf) return;
    float phase = 0.0f;
    for (uint32_t i = 0; i < n; i++) {
        float p = (float)i / (float)n;
        float freq = (p < 0.6f) ? (140.0f + 1460.0f * (p / 0.6f) * (p / 0.6f)) : 90.0f;
        phase += 2.0f * (float)PI * freq / SAMPLE_RATE;
        float noise = RndNoise();
        float env = (p < 0.6f) ? (p / 0.6f) : (1.0f - (p - 0.6f) / 0.4f);
        float sample = (sinf(phase) * 0.65f + noise * 0.35f) * env * 0.90f;
        buf[i] = (int16_t)(ClampFloat(sample, -1.0f, 1.0f) * 32767.0f);
    }
    SaveWavMono(filename, buf, n, SAMPLE_RATE);
    free(buf);
}

static void GenSfxMedalAward(const char *filename) {
    float dur = 0.75f;
    uint32_t n = (uint32_t)(dur * SAMPLE_RATE);
    int16_t *buf = (int16_t *)malloc(n * sizeof(int16_t));
    if (!buf) return;
    float phase = 0.0f;
    float notes[4] = { 523.25f, 659.25f, 783.99f, 1046.50f };
    for (uint32_t i = 0; i < n; i++) {
        float p = (float)i / (float)n;
        int idx = (int)(p * 4.0f);
        if (idx > 3) idx = 3;
        phase += 2.0f * (float)PI * notes[idx] / SAMPLE_RATE;
        float subT = fmodf(p * 4.0f, 1.0f);
        float sample = sinf(phase) * (1.0f - subT * 0.7f) * 0.85f;
        buf[i] = (int16_t)(ClampFloat(sample, -1.0f, 1.0f) * 32767.0f);
    }
    SaveWavMono(filename, buf, n, SAMPLE_RATE);
    free(buf);
}

int main(void) {
    printf("Synthesizing audio assets for SPACE JAM...\n");

    /* SFX */
    GenSfxShoot("assets/audio/shoot.wav");
    GenSfxEnemyShoot("assets/audio/enemy_shoot.wav");
    GenSfxExplosionSmall("assets/audio/explosion_small.wav");
    GenSfxExplosionLarge("assets/audio/explosion_large.wav");
    GenSfxPlayerHit("assets/audio/player_hit.wav");
    GenSfxPowerup("assets/audio/powerup.wav");
    GenSfxBossAppear("assets/audio/boss_appear.wav");
    GenSfxBossExplode("assets/audio/boss_explode.wav");
    GenSfxMenuSelect("assets/audio/menu_select.wav");
    GenSfxMenuConfirm("assets/audio/menu_confirm.wav");
    GenSfxGameOver("assets/audio/game_over.wav");
    GenSfxRadioChirp("assets/audio/radio_chirp.wav");
    GenSfxRadioStatic("assets/audio/radio_static.wav");
    GenSfxRadioBeep("assets/audio/radio_beep.wav");
    GenSfxWarpJump("assets/audio/warp_jump.wav");
    GenSfxMedalAward("assets/audio/medal_award.wav");

    /* Music */
    GenMusicIntro("assets/audio/music_intro.wav");
    GenMusicBattle("assets/audio/music.wav");

    printf("All audio assets synthesized successfully!\n");
    return 0;
}

