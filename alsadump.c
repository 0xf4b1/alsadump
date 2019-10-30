#define _GNU_SOURCE

#include <alsa/asoundlib.h>
#include <dlfcn.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>

#define CHANNELS 2;
#define FORMAT 2; // 16 bit sample size, divided by 8

static unsigned int cur_channels = CHANNELS;
static unsigned int cur_format = FORMAT;
static bool dump_only = false;

static FILE* pFile;

static typeof(snd_pcm_writei) *real_snd_pcm_writei = NULL;
static typeof(snd_pcm_set_params) *real_snd_pcm_set_params = NULL;
static typeof(snd_pcm_open) *real_snd_pcm_open = NULL;
static typeof(snd_pcm_start) *real_snd_pcm_start = NULL;
static typeof(snd_pcm_close) *real_snd_pcm_close = NULL;


void create_buffer() {
    if (pFile == NULL) {
        char buffer [20];
        sprintf(buffer, "%d.raw", time(NULL));
        printf("Dumping to %s\n", buffer);
        pFile = fopen(buffer,"wb");
    }   
}

static void init_params(void) {
    const char* s = getenv("DUMP_ONLY");
    if (s) {
        dump_only = true;
    }
    s = NULL;

    s = getenv("CHANNELS");
    if (s) {
        cur_channels = atoi(s);
    }
    s = NULL;

    s = getenv("FORMAT");
    if (s) {
        cur_format = atoi(s) / 8;
    }
}

snd_pcm_sframes_t snd_pcm_writei(snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size) {
    if (!real_snd_pcm_writei) {
        real_snd_pcm_writei = dlsym(RTLD_NEXT, "snd_pcm_writei");
        init_params();
    }

    if (pFile != NULL) {
        fwrite(buffer, size * cur_channels * cur_format, 1, pFile);
    }

    if (dump_only) {
        return size;
    }

    return real_snd_pcm_writei(pcm, buffer, size);
}

int snd_pcm_set_params(snd_pcm_t *pcm, snd_pcm_format_t format, snd_pcm_access_t access, unsigned int channels, unsigned int rate, int soft_resample, unsigned int latency) {
    if (!real_snd_pcm_set_params) real_snd_pcm_set_params = dlsym(RTLD_NEXT, "snd_pcm_set_params");

    printf("format: %u, access: %u, channels: %u, rate: %u, soft_resample: %i, latency: %u\n", format, access, channels, rate, soft_resample, latency);
    
    cur_channels = channels;

    // https://www.alsa-project.org/alsa-doc/alsa-lib/group___p_c_m.html#gaa14b7f26877a812acbb39811364177f8
    if (format >= 1 && format <= 2) {
        cur_format = 1;
    } else if (format >= 3 && format <= 6) {
        cur_format = 2;
    } else if (format >= 6 && format <= 9) {
        cur_format = 3;
    } else if (format >= 10 && format <= 13) {
        cur_format = 4;
    }

    return real_snd_pcm_set_params(pcm, format, access, channels, rate, soft_resample, latency);
}

int snd_pcm_open(snd_pcm_t **pcmp, const char *name, snd_pcm_stream_t stream, int mode) {
    if (!real_snd_pcm_open) real_snd_pcm_open = dlsym(RTLD_NEXT, "snd_pcm_open");
    
    printf("%s: %s, %x\n", "snd_pcm_open", name, pcmp);
    
    create_buffer();

    return real_snd_pcm_open(pcmp, name, stream, mode);
}

int snd_pcm_start(snd_pcm_t *pcm) {
    if (!real_snd_pcm_start) real_snd_pcm_start = dlsym(RTLD_NEXT, "snd_pcm_start");

    printf("%s: %x\n", "snd_pcm_start", pcm);

    if (pFile != NULL) {
        fclose(pFile);
        pFile = NULL;
    }

    create_buffer();
    
    return real_snd_pcm_start(pcm);
}

int snd_pcm_close(snd_pcm_t *pcm) {
    if (!real_snd_pcm_close) real_snd_pcm_close = dlsym(RTLD_NEXT, "snd_pcm_close");

    printf("%s: %x\n", "snd_pcm_close", pcm);

    if (pFile != NULL) {
        fclose(pFile);
        pFile = NULL;
    }

    return real_snd_pcm_close(pcm);
}