#include <stdexcept>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "SDLGL.h"
#include "App.h"

SDLResVidModes sdlResVideoModes[18] = {
	{480, 320},
	{640, 480},
	{720, 400},
	{720, 480},
	{720, 576},
	{800, 600},
	{832, 624},
	{960, 640},
	{1024, 768},
	{1152, 864},
	{1152, 872},
	{1280, 720},
	{1280, 800},
	{1280, 1024},
	{1440, 900},
	{1600, 1000},
	{1680, 1050},
	{1920, 1080}
};

#ifdef N64

SDLGL::SDLGL() {
	this->initialized = false;
	this->window = nullptr;
	this->glcontext = nullptr;
	this->resolutionIndex = 0;
	this->oldResolutionIndex = -1;
	this->winVidWidth = 320;
	this->winVidHeight = 240;
	this->vidWidth = Applet::IOS_WIDTH;
	this->vidHeight = Applet::IOS_HEIGHT;
	this->windowMode = 0;
	this->oldWindowMode = -1;
	this->vSync = true;
	this->oldVSync = false;
}

SDLGL::~SDLGL() {
}

bool SDLGL::Initialize() {
	this->initialized = true;
	return true;
}

void SDLGL::Error(const char* fmt, ...) {
	char errMsg[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(errMsg, sizeof(errMsg), fmt, ap);
	va_end(ap);
	printf("FATAL ERROR: %s\n", errMsg);
	while (1) {}
}

void SDLGL::transformCoord2f(float* x, float* y) {
	*x *= (float)winVidWidth / (float)Applet::IOS_WIDTH;
	*y *= (float)winVidHeight / (float)Applet::IOS_HEIGHT;
}

void SDLGL::centerMouse(int x, int y) {
	(void)x; (void)y;
}

void SDLGL::updateWinVid(int w, int h) {
	this->winVidWidth = w;
	this->winVidHeight = h;
}

void SDLGL::updateVideo() {
}

void SDLGL::restore() {
}

#else // !N64 (PC / Desktop SDL2 implementation)

SDLGL::SDLGL() {
	initialized = false;
}

SDLGL::~SDLGL() {
	if (this) {
		if (initialized) {
			if (this->window) {
				SDL_SetWindowFullscreen(this->window, 0);
			}
			SDL_GL_DeleteContext(glcontext);
			SDL_DestroyWindow(window);
			SDL_Quit();
		}
	}
}

bool SDLGL::Initialize() {
	Uint32 flags;

	if (!this->initialized) {
		
		SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
		if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
			printf("Could not initialize SDL: %s", SDL_GetError());
		}

		flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN /* | SDL_WINDOW_RESIZABLE*/;
		flags |= SDL_WINDOW_ALLOW_HIGHDPI;

		this->oldResolutionIndex = -1;
		this->resolutionIndex = 0;
		this->winVidWidth = sdlResVideoModes[this->resolutionIndex].width;
		this->winVidHeight = sdlResVideoModes[this->resolutionIndex].height;

		this->window = SDL_CreateWindow("Doom II RPG By [GEC] Version 0.1", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, winVidWidth, winVidHeight, flags);
		if (!this->window) {
			printf("Could not set %dx%d video mode: %s", winVidWidth, winVidHeight, SDL_GetError());
		}

		this->vidWidth = Applet::IOS_WIDTH;
		this->vidHeight = Applet::IOS_HEIGHT;
		this->oldWindowMode = -1;
		this->windowMode = 0;
		this->oldVSync = false;
		this->vSync = true;

		this->updateVideo();

		this->glcontext = SDL_GL_CreateContext(window);

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		SDL_GL_SwapWindow(this->window);

		SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "0");
		this->initialized = true;
	}

	return this->initialized;
}

void SDLGL::Error(const char* fmt, ...)
{
	char errMsg[256];
	va_list ap;
	va_start(ap, fmt);
#ifdef _WIN32
	vsprintf_s(errMsg, sizeof(errMsg), fmt, ap);
#else
	vsnprintf(errMsg, sizeof(errMsg), fmt, ap);
#endif
	va_end(ap);

	printf("%s", errMsg);

	const SDL_MessageBoxButtonData buttons[] = {
		{ 0, 0, "Ok" },
	};
	const SDL_MessageBoxColorScheme colorScheme = {
		{
			{ 255,   0,   0 },
			{   0, 255,   0 },
			{ 255, 255,   0 },
			{   0,   0, 255 },
			{ 255,   0, 255 }
		}
	};
	const SDL_MessageBoxData messageboxdata = {
		SDL_MESSAGEBOX_ERROR,
		NULL,
		"Doom II RPG Error",
		errMsg,
		SDL_arraysize(buttons),
		buttons,
		&colorScheme
	};

	SDL_ShowMessageBox(&messageboxdata, NULL);
	SDLGL::~SDLGL();
	exit(0);
}

void SDLGL::transformCoord2f(float* x, float* y) {
	int w, h;
	SDL_GetWindowSize(this->window, &w, &h);
	*x *= (float)w / (float)Applet::IOS_WIDTH;
	*y *= (float)h / (float)Applet::IOS_HEIGHT;
}

void SDLGL::centerMouse(int x, int y) {
	float X = (float)((Applet::IOS_WIDTH / 2) + x);
	float Y = (float)((Applet::IOS_HEIGHT / 2) + y);
	this->transformCoord2f(&X, &Y);
	SDL_WarpMouseInWindow(this->window, (int)X, (int)Y);
}

void SDLGL::updateWinVid(int w, int h) {
	this->winVidWidth = w;
	this->winVidHeight = h;
}

void SDLGL::updateVideo() {
	if (this->vSync != this->oldVSync) {
		SDL_GL_SetSwapInterval(this->vSync ? SDL_TRUE : SDL_FALSE);
		this->oldVSync = this->vSync;
	}

	if (this->windowMode != this->oldWindowMode) {
		SDL_SetWindowFullscreen(this->window, 0);
		SDL_SetWindowBordered(this->window, SDL_TRUE);

		if (this->windowMode == 1) {
			SDL_SetWindowBordered(this->window, SDL_FALSE);
		}
		else if (this->windowMode == 2) {
			SDL_SetWindowFullscreen(this->window, SDL_WINDOW_FULLSCREEN);
		}
		this->oldWindowMode = this->windowMode;
	}

	if (this->resolutionIndex != this->oldResolutionIndex) {
		SDL_SetWindowSize(this->window, sdlResVideoModes[this->resolutionIndex].width, sdlResVideoModes[this->resolutionIndex].height);
		SDL_SetWindowPosition(this->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		this->updateWinVid(sdlResVideoModes[this->resolutionIndex].width, sdlResVideoModes[this->resolutionIndex].height);
		this->oldResolutionIndex = this->resolutionIndex;
	}

	int winVidWidth, winVidHeight;
	SDL_GetWindowSize(this->window, &winVidWidth, &winVidHeight);
	this->updateWinVid(winVidWidth, winVidHeight);
}

void SDLGL::restore() {
	if (this->vSync != this->oldVSync) {
		this->vSync = this->oldVSync;
	}

	if (this->windowMode != this->oldWindowMode) {
		this->windowMode = this->oldWindowMode;
	}

	if (this->resolutionIndex != this->oldResolutionIndex) {
		this->resolutionIndex = this->oldResolutionIndex;
	}
}

#endif
