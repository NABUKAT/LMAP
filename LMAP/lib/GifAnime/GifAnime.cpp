#define FILESYSTEM SPIFFS
#include <SPIFFS.h>
#include <AnimatedGIF.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

MatrixPanel_I2S_DMA* dma_display = nullptr;
uint8_t dma_brightness = 128;

AnimatedGIF gif;
File f;

void GIFDraw(GIFDRAW* pDraw) {
  uint8_t* s;
  uint16_t* d, * usPalette, usTemp[320];
  int x, y, iWidth;

  iWidth = pDraw->iWidth;
  if (iWidth > MATRIX_WIDTH)
    iWidth = MATRIX_WIDTH;

  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y;

  s = pDraw->pPixels;
  if (pDraw->ucDisposalMethod == 2) {
    for (x = 0; x < iWidth; x++) {
      if (s[x] == pDraw->ucTransparent)
        s[x] = pDraw->ucBackground;
    }
    pDraw->ucHasTransparency = 0;
  }
  if (pDraw->ucHasTransparency) {
    uint8_t* pEnd, c, ucTransparent = pDraw->ucTransparent;
    int x, iCount;
    pEnd = s + pDraw->iWidth;
    x = 0;
    iCount = 0;
    while (x < pDraw->iWidth) {
      c = ucTransparent - 1;
      d = usTemp;
      while (c != ucTransparent && s < pEnd) {
        c = *s++;
        if (c == ucTransparent) {
          s--;
        }
        else {
          *d++ = usPalette[c];
          iCount++;
        }
      }
      if (iCount) {
        for (int xOffset = 0; xOffset < iCount; xOffset++) {
          dma_display->drawPixel(x + xOffset, y, usTemp[xOffset]);
        }
        x += iCount;
        iCount = 0;
      }
      c = ucTransparent;
      while (c == ucTransparent && s < pEnd) {
        c = *s++;
        if (c == ucTransparent)
          iCount++;
        else
          s--;
      }
      if (iCount) {
        x += iCount;
        iCount = 0;
      }
    }
  }
  else {
    s = pDraw->pPixels;
    for (x = 0; x < pDraw->iWidth; x++) {
      dma_display->drawPixel(x, y, usPalette[*s++]);
    }
  }
}

void* GIFOpenFile(const char* fname, int32_t* pSize) {
  Serial.print("Playing gif: ");
  Serial.println(fname);
  f = FILESYSTEM.open(fname);
  if (f) {
    *pSize = f.size();
    return (void*)&f;
  }
  return NULL;
}

void GIFCloseFile(void* pHandle) {
  File* f = static_cast<File*>(pHandle);
  if (f != NULL)
    f->close();
}

int32_t GIFReadFile(GIFFILE* pFile, uint8_t* pBuf, int32_t iLen) {
  int32_t iBytesRead;
  iBytesRead = iLen;
  File* f = static_cast<File*>(pFile->fHandle);
  if ((pFile->iSize - pFile->iPos) < iLen)
    iBytesRead = pFile->iSize - pFile->iPos - 1;
  if (iBytesRead <= 0)
    return 0;
  iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
  pFile->iPos = f->position();
  return iBytesRead;
}

int32_t GIFSeekFile(GIFFILE* pFile, int32_t iPosition) {
  int i = micros();
  File* f = static_cast<File*>(pFile->fHandle);
  f->seek(iPosition);
  pFile->iPos = (int32_t)f->position();
  i = micros() - i;
  return pFile->iPos;
}

unsigned long start_tick = 0;

void initGifDisplay() {
  dma_display->setBrightness8(dma_brightness);
  dma_display->fillScreen(dma_display->color565(0, 0, 0));
}

void initGifAnime(MatrixPanel_I2S_DMA* d, uint8_t brightness) {
  SPIFFS.begin();
  gif.begin(LITTLE_ENDIAN_PIXELS);
  dma_display = d;
  dma_brightness = brightness;
}

void ShowGIF(String name, uint8_t stopframe) {
  initGifDisplay();

  if (gif.open(name.c_str(), GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw)) {
    uint8_t cnt = 0;
    while (gif.playFrame(true, NULL)) {
      if (cnt == stopframe)
        break;
      cnt++;
      dma_display->flipDMABuffer();
      dma_display->clearScreen();
    }
    dma_display->flipDMABuffer();
    dma_display->clearScreen();
    gif.close();
    delay(5000);
  }
}
