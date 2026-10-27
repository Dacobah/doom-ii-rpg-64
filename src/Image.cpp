#include <stdexcept>

#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Image.h"
#include "IDIB.h"
#include "SDLGL.h"
#ifdef N64
#include <libdragon.h>
#endif

#ifdef N64
static Image* s_lruHead = nullptr;
static Image* s_lruTail = nullptr;
static uint32_t s_total2DGlBytes = 0;
static constexpr uint32_t MAX_2D_GL_BYTES = 256 * 1024; // 256 KB 2D GL LRU cache
static uint16_t s_tileBuf[2048]; // 4 KB TMEM-sized conversion scratch buffer

static void UnlinkImageLRU(Image* img) {
    if (!img) return;
    if (img->lruPrev) {
        img->lruPrev->lruNext = img->lruNext;
    } else if (s_lruHead == img) {
        s_lruHead = img->lruNext;
    }
    if (img->lruNext) {
        img->lruNext->lruPrev = img->lruPrev;
    } else if (s_lruTail == img) {
        s_lruTail = img->lruPrev;
    }
    img->lruPrev = nullptr;
    img->lruNext = nullptr;
}

static void LinkHeadImageLRU(Image* img) {
    if (!img) return;
    UnlinkImageLRU(img);
    img->lruPrev = nullptr;
    img->lruNext = s_lruHead;
    if (s_lruHead) {
        s_lruHead->lruPrev = img;
    }
    s_lruHead = img;
    if (!s_lruTail) {
        s_lruTail = img;
    }
}

static void ReleaseImageGLHelper(Image* img) {
    if (!img) return;
    UnlinkImageLRU(img);
    if (img->tileTextures != nullptr) {
        int count = (int)img->numTilesX * (int)img->numTilesY;
        for (int i = 0; i < count; i++) {
            if (img->tileTextures[i] != 0 && img->tileTextures[i] != (GLuint)-1) {
                glDeleteTextures(1, &img->tileTextures[i]);
            }
        }
        std::free(img->tileTextures);
        img->tileTextures = nullptr;
    } else if (img->texture != (GLuint)-1 && img->texture != 0) {
        glDeleteTextures(1, &img->texture);
    }
    if (s_total2DGlBytes >= img->glBytes) {
        s_total2DGlBytes -= img->glBytes;
    } else {
        s_total2DGlBytes = 0;
    }
    img->glBytes = 0;
    img->numTilesX = 0;
    img->numTilesY = 0;
    img->texture = (GLuint)-1;
}

static void DisposeImageHelper(Image* img) {
    if (!img) return;
    if (img->piDIB) {
        img->piDIB->~IDIB();
        std::free(img->piDIB);
        img->piDIB = nullptr;
    }
    ReleaseImageGLHelper(img);
    std::free(img);
}
#endif

Image::Image() {
    this->piDIB = nullptr;
    this->width = 0;
    this->height = 0;
    this->depth = 0;
    this->isTransparentMask = false;
    this->texWidth = 0;
    this->texHeight = 0;
    this->texture = (GLuint)-1;
#ifdef N64
    this->tileTextures = nullptr;
    this->numTilesX = 0;
    this->numTilesY = 0;
    this->tileW = 0;
    this->tileH = 0;
    this->glBytes = 0;
    this->lruPrev = nullptr;
    this->lruNext = nullptr;
#endif
}

Image::~Image() {
#ifdef N64
    DisposeImageHelper(this);
#else
    if (this) {
        if (this->piDIB) {
            this->piDIB->~IDIB();
            std::free(this->piDIB);
            this->piDIB = nullptr;
        }
        if (this->texture != (GLuint)-1 && this->texture != 0) {
            glDeleteTextures(1, &this->texture);
        }
        this->texture = (GLuint)-1;
        std::free(this);
    }
#endif
}

#ifdef N64
void Image::ReleaseGLTextures() {
    ReleaseImageGLHelper(this);
}

void Image::TouchLRU() {
    if (this->texture != (GLuint)-1 && this->texture != 0) {
        LinkHeadImageLRU(this);
    }
}

bool Image::EnsureTexture() {
    if (this->texture != (GLuint)-1 && this->texture != 0) {
        this->TouchLRU();
        return true;
    }
    if (!rdpq_is_attached()) {
        return false;
    }
    if (!this->piDIB || !this->piDIB->pBmp || !this->piDIB->pRGB565) {
        return false;
    }

    const int imgW = this->piDIB->width;
    const int imgH = this->piDIB->height;
    if (imgW <= 0 || imgH <= 0) {
        return false;
    }

    this->width = imgW;
    this->height = imgH;

    // Minimum width/height of 4 guarantees tmem_pitch (width*2 >= 8) is a multiple of 8 bytes
    int powW = 4;
    while (powW < imgW) powW <<= 1;
    int powH = 4;
    while (powH < imgH) powH <<= 1;

    int tw = (powW <= 64) ? powW : 64;
    int maxTh = 2048 / tw; // Max 2048 RGBA16 texels = 4096 bytes (exact N64 TMEM size)
    int th = (powH <= maxTh) ? powH : maxTh;

    uint16_t ntx = (uint16_t)((imgW + tw - 1) / tw);
    uint16_t nty = (uint16_t)((imgH + th - 1) / th);
    uint32_t totalTiles = (uint32_t)ntx * (uint32_t)nty;
    uint32_t neededBytes = totalTiles * (uint32_t)(tw * th * 2);

    // Evict least-recently-used 2D textures if over 1.25 MB budget
    uint32_t evictedBytes = 0;
    while (s_total2DGlBytes + neededBytes > MAX_2D_GL_BYTES && s_lruTail != nullptr) {
        Image* victim = s_lruTail;
        if (victim == this) {
            victim = victim->lruPrev;
            if (victim == nullptr) break;
        }
        evictedBytes += victim->glBytes;
        ReleaseImageGLHelper(victim);
    }
    if (evictedBytes > 0) {
        rspq_wait();
    }

    const uint8_t* pBmp = this->piDIB->pBmp;
    const uint16_t* pPal = this->piDIB->pRGB565;
    this->isTransparentMask = false;
    const int totalPx = imgW * imgH;
    for (int i = 0; i < totalPx; i++) {
        if (pPal[pBmp[i]] == 0xf81f) {
            this->isTransparentMask = true;
            break;
        }
    }

    if (totalTiles > 1) {
        this->tileTextures = (GLuint*)std::malloc(totalTiles * sizeof(GLuint));
        if (!this->tileTextures) {
            debugf("Image::EnsureTexture: malloc failed for %lu tile IDs (%dx%d)\n",
                   (unsigned long)totalTiles, imgW, imgH);
            return false;
        }
        for (uint32_t i = 0; i < totalTiles; i++) {
            this->tileTextures[i] = 0;
        }
        glGenTextures((GLsizei)totalTiles, this->tileTextures);
    } else {
        this->tileTextures = nullptr;
        this->texture = 0;
        glGenTextures(1, &this->texture);
        if (this->texture == 0 || this->texture == (GLuint)-1) {
            debugf("Image::EnsureTexture: glGenTextures failed (%dx%d)\n", imgW, imgH);
            this->texture = (GLuint)-1;
            return false;
        }
    }

    glEnable(GL_TEXTURE_2D);

    for (uint16_t ty = 0; ty < nty; ty++) {
        for (uint16_t tx = 0; tx < ntx; tx++) {
            GLuint texId = (totalTiles > 1) ? this->tileTextures[ty * ntx + tx] : this->texture;
            if (texId == 0 || texId == (GLuint)-1) {
                ReleaseImageGLHelper(this);
                return false;
            }

            int baseX = (int)tx * tw;
            int baseY = (int)ty * th;
            for (int row = 0; row < th; row++) {
                int py = baseY + row;
                uint16_t* dstRow = &s_tileBuf[row * tw];
                if (py >= imgH) {
                    if (this->isTransparentMask) {
                        for (int col = 0; col < tw; col++) dstRow[col] = 0x0000;
                        continue;
                    }
                    py = imgH - 1;
                }
                const uint8_t* srcRow = &pBmp[py * imgW];
                for (int col = 0; col < tw; col++) {
                    int px = baseX + col;
                    if (px >= imgW) {
                        if (this->isTransparentMask) {
                            dstRow[col] = 0x0000;
                            continue;
                        }
                        px = imgW - 1;
                    }
                    uint16_t rgb = pPal[srcRow[px]];
                    if (this->isTransparentMask && rgb == 0xf81f) {
                        dstRow[col] = 0x0000;
                    } else {
                        dstRow[col] = (uint16_t)((rgb & 0xffc0) | ((rgb & 0x1f) << 1) | 1);
                    }
                }
            }

            glBindTexture(GL_TEXTURE_2D, texId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, s_tileBuf);
        }
    }

    this->tileW = (uint16_t)tw;
    this->tileH = (uint16_t)th;
    this->numTilesX = ntx;
    this->numTilesY = nty;
    this->texWidth = (int)ntx * tw;
    this->texHeight = (int)nty * th;
    if (totalTiles > 1) {
        this->texture = this->tileTextures[0];
    }
    this->glBytes = neededBytes;
    s_total2DGlBytes += neededBytes;
    LinkHeadImageLRU(this);
    return true;
}

void Image::DrawQuadTiled(int texX, int texY, int texW, int texH, float quadScale, bool portalMode) {
    if (texW <= 0 || texH <= 0 || this->tileW == 0 || this->tileH == 0) {
        return;
    }
    const int srcX0 = texX;
    const int srcY0 = texY;
    const int srcX1 = texX + texW;
    const int srcY1 = texY + texH;

    const int cx0 = (srcX0 < 0) ? 0 : srcX0;
    const int cy0 = (srcY0 < 0) ? 0 : srcY0;
    const int cx1 = (srcX1 > this->width) ? this->width : srcX1;
    const int cy1 = (srcY1 > this->height) ? this->height : srcY1;
    if (cx0 >= cx1 || cy0 >= cy1) {
        return;
    }

    const int tw = (int)this->tileW;
    const int th = (int)this->tileH;
    const int tx0 = cx0 / tw;
    const int tx1 = (cx1 - 1) / tw;
    const int ty0 = cy0 / th;
    const int ty1 = (cy1 - 1) / th;

    const float halfW = (float)texW * 0.5f * quadScale;
    const float halfH = (float)texH * 0.5f * quadScale;
    const float invTW = 1.0f / (float)tw;
    const float invTH = 1.0f / (float)th;

    float vp[12];
    float st[8];
    vp[2] = 0.5f;
    vp[5] = 0.5f;
    vp[8] = 0.5f;
    vp[11] = 0.5f;

    glVertexPointer(3, GL_FLOAT, 0, vp);
    glEnableClientState(GL_VERTEX_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, 0, st);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    for (int ty = ty0; ty <= ty1; ty++) {
        if (ty < 0 || ty >= (int)this->numTilesY) continue;
        const int tileBaseY = ty * th;
        const int sy0 = (cy0 > tileBaseY) ? cy0 : tileBaseY;
        const int sy1 = (cy1 < tileBaseY + th) ? cy1 : (tileBaseY + th);
        const float v0 = (float)(sy0 - tileBaseY) * invTH;
        const float v1 = (float)(sy1 - tileBaseY) * invTH;

        for (int tx = tx0; tx <= tx1; tx++) {
            if (tx < 0 || tx >= (int)this->numTilesX) continue;
            GLuint texId = (this->tileTextures != nullptr)
                ? this->tileTextures[ty * this->numTilesX + tx]
                : this->texture;
            if (texId == 0 || texId == (GLuint)-1) continue;

            const int tileBaseX = tx * tw;
            const int sx0 = (cx0 > tileBaseX) ? cx0 : tileBaseX;
            const int sx1 = (cx1 < tileBaseX + tw) ? cx1 : (tileBaseX + tw);
            const float u0 = (float)(sx0 - tileBaseX) * invTW;
            const float u1 = (float)(sx1 - tileBaseX) * invTW;

            if (!portalMode) {
                const float lx0 = (float)(sx0 - srcX0) * quadScale - halfW;
                const float lx1 = (float)(sx1 - srcX0) * quadScale - halfW;
                const float ly0 = (float)(sy0 - srcY0) * quadScale - halfH;
                const float ly1 = (float)(sy1 - srcY0) * quadScale - halfH;

                vp[0] = lx1; vp[1] = ly0;
                vp[3] = lx0; vp[4] = ly0;
                vp[6] = lx1; vp[7] = ly1;
                vp[9] = lx0; vp[10] = ly1;

                st[0] = u1; st[1] = v0;
                st[2] = u0; st[3] = v0;
                st[4] = u1; st[5] = v1;
                st[6] = u0; st[7] = v1;
            } else {
                const float scaleXFromY = (texH > 0) ? ((float)texW * quadScale / (float)texH) : quadScale;
                const float scaleYFromX = (texW > 0) ? ((float)texH * quadScale / (float)texW) : quadScale;
                const float lx0 = halfW - (float)(sy1 - srcY0) * scaleXFromY;
                const float lx1 = halfW - (float)(sy0 - srcY0) * scaleXFromY;
                const float ly0 = (float)(sx0 - srcX0) * scaleYFromX - halfH;
                const float ly1 = (float)(sx1 - srcX0) * scaleYFromX - halfH;

                vp[0] = lx1; vp[1] = ly0;
                vp[3] = lx0; vp[4] = ly0;
                vp[6] = lx1; vp[7] = ly1;
                vp[9] = lx0; vp[10] = ly1;

                st[0] = u0; st[1] = v0;
                st[2] = u0; st[3] = v1;
                st[4] = u1; st[5] = v0;
                st[6] = u1; st[7] = v1;
            }

            glBindTexture(GL_TEXTURE_2D, texId);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
    }
}
#endif

void Image::CreateTexture(uint16_t* data, uint32_t width, uint32_t height) {
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &this->texture);
    if (this->texture == 0 || this->texture == (GLuint)-1) {
        return;
    }
    glBindTexture(GL_TEXTURE_2D, this->texture);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    if (this->isTransparentMask == false) {
        uint32_t count = height * width;
        for (uint32_t i = 0; i < count; i++) {
            uint32_t pix = data[i];
            data[i] = (uint16_t)((pix & 0xffc0) | ((pix & 0x1f) << 1) | 1);
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, data);
    }
    else {
        uint32_t i = 0;
        uint16_t* dat = data;
        while (i < height * width) {
            uint32_t pix = *dat;
            if (pix == 0xf81f) {
                pix = 0x0000;
            }
            else {
                pix = (pix & 0xffc0) | (uint16_t)((pix & 0x1f) << 1) | 1;
            }
            *dat++ = pix;
            i++;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, data);
    }
}

void Image::DrawTexture(int texX, int texY, int texW, int texH, int posX, int posY, int rotateMode, int renderMode) {
#ifdef N64
    if (!this->EnsureTexture()) {
        return;
    }
#else
    if (this->texture == 0 || this->texture == (GLuint)-1) {
        return;
    }
#endif

    float scaleW, scaleH;
    float scaleX, scaleY;

    PFNGLACTIVETEXTUREPROC glActiveTexture = (PFNGLACTIVETEXTUREPROC)SDL_GL_GetProcAddress("glActiveTexture");

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    this->setRenderMode(renderMode);
    scaleW = (float)texW * 0.5f;
    scaleH = (float)texH * 0.5f;
    scaleX = (float)posX;
    scaleY = (float)posY;

    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(scaleW + (float)(scaleX), scaleH + (float)(scaleY), 0.0);
    switch (rotateMode)
    {
    case 1:
        glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
        break;
    case 2:
        glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
        break;
    case 3:
        glRotatef(270.0f, 0.0f, 0.0f, 1.0f);
        break;
    case 4:
        glScalef(-1.0f, 1.0f, 1.0f);
        break;
    case 5:
        glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
        glScalef(-1.0f, 1.0f, 1.0f);
        break;
    case 6:
        glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
        glScalef(-1.0f, 1.0f, 1.0f);
        break;
    case 7:
        glRotatef(270.0f, 0.0f, 0.0f, 1.0f);
        glScalef(-1.0f, 1.0f, 1.0f);
        break;
    case 8: // New
        glScalef(1.0f, -1.0f, 1.0f);
        break;
    default:
        break;
    }

#ifdef N64
    this->DrawQuadTiled(texX, texY, texW, texH, 1.0f, false);
#else
    float scaleTexW, scaleTexH;
    float vp[12];
    float st[8];

    vp[2] = 0.5f;
    vp[5] = 0.5f;
    vp[0] = scaleW;
    vp[8] = 0.5f;
    vp[11] = 0.5f;
    vp[3] = -scaleW;
    vp[6] = scaleW;
    vp[7] = scaleH;
    vp[1] = -scaleH;
    vp[4] = -scaleH;
    vp[9] = -scaleW;
    vp[10] = scaleH;

    scaleTexW = 1.0f / (float)this->texWidth;
    scaleTexH = 1.0f / (float)this->texHeight;
    st[0] = scaleTexW * (float)(texW + texX);
    st[4] = st[0];
    st[1] = scaleTexH * (float)texY;
    st[3] = st[1];
    st[2] = scaleTexW * (float)texX;
    st[6] = st[2];
    st[5] = scaleTexH * (float)(texH + texY);
    st[7] = st[5];

    glBindTexture(GL_TEXTURE_2D, this->texture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glVertexPointer(3, GL_FLOAT, 0, vp);
    glEnableClientState(GL_VERTEX_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, 0, st);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
#endif
    glPopMatrix();
}

void Image::setRenderMode(int renderMode) {
    Applet* app = CAppContainer::getInstance()->app;
    int color;

    switch (renderMode) {
    case 0:
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        if (this->isTransparentMask == false) {
            glDisable(GL_ALPHA_TEST);
            glDisable(GL_BLEND);
            return;
        }
        glEnable(GL_ALPHA_TEST);
        break;
    case 1:
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glColor4f(1.0f, 1.0f, 1.0f, 0.25f);
        glDisable(GL_ALPHA_TEST);
        break;
    case 2:
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
        glDisable(GL_ALPHA_TEST);
        break;
    case 3:
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_COLOR, GL_ONE);
        return;
    case 8:
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        color = Graphics::charColors[app->canvas->graphics.currentCharColor];
        glColor4ub(color >> 0x10 & 0xff, color >> 8 & 0xff, color & 0xff, color >> 0x18);
        return;
    case 12:
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glColor4f(1.0f, 1.0f, 1.0f, 0.75f);
        glDisable(GL_ALPHA_TEST);
        break;
    case 13:
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glColor4f(1.0f, 1.0f, 1.0f, app->canvas->blendSpecialAlpha);
        glDisable(GL_ALPHA_TEST);
        break;
    default:
        return;
    }
    glAlphaFunc(GL_GREATER, 0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
