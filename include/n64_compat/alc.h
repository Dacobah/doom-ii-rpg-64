#ifndef __N64_ALC_COMPAT_H__
#define __N64_ALC_COMPAT_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef void ALCdevice;
typedef void ALCcontext;

#define ALC_FALSE 0
#define ALC_TRUE 1

static inline ALCdevice* alcOpenDevice(const char* devicename) { (void)devicename; return (ALCdevice*)1; }
static inline int alcCloseDevice(ALCdevice* device) { (void)device; return ALC_TRUE; }
static inline ALCcontext* alcCreateContext(ALCdevice* device, const int* attrlist) { (void)device; (void)attrlist; return (ALCcontext*)1; }
static inline ALCcontext* alcGetCurrentContext(void) { return (ALCcontext*)1; }
static inline int alcMakeContextCurrent(ALCcontext* context) { (void)context; return ALC_TRUE; }
static inline void alcSuspendContext(ALCcontext* context) { (void)context; }
static inline void alcProcessContext(ALCcontext* context) { (void)context; }
static inline void alcDestroyContext(ALCcontext* context) { (void)context; }

#ifdef __cplusplus
}
#endif

#endif // __N64_ALC_COMPAT_H__
