#ifndef __N64_AL_COMPAT_H__
#define __N64_AL_COMPAT_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t ALuint;
typedef int32_t  ALint;
typedef int32_t  ALenum;
typedef uint8_t  ALboolean;
typedef size_t   ALsizei;
typedef float    ALfloat;
typedef char     ALchar;
typedef void     ALvoid;

#define AL_NONE 0
#define AL_FALSE 0
#define AL_TRUE 1
#define AL_NO_ERROR 0
#define AL_POSITION 0x1004
#define AL_VELOCITY 0x1006
#define AL_GAIN 0x100A
#define AL_MAX_GAIN 0x100D
#define AL_BUFFER 0x1009
#define AL_LOOPING 0x1007
#define AL_SOURCE_STATE 0x1010
#define AL_PLAYING 0x1012
#define AL_PAUSED 0x1013
#define AL_REFERENCE_DISTANCE 0x1020

#define AL_FORMAT_MONO8 0x1100
#define AL_FORMAT_MONO16 0x1101
#define AL_FORMAT_STEREO8 0x1102
#define AL_FORMAT_STEREO16 0x1103

static inline ALenum alGetError(void) { return AL_NO_ERROR; }
static inline const ALchar* alGetString(ALenum param) { (void)param; return "OK"; }
static inline void alGenSources(ALsizei n, ALuint* sources) { for (ALsizei i=0; i<n; i++) sources[i] = i+1; }
static inline void alDeleteSources(ALsizei n, const ALuint* sources) { (void)n; (void)sources; }
static inline void alGenBuffers(ALsizei n, ALuint* buffers) { for (ALsizei i=0; i<n; i++) buffers[i] = i+1; }
static inline void alDeleteBuffers(ALsizei n, const ALuint* buffers) { (void)n; (void)buffers; }
static inline void alSourcei(ALuint source, ALenum param, ALint value) { (void)source; (void)param; (void)value; }
static inline void alSourcef(ALuint source, ALenum param, ALfloat value) { (void)source; (void)param; (void)value; }
static inline void alSource3i(ALuint source, ALenum param, ALint x, ALint y, ALint z) { (void)source; (void)param; (void)x; (void)y; (void)z; }
static inline void alGetSourcei(ALuint source, ALenum param, ALint* value) { (void)source; (void)param; if (value) *value = 0; }
static inline void alListenerf(ALenum param, ALfloat value) { (void)param; (void)value; }
static inline void alListener3i(ALenum param, ALint x, ALint y, ALint z) { (void)param; (void)x; (void)y; (void)z; }
static inline void alBufferData(ALuint buffer, ALenum format, const void* data, ALsizei size, ALsizei freq) {
    (void)buffer; (void)format; (void)data; (void)size; (void)freq;
}
static inline void alSourcePlay(ALuint source) { (void)source; }
static inline void alSourceStop(ALuint source) { (void)source; }
static inline void alSourcePause(ALuint source) { (void)source; }

#ifdef __cplusplus
}
#endif

#endif // __N64_AL_COMPAT_H__
