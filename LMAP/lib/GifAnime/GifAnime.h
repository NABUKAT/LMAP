#ifndef GIFANIME_H_
#define GIFANIME_H_

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

void initGifAnime(MatrixPanel_I2S_DMA *d, uint8_t brightness);
void ShowGIF(String name, uint8_t stopframe);

#endif