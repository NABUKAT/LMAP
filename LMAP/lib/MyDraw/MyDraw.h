#ifndef MYDRAW_H_
#define MYDRAW_H_

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

void initDraw(MatrixPanel_I2S_DMA* dma_display);
void drawMonoColorBitmap(int16_t x, int16_t y, const uint16_t bitmap[], int16_t w, int16_t h, unsigned short color);
void drawRGBBitmap(int16_t x, int16_t y, const uint16_t bitmap[], int16_t w, int16_t h);
void drawNumL(int16_t x, int16_t y, int8_t n, unsigned short color);
void drawNumS(int16_t x, int16_t y, int8_t n, unsigned short color);
void drawIPAddr(int16_t x, int16_t y, String ip_addr_str, unsigned short color);
void drawOmikuji(int16_t x, int16_t y, uint8_t omikuji_resulst);

#endif