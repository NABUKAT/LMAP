#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#include "ImgData.h"

MatrixPanel_I2S_DMA* disp = nullptr;

//----------------------------------------------------------------
// 初期化用
//----------------------------------------------------------------
void initDraw(MatrixPanel_I2S_DMA* dma_display) {
  disp = dma_display;
}

//----------------------------------------------------------------
// 画像表示用
//----------------------------------------------------------------
// 単色画像表示
void drawMonoColorBitmap(int16_t x, int16_t y, const uint16_t bitmap[], int16_t w, int16_t h, unsigned short color) {
  for (int16_t j = 0; j < h; j++) {
    for (int16_t i = 0; i < w; i++) {
      unsigned short c = pgm_read_word(&bitmap[j * w + i]);
      if (c == 0)
        disp->drawPixel(x + i, y + j, c);
      else
        disp->drawPixel(x + i, y + j, color);
    }
  }
}

// カラー画像表示
void drawRGBBitmap(int16_t x, int16_t y, const uint16_t bitmap[], int16_t w, int16_t h) {
  for (int16_t j = 0; j < h; j++) {
    for (int16_t i = 0; i < w; i++) {
      disp->drawPixel(x + i, y + j, pgm_read_word(&bitmap[j * w + i]));
    }
  }
}

//----------------------------------------------------------------
// 数字描写用(n: -99～99)
//----------------------------------------------------------------
// 共通関数
void drawNum(int16_t x, int16_t y, int8_t n, bool is_large, unsigned short color) {
  if (is_large) {
    switch (n) {
    case 1:
      drawMonoColorBitmap(x, y, IMG_L_1, IMG_L_N_W, IMG_L_N_H, color);
      break;
    case 2:
      drawMonoColorBitmap(x, y, IMG_L_2, IMG_L_N_W, IMG_L_N_H, color);
      break;
    case 3:
      drawMonoColorBitmap(x, y, IMG_L_3, IMG_L_N_W, IMG_L_N_H, color);
      break;
    case 4:
      drawMonoColorBitmap(x, y, IMG_L_4, IMG_L_N_W, IMG_L_N_H, color);
      break;
    case 5:
      drawMonoColorBitmap(x, y, IMG_L_5, IMG_L_N_W, IMG_L_N_H, color);
      break;
    case 6:
      drawMonoColorBitmap(x, y, IMG_L_6, IMG_L_N_W, IMG_L_N_H, color);
      break;
    case 7:
      drawMonoColorBitmap(x, y, IMG_L_7, IMG_L_N_W, IMG_L_N_H, color);
      break;
    case 8:
      drawMonoColorBitmap(x, y, IMG_L_8, IMG_L_N_W, IMG_L_N_H, color);
      break;
    case 9:
      drawMonoColorBitmap(x, y, IMG_L_9, IMG_L_N_W, IMG_L_N_H, color);
      break;
    case 0:
      drawMonoColorBitmap(x, y, IMG_L_0, IMG_L_N_W, IMG_L_N_H, color);
      break;
    }
  }
  else {
    switch (n) {
    case 1:
      drawMonoColorBitmap(x, y, IMG_S_1, IMG_S_N_W, IMG_S_N_H, color);
      break;
    case 2:
      drawMonoColorBitmap(x, y, IMG_S_2, IMG_S_N_W, IMG_S_N_H, color);
      break;
    case 3:
      drawMonoColorBitmap(x, y, IMG_S_3, IMG_S_N_W, IMG_S_N_H, color);
      break;
    case 4:
      drawMonoColorBitmap(x, y, IMG_S_4, IMG_S_N_W, IMG_S_N_H, color);
      break;
    case 5:
      drawMonoColorBitmap(x, y, IMG_S_5, IMG_S_N_W, IMG_S_N_H, color);
      break;
    case 6:
      drawMonoColorBitmap(x, y, IMG_S_6, IMG_S_N_W, IMG_S_N_H, color);
      break;
    case 7:
      drawMonoColorBitmap(x, y, IMG_S_7, IMG_S_N_W, IMG_S_N_H, color);
      break;
    case 8:
      drawMonoColorBitmap(x, y, IMG_S_8, IMG_S_N_W, IMG_S_N_H, color);
      break;
    case 9:
      drawMonoColorBitmap(x, y, IMG_S_9, IMG_S_N_W, IMG_S_N_H, color);
      break;
    case 0:
      drawMonoColorBitmap(x, y, IMG_S_0, IMG_S_N_W, IMG_S_N_H, color);
      break;
    }
  }
}

//----------------------------------------------------------------
// 気温描写用
//----------------------------------------------------------------
// 大きい数字
void drawNumL(int16_t x, int16_t y, int8_t n, unsigned short color) {
  // 範囲外の場合は何もしない
  if (n < -99 || n > 99)
    return;

  // カーソル
  int8_t cur = 0;

  // 符号表示
  // マイナス2桁の場合
  if (n < -9) {
    drawMonoColorBitmap(x + cur, y, IMG_L_M, IMG_L_M_W, IMG_L_M_H, color);
    cur += IMG_L_M_W;
  }
  // マイナス一桁の場合
  else if (n < 0) {
    cur += IMG_L_N_W;
    drawMonoColorBitmap(x + cur, y, IMG_L_M, IMG_L_M_W, IMG_L_M_H, color);
    cur += IMG_L_M_W;
  }
  // プラス1桁の場合(もマイナスがある場合と同じ位置に表示する)
  else if(n < 10) {
    cur += IMG_L_N_W;
    cur += IMG_L_M_W;
  }
  // プラス2桁の場合
  else
    cur += IMG_S_M_W;

  // 数値から符号を消す
  n = n < 0 ? n * (-1) : n;

  // 2桁の場合
  if (n >= 10) {
    int8_t n10 = n / 10;
    drawNum(x + cur, y, n10, true, color);
    cur += IMG_L_N_W;
  }

  // 1桁目を表示
  int8_t n1 = n % 10;
  drawNum(x + cur, y, n1, true, color);
}

// 小さい数字
void drawNumS(int16_t x, int16_t y, int8_t n, unsigned short color) {
  // 範囲外の場合は何もしない
  if (n < -99 || n > 99)
    return;

  // カーソル
  int8_t cur = 0;

  // 符号表示
  // マイナス2桁の場合
  if (n < -9) {
    drawMonoColorBitmap(x + cur, y, IMG_S_M, IMG_S_M_W, IMG_S_M_H, color);
    cur += IMG_S_M_W;
  }
  // マイナス1桁の場合
  else if (n < 0) {
    cur += IMG_S_N_W;
    drawMonoColorBitmap(x + cur, y, IMG_S_M, IMG_S_M_W, IMG_S_M_H, color);
    cur += IMG_S_M_W;
  }
  // プラス1桁の場合(もマイナスがある場合と同じ位置に表示する)
  else if(n < 10)
    cur += IMG_S_M_W;
  // プラス2桁の場合(はマイナスがあるときよりも中央に寄せる)
  else
    cur += IMG_S_M_W - 2;

  // 数値から符号を消す
  n = n < 0 ? n * (-1) : n;

  // 2桁の場合
  if (n >= 10) {
    int8_t n10 = n / 10;
    drawNum(x + cur, y, n10, false, color);
    cur += IMG_S_N_W;
  }

  // 1桁目を表示
  int8_t n1 = n % 10;
  drawNum(x + cur, y, n1, false, color);
}

//----------------------------------------------------------------
// IPアドレス(URL)表示用
//----------------------------------------------------------------
void drawIPAddr(int16_t x, int16_t y, String ip_addr_str, unsigned short color) {
  int8_t cur = 0;
  int len = ip_addr_str.length();
  // IPアドレスを1文字ずつ処理
  for (int i = 0; i < len; i++) {
    // １文字取り出す
    String c = ip_addr_str.substring(i, i + 1);
    // ドットの場合
    if (c.compareTo(".") == 0) {
      drawMonoColorBitmap(x + cur, y, IMG_DOT, IMG_DOT_W, IMG_DOT_H, color);
      cur += IMG_DOT_W;
    }
    else if(c.compareTo(":") == 0) {
      drawMonoColorBitmap(x + cur, y, IMG_COLON, IMG_COLON_W, IMG_COLON_H, color);
      cur += IMG_COLON_W;
    }
    // 数字の場合
    else {
      drawNum(x + cur, y, c.toInt(), false, color);
      cur += IMG_S_N_W;
    }
  }
}

//----------------------------------------------------------------
// おみくじ表示用
//----------------------------------------------------------------
void drawOmikuji(int16_t x, int16_t y, uint8_t omikuji_resulst) {
  switch (omikuji_resulst) {
  case 0:
    drawRGBBitmap(x, y, IMG_O_0, IMG_O_W, IMG_O_H);
    break;
  case 1:
    drawRGBBitmap(x, y, IMG_O_1, IMG_O_W, IMG_O_H);
    break;
  case 2:
    drawRGBBitmap(x, y, IMG_O_2, IMG_O_W, IMG_O_H);
    break;
  case 3:
    drawRGBBitmap(x, y, IMG_O_3, IMG_O_W, IMG_O_H);
    break;
  case 4:
    drawRGBBitmap(x, y, IMG_O_4, IMG_O_W, IMG_O_H);
    break;
  }
}