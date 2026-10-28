#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>
#include "SDLGL.h"
#include "ZipFile.h"

#include "CAppContainer.h"
#include "App.h"
#include "Image.h"
#include "Resource.h"
#include "Render.h"
#include "GLES.h"

#include "Canvas.h"
#include "Graphics.h"
#include "Player.h"
#include "Game.h"
#include "Graphics.h"
#include "Utils.h"
#include "TinyGL.h"
#include "Input.h"

#ifdef N64
#include <libdragon.h>
#include <new>
#include <malloc.h>
extern "C" void __debug_init_cpp(void) {}

void* operator new(size_t size) {
    void* p = malloc(size);
    if (!p) {
        struct mallinfo minf = mallinfo();
        debugf("!!! [BAD_ALLOC] operator new(%lu) failed! heap free=%d used=%d !!!\n",
               (unsigned long)size, minf.fordblks, minf.uordblks);
        printf("!!! [BAD_ALLOC] operator new(%lu) failed! heap free=%d used=%d !!!\n",
               (unsigned long)size, minf.fordblks, minf.uordblks);
        abort();
    }
    return p;
}

void* operator new[](size_t size) {
    void* p = malloc(size);
    if (!p) {
        struct mallinfo minf = mallinfo();
        debugf("!!! [BAD_ALLOC] operator new[](%lu) failed! heap free=%d used=%d !!!\n",
               (unsigned long)size, minf.fordblks, minf.uordblks);
        printf("!!! [BAD_ALLOC] operator new[](%lu) failed! heap free=%d used=%d !!!\n",
               (unsigned long)size, minf.fordblks, minf.uordblks);
        abort();
    }
    return p;
}

void operator delete(void* p) noexcept {
    if (p) free(p);
}

void operator delete[](void* p) noexcept {
    if (p) free(p);
}

void operator delete(void* p, size_t) noexcept {
    if (p) free(p);
}

void operator delete[](void* p, size_t) noexcept {
    if (p) free(p);
}
#endif

void drawView(SDLGL* sdlGL);

int main(int argc, char* args[]) {

#ifdef N64
    debug_init_isviewer();
    debug_init_usblog();
    debugf("Doom II RPG 64 booting (RDRAM=%d KB)...\n", get_memory_size() / 1024);

    // Expansion Pak (8 MB) check
    if (get_memory_size() < 8 * 1024 * 1024) {
        printf("ERROR: Expansion Pak (8 MB) required!\n");
        display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE);
        console_init();
        printf("\n\n\n   Expansion Pak (8 MB) required!\n");
        printf("   Please install Expansion Pak\n");
        printf("   to play Doom II RPG 64.\n");
        while (1) {}
    }

    dfs_init(DFS_DEFAULT_LOCATION);
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE);
    rdpq_init();
    gl_init();
    joypad_init();
#endif

    int		UpTime = 0;
    
    ZipFile zipFile;
    zipFile.openZipFile("Doom 2 RPG.ipa");

	SDLGL sdlGL;
	sdlGL.Initialize();

    Input input;
    input.init(); // [GEC] Port: set default Binds

    CAppContainer::getInstance()->Construct(&sdlGL, &zipFile);
    sdlGL.updateVideo(); // [GEC]

    SDL_Event ev;
    

    float x = 0.0f, y = 30.0f;
    int   vp_cx = 480;
    int   vp_cy = 320;


    Uint8 state;
    int mX, mY;           /* mouse location*/
    float mousePressX = 0.f, mousePressY = 0.f;           /* mouse location float*/
    int winVidWidth = sdlGL.winVidWidth;
    int winVidHeight = sdlGL.winVidHeight;
    bool useMouse = false;
   
    

    while (CAppContainer::getInstance()->app->closeApplet != true) {
#ifdef N64
        mixer_try_play();
#endif
        int currentTimeMillis = CAppContainer::getInstance()->getTimeMS();

        //if (!useMouse) {
            //float ax = SDL_GameControllerGetAxis(sdlGL.accelerometer, SDL_CONTROLLER_AXIS_LEFTX);
            //float ay = SDL_GameControllerGetAxis(sdlGL.accelerometer, SDL_CONTROLLER_AXIS_LEFTY);
            //CAppContainer::getInstance()->UpdateAccelerometer((ax* (1.0f / 32767))/2, (ay* (1.0f / 32767))/2, 0.f, false);
        //}

        if (currentTimeMillis > UpTime) {
            input.handleEvents();
            UpTime = currentTimeMillis + 15;
            drawView(&sdlGL);
            input.consumeEvents();
        }
    }

    printf("APP_QUIT\n");
    CAppContainer::getInstance()->~CAppContainer();
    zipFile.closeZipFile();
    sdlGL.~SDLGL();
    input.~Input();
	return 0;
}


static uint32_t lastTimems = 0;

void drawView(SDLGL *sdlGL) {

    if (lastTimems == 0) {
        lastTimems = CAppContainer::getInstance()->getTimeMS();
    }

#ifdef N64
    surface_t *disp = display_get();
    rdpq_attach_clear(disp, NULL);
    gl_context_begin();

    // Letterbox: 320x213 in 320x240 display (27px vertical margin / 2 = 13px top, 14px bottom)
    glViewport(0, 13, 320, 213);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    uint32_t startTime = CAppContainer::getInstance()->getTimeMS();
    uint32_t passedTime = startTime - lastTimems;
    lastTimems = startTime;

    if (passedTime >= 125) {
        passedTime = 125;
    }

    CAppContainer::getInstance()->DoLoop(passedTime);

#ifdef N64
    static bool s_loggedFirstFrame = false;
    if (!s_loggedFirstFrame) {
        s_loggedFirstFrame = true;
        heap_stats_t hs;
        sys_get_heap_stats(&hs);
        debugf("[FRAME 1] state=%d  heap_used=%d KB  heap_free=%d KB\n",
               CAppContainer::getInstance()->app->canvas->state,
               hs.used / 1024, (hs.total - hs.used) / 1024);
    }

    static int s_perfFrames = 0;
    static uint32_t s_perfLastTime = 0;
    s_perfFrames++;
    uint32_t nowTime = CAppContainer::getInstance()->getTimeMS();
    if (nowTime - s_perfLastTime >= 1000) {
        int fps = (s_perfFrames * 1000) / (nowTime - s_perfLastTime);
        struct mallinfo minf = mallinfo();
        debugf("[PERF] FPS=%d state=%d heap_free=%d KB\n",
               fps, CAppContainer::getInstance()->app->canvas->state,
               minf.fordblks / 1024);
        s_perfFrames = 0;
        s_perfLastTime = nowTime;
    }

    mixer_try_play();
#endif

    gl_context_end();
    rdpq_detach_show();

#else // !N64

    int cx, cy;
    int w = sdlGL->vidWidth;
    int h = sdlGL->vidHeight;

    SDL_GetWindowSize(sdlGL->window, &cx, &cy);
    if (w != cx || h != cy) {
        w = cx; h = cy;
    }

    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_ALPHA_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    
    uint32_t startTime = CAppContainer::getInstance()->getTimeMS();
    uint32_t passedTime = startTime - lastTimems;
    lastTimems = startTime;

    if (passedTime >= 125) {
        passedTime = 125;
    }

    CAppContainer::getInstance()->DoLoop(passedTime);

    SDL_GL_SwapWindow(sdlGL->window);  // Swap the window/pBmp to display the result.
#endif
}