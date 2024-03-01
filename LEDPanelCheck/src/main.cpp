#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

//----------------------------------------------------------------
// マトリックスパネル用
//----------------------------------------------------------------
#define PANEL_W 64         // パネルの横幅(pix)
#define PANEL_H 32         // パネルの縦幅(pix)
#define PANEL_NUM 1        // パネルの枚数
MatrixPanel_I2S_DMA* d = nullptr;
HUB75_I2S_CFG mxconfig;
const uint8_t high_brightness = 90;  // LEDの明るさ(0-255)

//----------------------------------------------------------------
// 色の定義(16bit)
//----------------------------------------------------------------
const unsigned short c_line_r = 0xf887;     // 赤色
const unsigned short c_line_g = 0x17e7;     // 緑色
const unsigned short c_line_b = 0x209f;     // 青色
const unsigned short c_line_w = 0xffff;     // 白色

//----------------------------------------------------------------
// ディスプレイ初期化用
//----------------------------------------------------------------
void initMainDisplay() {
  d->setBrightness8(high_brightness);
  d->clearScreen();
}

//----------------------------------------------------------------
// パネルの四隅に線を引く
// 想定通りに表示されない場合
// https://www.microfan.jp/2023/01/esp32-hub75-matrixpanel-dma/
//----------------------------------------------------------------
void showBase() {
  // 左線(赤)
  d->drawLine(0, 0, 0, 31, c_line_r);
  // 上線(緑)
  d->drawLine(0, 0, 63, 0, c_line_g);
  // 右線(青)
  d->drawLine(63, 0, 63, 31, c_line_b);
  // 下線(白)
  d->drawLine(0, 31, 63, 31, c_line_w);
}

//----------------------------------------------------------------
// セットアップ
//----------------------------------------------------------------
void setup() {
  // デバッグ用
  Serial.begin(115200);
  delay(1000);

  // ディスプレイ使用準備
  mxconfig = HUB75_I2S_CFG(PANEL_W, PANEL_H, PANEL_NUM);
  d = new MatrixPanel_I2S_DMA();
  mxconfig.double_buff = true;

  // x=0 to x=63の問題がある場合はコメントアウトを外す
  //mxconfig.clkphase = false;

  d->begin(mxconfig);

  // ディスプレイ初期化
  initMainDisplay();
}

//----------------------------------------------------------------
// メインループ
//----------------------------------------------------------------
void loop() {
  // ベース描写
  showBase();

  // ループディレイ
  delay(600);
  
  // 画面更新
  d->flipDMABuffer();

  // 画面バッファ消去
  d->clearScreen();
}