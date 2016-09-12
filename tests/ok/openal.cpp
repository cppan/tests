/*
local_settings:
    build:
        use_shared_libs: true
dependencies:
    - pvt.cppan.demo.kcat.openal
*/

/*
 * OpenAL Tone Generator Test
 *
 * Copyright (c) 2015 by Chris Robinson <chris.kcat@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* This file contains a test for generating waveforms and plays them for a
 * given length of time. Intended to inspect the behavior of the mixer by
 * checking the output with a spectrum analyzer and oscilloscope.
 *
 * TODO: This would actually be nicer as a GUI app with buttons to start and
 * stop individual waveforms, include additional whitenoise and pinknoise
 * generators, and have the ability to hook up EFX filters and effects.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "AL/al.h"
#include "AL/alc.h"
#include "AL/alext.h"

#ifndef ALHELPERS_H
#define ALHELPERS_H

#include "AL/alc.h"
#include "AL/al.h"
#include "AL/alext.h"

#include "threads.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Some helper functions to get the name from the channel and type enums. */
const char *ChannelsName(ALenum chans);
const char *TypeName(ALenum type);

/* Helpers to convert frame counts and byte lengths. */
ALsizei FramesToBytes(ALsizei size, ALenum channels, ALenum type);
ALsizei BytesToFrames(ALsizei size, ALenum channels, ALenum type);

/* Retrieves a compatible buffer format given the channel configuration and
 * sample type. If an alIsBufferFormatSupportedSOFT-compatible function is
 * provided, it will be called to find the closest-matching format from
 * AL_SOFT_buffer_samples. Returns AL_NONE (0) if no supported format can be
 * found. */
ALenum GetFormat(ALenum channels, ALenum type, LPALISBUFFERFORMATSUPPORTEDSOFT palIsBufferFormatSupportedSOFT);

/* Loads samples into a buffer using the standard alBufferData call, but with a
 * LPALBUFFERSAMPLESSOFT-compatible prototype. Assumes internalformat is valid
 * for alBufferData, and that channels and type match it. */
void AL_APIENTRY wrap_BufferSamples(ALuint buffer, ALuint samplerate,
                                    ALenum internalformat, ALsizei samples,
                                    ALenum channels, ALenum type,
                                    const ALvoid *data);

/* Easy device init/deinit functions. InitAL returns 0 on success. */
int InitAL(char ***argv, int *argc);
void CloseAL(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ALHELPERS_H */


/*
 * OpenAL Helpers
 *
 * Copyright (c) 2011 by Chris Robinson <chris.kcat@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* This file contains routines to help with some menial OpenAL-related tasks,
 * such as opening a device and setting up a context, closing the device and
 * destroying its context, converting between frame counts and byte lengths,
 * finding an appropriate buffer format, and getting readable strings for
 * channel configs and sample types. */

#include <stdio.h>
#include <string.h>

#include "AL/al.h"
#include "AL/alc.h"
#include "AL/alext.h"


/* InitAL opens a device and sets up a context using default attributes, making
 * the program ready to call OpenAL functions. */
int InitAL(char ***argv, int *argc)
{
    const ALCchar *name;
    ALCdevice *device;
    ALCcontext *ctx;

    /* Open and initialize a device */
    device = NULL;
    if(argc && argv && *argc > 1 && strcmp((*argv)[0], "-device") == 0)
    {
        device = alcOpenDevice((*argv)[1]);
        if(!device)
            fprintf(stderr, "Failed to open \"%s\", trying default\n", (*argv)[1]);
        (*argv) += 2;
        (*argc) -= 2;
    }
    if(!device)
        device = alcOpenDevice(NULL);
    if(!device)
    {
        fprintf(stderr, "Could not open a device!\n");
        return 1;
    }

    ctx = alcCreateContext(device, NULL);
    if(ctx == NULL || alcMakeContextCurrent(ctx) == ALC_FALSE)
    {
        if(ctx != NULL)
            alcDestroyContext(ctx);
        alcCloseDevice(device);
        fprintf(stderr, "Could not set a context!\n");
        return 1;
    }

    name = NULL;
    if(alcIsExtensionPresent(device, "ALC_ENUMERATE_ALL_EXT"))
        name = alcGetString(device, ALC_ALL_DEVICES_SPECIFIER);
    if(!name || alcGetError(device) != AL_NO_ERROR)
        name = alcGetString(device, ALC_DEVICE_SPECIFIER);
    printf("Opened \"%s\"\n", name);

    return 0;
}

/* CloseAL closes the device belonging to the current context, and destroys the
 * context. */
void CloseAL(void)
{
    ALCdevice *device;
    ALCcontext *ctx;

    ctx = alcGetCurrentContext();
    if(ctx == NULL)
        return;

    device = alcGetContextsDevice(ctx);

    alcMakeContextCurrent(NULL);
    alcDestroyContext(ctx);
    alcCloseDevice(device);
}


/* GetFormat retrieves a compatible buffer format given the channel config and
 * sample type. If an alIsBufferFormatSupportedSOFT-compatible function is
 * provided, it will be called to find the closest-matching format from
 * AL_SOFT_buffer_samples. Returns AL_NONE (0) if no supported format can be
 * found. */
ALenum GetFormat(ALenum channels, ALenum type, LPALISBUFFERFORMATSUPPORTEDSOFT palIsBufferFormatSupportedSOFT)
{
    ALenum format = AL_NONE;

    /* If using AL_SOFT_buffer_samples, try looking through its formats */
    if(palIsBufferFormatSupportedSOFT)
    {
        /* AL_SOFT_buffer_samples is more lenient with matching formats. The
         * specified sample type does not need to match the returned format,
         * but it is nice to try to get something close. */
        if(type == AL_UNSIGNED_BYTE_SOFT || type == AL_BYTE_SOFT)
        {
            if(channels == AL_MONO_SOFT) format = AL_MONO8_SOFT;
            else if(channels == AL_STEREO_SOFT) format = AL_STEREO8_SOFT;
            else if(channels == AL_QUAD_SOFT) format = AL_QUAD8_SOFT;
            else if(channels == AL_5POINT1_SOFT) format = AL_5POINT1_8_SOFT;
            else if(channels == AL_6POINT1_SOFT) format = AL_6POINT1_8_SOFT;
            else if(channels == AL_7POINT1_SOFT) format = AL_7POINT1_8_SOFT;
        }
        else if(type == AL_UNSIGNED_SHORT_SOFT || type == AL_SHORT_SOFT)
        {
            if(channels == AL_MONO_SOFT) format = AL_MONO16_SOFT;
            else if(channels == AL_STEREO_SOFT) format = AL_STEREO16_SOFT;
            else if(channels == AL_QUAD_SOFT) format = AL_QUAD16_SOFT;
            else if(channels == AL_5POINT1_SOFT) format = AL_5POINT1_16_SOFT;
            else if(channels == AL_6POINT1_SOFT) format = AL_6POINT1_16_SOFT;
            else if(channels == AL_7POINT1_SOFT) format = AL_7POINT1_16_SOFT;
        }
        else if(type == AL_UNSIGNED_BYTE3_SOFT || type == AL_BYTE3_SOFT ||
                type == AL_UNSIGNED_INT_SOFT || type == AL_INT_SOFT ||
                type == AL_FLOAT_SOFT || type == AL_DOUBLE_SOFT)
        {
            if(channels == AL_MONO_SOFT) format = AL_MONO32F_SOFT;
            else if(channels == AL_STEREO_SOFT) format = AL_STEREO32F_SOFT;
            else if(channels == AL_QUAD_SOFT) format = AL_QUAD32F_SOFT;
            else if(channels == AL_5POINT1_SOFT) format = AL_5POINT1_32F_SOFT;
            else if(channels == AL_6POINT1_SOFT) format = AL_6POINT1_32F_SOFT;
            else if(channels == AL_7POINT1_SOFT) format = AL_7POINT1_32F_SOFT;
        }

        if(format != AL_NONE && !palIsBufferFormatSupportedSOFT(format))
            format = AL_NONE;

        /* A matching format was not found or supported. Try 32-bit float. */
        if(format == AL_NONE)
        {
            if(channels == AL_MONO_SOFT) format = AL_MONO32F_SOFT;
            else if(channels == AL_STEREO_SOFT) format = AL_STEREO32F_SOFT;
            else if(channels == AL_QUAD_SOFT) format = AL_QUAD32F_SOFT;
            else if(channels == AL_5POINT1_SOFT) format = AL_5POINT1_32F_SOFT;
            else if(channels == AL_6POINT1_SOFT) format = AL_6POINT1_32F_SOFT;
            else if(channels == AL_7POINT1_SOFT) format = AL_7POINT1_32F_SOFT;

            if(format != AL_NONE && !palIsBufferFormatSupportedSOFT(format))
                format = AL_NONE;
        }
        /* 32-bit float not supported. Try 16-bit int. */
        if(format == AL_NONE)
        {
            if(channels == AL_MONO_SOFT) format = AL_MONO16_SOFT;
            else if(channels == AL_STEREO_SOFT) format = AL_STEREO16_SOFT;
            else if(channels == AL_QUAD_SOFT) format = AL_QUAD16_SOFT;
            else if(channels == AL_5POINT1_SOFT) format = AL_5POINT1_16_SOFT;
            else if(channels == AL_6POINT1_SOFT) format = AL_6POINT1_16_SOFT;
            else if(channels == AL_7POINT1_SOFT) format = AL_7POINT1_16_SOFT;

            if(format != AL_NONE && !palIsBufferFormatSupportedSOFT(format))
                format = AL_NONE;
        }
        /* 16-bit int not supported. Try 8-bit int. */
        if(format == AL_NONE)
        {
            if(channels == AL_MONO_SOFT) format = AL_MONO8_SOFT;
            else if(channels == AL_STEREO_SOFT) format = AL_STEREO8_SOFT;
            else if(channels == AL_QUAD_SOFT) format = AL_QUAD8_SOFT;
            else if(channels == AL_5POINT1_SOFT) format = AL_5POINT1_8_SOFT;
            else if(channels == AL_6POINT1_SOFT) format = AL_6POINT1_8_SOFT;
            else if(channels == AL_7POINT1_SOFT) format = AL_7POINT1_8_SOFT;

            if(format != AL_NONE && !palIsBufferFormatSupportedSOFT(format))
                format = AL_NONE;
        }

        return format;
    }

    /* We use the AL_EXT_MCFORMATS extension to provide output of Quad, 5.1,
     * and 7.1 channel configs, AL_EXT_FLOAT32 for 32-bit float samples, and
     * AL_EXT_DOUBLE for 64-bit float samples. */
    if(type == AL_UNSIGNED_BYTE_SOFT)
    {
        if(channels == AL_MONO_SOFT)
            format = AL_FORMAT_MONO8;
        else if(channels == AL_STEREO_SOFT)
            format = AL_FORMAT_STEREO8;
        else if(alIsExtensionPresent("AL_EXT_MCFORMATS"))
        {
            if(channels == AL_QUAD_SOFT)
                format = alGetEnumValue("AL_FORMAT_QUAD8");
            else if(channels == AL_5POINT1_SOFT)
                format = alGetEnumValue("AL_FORMAT_51CHN8");
            else if(channels == AL_6POINT1_SOFT)
                format = alGetEnumValue("AL_FORMAT_61CHN8");
            else if(channels == AL_7POINT1_SOFT)
                format = alGetEnumValue("AL_FORMAT_71CHN8");
        }
    }
    else if(type == AL_SHORT_SOFT)
    {
        if(channels == AL_MONO_SOFT)
            format = AL_FORMAT_MONO16;
        else if(channels == AL_STEREO_SOFT)
            format = AL_FORMAT_STEREO16;
        else if(alIsExtensionPresent("AL_EXT_MCFORMATS"))
        {
            if(channels == AL_QUAD_SOFT)
                format = alGetEnumValue("AL_FORMAT_QUAD16");
            else if(channels == AL_5POINT1_SOFT)
                format = alGetEnumValue("AL_FORMAT_51CHN16");
            else if(channels == AL_6POINT1_SOFT)
                format = alGetEnumValue("AL_FORMAT_61CHN16");
            else if(channels == AL_7POINT1_SOFT)
                format = alGetEnumValue("AL_FORMAT_71CHN16");
        }
    }
    else if(type == AL_FLOAT_SOFT && alIsExtensionPresent("AL_EXT_FLOAT32"))
    {
        if(channels == AL_MONO_SOFT)
            format = alGetEnumValue("AL_FORMAT_MONO_FLOAT32");
        else if(channels == AL_STEREO_SOFT)
            format = alGetEnumValue("AL_FORMAT_STEREO_FLOAT32");
        else if(alIsExtensionPresent("AL_EXT_MCFORMATS"))
        {
            if(channels == AL_QUAD_SOFT)
                format = alGetEnumValue("AL_FORMAT_QUAD32");
            else if(channels == AL_5POINT1_SOFT)
                format = alGetEnumValue("AL_FORMAT_51CHN32");
            else if(channels == AL_6POINT1_SOFT)
                format = alGetEnumValue("AL_FORMAT_61CHN32");
            else if(channels == AL_7POINT1_SOFT)
                format = alGetEnumValue("AL_FORMAT_71CHN32");
        }
    }
    else if(type == AL_DOUBLE_SOFT && alIsExtensionPresent("AL_EXT_DOUBLE"))
    {
        if(channels == AL_MONO_SOFT)
            format = alGetEnumValue("AL_FORMAT_MONO_DOUBLE");
        else if(channels == AL_STEREO_SOFT)
            format = alGetEnumValue("AL_FORMAT_STEREO_DOUBLE");
    }

    /* NOTE: It seems OSX returns -1 from alGetEnumValue for unknown enums, as
     * opposed to 0. Correct it. */
    if(format == -1)
        format = 0;

    return format;
}


void AL_APIENTRY wrap_BufferSamples(ALuint buffer, ALuint samplerate,
                                    ALenum internalformat, ALsizei samples,
                                    ALenum channels, ALenum type,
                                    const ALvoid *data)
{
    alBufferData(buffer, internalformat, data,
                 FramesToBytes(samples, channels, type),
                 samplerate);
}


const char *ChannelsName(ALenum chans)
{
    switch(chans)
    {
    case AL_MONO_SOFT: return "Mono";
    case AL_STEREO_SOFT: return "Stereo";
    case AL_REAR_SOFT: return "Rear";
    case AL_QUAD_SOFT: return "Quadraphonic";
    case AL_5POINT1_SOFT: return "5.1 Surround";
    case AL_6POINT1_SOFT: return "6.1 Surround";
    case AL_7POINT1_SOFT: return "7.1 Surround";
    }
    return "Unknown Channels";
}

const char *TypeName(ALenum type)
{
    switch(type)
    {
    case AL_BYTE_SOFT: return "S8";
    case AL_UNSIGNED_BYTE_SOFT: return "U8";
    case AL_SHORT_SOFT: return "S16";
    case AL_UNSIGNED_SHORT_SOFT: return "U16";
    case AL_INT_SOFT: return "S32";
    case AL_UNSIGNED_INT_SOFT: return "U32";
    case AL_FLOAT_SOFT: return "Float32";
    case AL_DOUBLE_SOFT: return "Float64";
    }
    return "Unknown Type";
}


ALsizei FramesToBytes(ALsizei size, ALenum channels, ALenum type)
{
    switch(channels)
    {
    case AL_MONO_SOFT:    size *= 1; break;
    case AL_STEREO_SOFT:  size *= 2; break;
    case AL_REAR_SOFT:    size *= 2; break;
    case AL_QUAD_SOFT:    size *= 4; break;
    case AL_5POINT1_SOFT: size *= 6; break;
    case AL_6POINT1_SOFT: size *= 7; break;
    case AL_7POINT1_SOFT: size *= 8; break;
    }

    switch(type)
    {
    case AL_BYTE_SOFT:           size *= sizeof(ALbyte); break;
    case AL_UNSIGNED_BYTE_SOFT:  size *= sizeof(ALubyte); break;
    case AL_SHORT_SOFT:          size *= sizeof(ALshort); break;
    case AL_UNSIGNED_SHORT_SOFT: size *= sizeof(ALushort); break;
    case AL_INT_SOFT:            size *= sizeof(ALint); break;
    case AL_UNSIGNED_INT_SOFT:   size *= sizeof(ALuint); break;
    case AL_FLOAT_SOFT:          size *= sizeof(ALfloat); break;
    case AL_DOUBLE_SOFT:         size *= sizeof(ALdouble); break;
    }

    return size;
}

ALsizei BytesToFrames(ALsizei size, ALenum channels, ALenum type)
{
    return size / FramesToBytes(1, channels, type);
}


#ifndef M_PI
#define M_PI    (3.14159265358979323846)
#endif

enum WaveType {
    WT_Sine,
    WT_Square,
    WT_Sawtooth,
    WT_Triangle,
    WT_Impulse,
};

static const char *GetWaveTypeName(enum WaveType type)
{
    switch(type)
    {
        case WT_Sine: return "sine";
        case WT_Square: return "square";
        case WT_Sawtooth: return "sawtooth";
        case WT_Triangle: return "triangle";
        case WT_Impulse: return "impulse";
    }
    return "(unknown)";
}

static void ApplySin(ALfloat *data, ALdouble g, ALuint srate, ALuint freq)
{
    ALdouble smps_per_cycle = (ALdouble)srate / freq;
    ALuint i;
    for(i = 0;i < srate;i++)
        data[i] += (ALfloat)(sin(i/smps_per_cycle * 2.0*M_PI) * g);
}

/* Generates waveforms using additive synthesis. Each waveform is constructed
 * by summing one or more sine waves, up to (and excluding) nyquist.
 */
static ALuint CreateWave(enum WaveType type, ALuint freq, ALuint srate)
{
    ALint data_size;
    ALfloat *data;
    ALuint buffer;
    ALenum err;
    ALuint i;

    data_size = srate * sizeof(ALfloat);
    data = (ALfloat*)calloc(1, data_size);
    if(type == WT_Sine)
        ApplySin(data, 1.0, srate, freq);
    else if(type == WT_Square)
        for(i = 1;freq*i < srate/2;i+=2)
            ApplySin(data, 4.0/M_PI * 1.0/i, srate, freq*i);
    else if(type == WT_Sawtooth)
        for(i = 1;freq*i < srate/2;i++)
            ApplySin(data, 2.0/M_PI * ((i&1)*2 - 1.0) / i, srate, freq*i);
    else if(type == WT_Triangle)
        for(i = 1;freq*i < srate/2;i+=2)
            ApplySin(data, 8.0/(M_PI*M_PI) * (1.0 - (i&2)) / (i*i), srate, freq*i);
    else if(type == WT_Impulse)
    {
        /* NOTE: Impulse isn't really a waveform, but it can still be useful to
         * test (other than resampling, the ALSOFT_DEFAULT_REVERB environment
         * variable can prove useful here to test the reverb response).
         */
        for(i = 0;i < srate;i++)
            data[i] = (i%(srate/freq)) ? 0.0f : 1.0f;
    }

    /* Buffer the audio data into a new buffer object. */
    buffer = 0;
    alGenBuffers(1, &buffer);
    alBufferData(buffer, AL_FORMAT_MONO_FLOAT32, data, data_size, srate);
    free(data);

    /* Check if an error occured, and clean up if so. */
    err = alGetError();
    if(err != AL_NO_ERROR)
    {
        fprintf(stderr, "OpenAL Error: %s\n", alGetString(err));
        if(alIsBuffer(buffer))
            alDeleteBuffers(1, &buffer);
        return 0;
    }

    return buffer;
}


int main(int argc, char *argv[])
{
    enum WaveType wavetype = WT_Sine;
    const char *appname = argv[0];
    ALuint source, buffer;
    ALint last_pos, num_loops;
    ALint max_loops = 4;
    ALint srate = -1;
    ALint tone_freq = 1000;
    ALCint dev_rate;
    ALenum state;
    int i;

    argv++; argc--;
    if(InitAL(&argv, &argc) != 0)
        return 1;

    if(!alIsExtensionPresent("AL_EXT_FLOAT32"))
    {
        fprintf(stderr, "Required AL_EXT_FLOAT32 extension not supported on this device!\n");
        CloseAL();
        return 1;
    }

    for(i = 0;i < argc;i++)
    {
        if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            fprintf(stderr, "OpenAL Tone Generator\n"
"\n"
"Usage: %s [-device <name>] <options>\n"
"\n"
"Available options:\n"
"  --help/-h                 This help text\n"
"  -t <seconds>              Time to play a tone (default 5 seconds)\n"
"  --waveform/-w <type>      Waveform type: sine (default), square, sawtooth,\n"
"                                triangle, impulse\n"
"  --freq/-f <hz>            Tone frequency (default 1000 hz)\n"
"  --srate/-s <sample rate>  Sampling rate (default output rate)\n",
                appname
            );
            CloseAL();
            return 1;
        }
        else if(i+1 < argc && strcmp(argv[i], "-t") == 0)
        {
            i++;
            max_loops = atoi(argv[i]) - 1;
        }
        else if(i+1 < argc && (strcmp(argv[i], "--waveform") == 0 || strcmp(argv[i], "-w") == 0))
        {
            i++;
            if(strcmp(argv[i], "sine") == 0)
                wavetype = WT_Sine;
            else if(strcmp(argv[i], "square") == 0)
                wavetype = WT_Square;
            else if(strcmp(argv[i], "sawtooth") == 0)
                wavetype = WT_Sawtooth;
            else if(strcmp(argv[i], "triangle") == 0)
                wavetype = WT_Triangle;
            else if(strcmp(argv[i], "impulse") == 0)
                wavetype = WT_Impulse;
            else
                fprintf(stderr, "Unhandled waveform: %s\n", argv[i]);
        }
        else if(i+1 < argc && (strcmp(argv[i], "--freq") == 0 || strcmp(argv[i], "-f") == 0))
        {
            i++;
            tone_freq = atoi(argv[i]);
            if(tone_freq < 1)
            {
                fprintf(stderr, "Invalid tone frequency: %s (min: 1hz)\n", argv[i]);
                tone_freq = 1;
            }
        }
        else if(i+1 < argc && (strcmp(argv[i], "--srate") == 0 || strcmp(argv[i], "-s") == 0))
        {
            i++;
            srate = atoi(argv[i]);
            if(srate < 40)
            {
                fprintf(stderr, "Invalid sample rate: %s (min: 40hz)\n", argv[i]);
                srate = 40;
            }
        }
    }

    {
        ALCdevice *device = alcGetContextsDevice(alcGetCurrentContext());
        alcGetIntegerv(device, ALC_FREQUENCY, 1, &dev_rate);
        assert(alcGetError(device)==ALC_NO_ERROR && "Failed to get device sample rate");
    }
    if(srate < 0)
        srate = dev_rate;

    /* Load the sound into a buffer. */
    buffer = CreateWave(wavetype, tone_freq, srate);
    if(!buffer)
    {
        CloseAL();
        return 1;
    }

    printf("Playing %dhz %s-wave tone with %dhz sample rate and %dhz output, for %d second%s...\n",
           tone_freq, GetWaveTypeName(wavetype), srate, dev_rate, max_loops+1, max_loops?"s":"");
    fflush(stdout);

    /* Create the source to play the sound with. */
    source = 0;
    alGenSources(1, &source);
    alSourcei(source, AL_BUFFER, buffer);
    assert(alGetError()==AL_NO_ERROR && "Failed to setup sound source");

    /* Play the sound for a while. */
    num_loops = 0;
    last_pos = 0;
    alSourcei(source, AL_LOOPING, (max_loops > 0) ? AL_TRUE : AL_FALSE);
    alSourcePlay(source);
    do {
        ALint pos;
        //al_nssleep(10000000);
        alGetSourcei(source, AL_SAMPLE_OFFSET, &pos);
        alGetSourcei(source, AL_SOURCE_STATE, &state);
        if(pos < last_pos && state == AL_PLAYING)
        {
            ++num_loops;
            if(num_loops >= max_loops)
                alSourcei(source, AL_LOOPING, AL_FALSE);
            printf("%d...\n", max_loops - num_loops + 1);
            fflush(stdout);
        }
        last_pos = pos;
    } while(alGetError() == AL_NO_ERROR && state == AL_PLAYING);

    /* All done. Delete resources, and close OpenAL. */
    alDeleteSources(1, &source);
    alDeleteBuffers(1, &buffer);

    /* Close up OpenAL. */
    CloseAL();

    return 0;
}