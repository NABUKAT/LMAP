#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <ezTime.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <EEPROM.h>

#include <GifAnime.h>
#include <ImgData.h>
#include <MyDraw.h>

// 配列のインデックス定義
#define X 0
#define Y 1
#define W 2
#define H 3

//----------------------------------------------------------------
// マトリックスパネル用
//----------------------------------------------------------------
#define PANEL_W 64         // パネルの横幅(pix)
#define PANEL_H 32         // パネルの縦幅(pix)
#define PANEL_NUM 1        // パネルの枚数
MatrixPanel_I2S_DMA* d = nullptr;
HUB75_I2S_CFG mxconfig;
const uint8_t low_brightness = 30;   // LEDの明るさ(0-255)(夜間用)
const uint8_t high_brightness = 90;  // LEDの明るさ(0-255)(昼間用)
const uint8_t night_start_hour = 18; // 夜の開始時間(h)
const uint8_t night_end_hour = 6;    // 夜の終了時間(h)

//----------------------------------------------------------------
// 色の定義(16bit)
//----------------------------------------------------------------
const unsigned short c_char = 0xDEFB;     // 文字色
const unsigned short c_line = 0x7BEF;     // 境界線の色
const unsigned short c_max_temp = 0xE0E4; // 最高気温の色
const unsigned short c_min_temp = 0x6DDF; // 最低気温の色

//----------------------------------------------------------------
// Webサーバ用
//----------------------------------------------------------------
#define WEB_PORT 8080                     // Webサーバポート
AsyncWebServer aws(WEB_PORT);
struct weather_place {	                  // 位置情報を格納する構造体
  char latitude[20];
  char longitude[20];
  char check[10];
};
weather_place wp;                         // 位置情報格納変数
char mac_addr[20];

//----------------------------------------------------------------
// Wi-Fi用
//----------------------------------------------------------------
#define RESET_SW 0                        // リセットスイッチ
const char* ssid = "LMAP";                // APモードのWi-Fi設定(SSID)
const char* password = "LMAPLMAP";        // APモードのWi-Fi設定(Password)
const int16_t show_ap_xy[] = { 1, 1 };    // SSID/Passを表示する位置
const int16_t show_apip_xy[] = { 1, 16 }; // APのIPを表示する位置
uint8_t reset_cnt = 0;                    // リセットスイッチが押された秒数をカウント
WiFiManager wifiManager;
unsigned long previousMillis = 0;
unsigned long interval = 30000;
String wifiuptime = "2024/1/1 12:00";     // Wi-Fi接続時刻

//----------------------------------------------------------------
// IPアドレス表示用
//----------------------------------------------------------------
const int16_t show_ip_xy[] = { 1, 2 };      // IPアドレスを表示する位置
const int16_t show_ip_sec_xy[] = { 1, 15 }; // IPアドレスを表示する時間(秒)カウンタ位置
const uint8_t show_ip_sec = 30;             // 起動直後にIPアドレスを表示する時間(秒)(99秒未満)
String show_ip_addr = "";                   // 取得したIPアドレスを格納
bool show_ip_flg = true;                    // 起動直後にIPアドレスを表示する
uint8_t show_ip_cnt = 0;                    // IPアドレスを表示する時間(秒)カウンタ

//----------------------------------------------------------------
// 天気予報表示用
//----------------------------------------------------------------
#define NO_DEFAULT "nodefault"
// 天気予報のデフォルト地域を緯度、経度で指定(国会議事堂)
const char* latitude = "35.675848753527646";
const char* longitude = "139.74478289446475";
const int16_t weather_xy[] = { 1, 2 };    // 天気予報アイコンの位置
const int16_t temp_now_xy[] = { 19, 3 };  // 現在気温の位置
const int16_t temp_max_xy[] = { 19, 13 }; // 最高気温の位置
const int16_t temp_min_xy[] = { 32, 13 }; // 最低気温の位置
const int16_t wind_xy[] = { 4, 23 };      // 風向きアイコンの位置
const int16_t wind_mps_xy[] = { 12, 23 }; // 風速の位置
const uint16_t update_sec = 900;          // アップデート間隔
char om_url[320];                         // URL
HTTPClient http;                          // HTTP Client
StaticJsonDocument<1000> doc;             // JSONパーサ
bool weather_ok = false;                  // 取得完了フラグ
int8_t temp = 25;                         // 現在気温
uint8_t winsp = 2;                        // 風速
uint16_t windr = 180;                     // 風向
uint16_t wcode = 0;                       // 天気コード
int8_t max_temp = 27;                     // 最高気温
int8_t min_temp = 18;                     // 最低気温
uint16_t weather_cnt = update_sec;        // アップデートカウント
String last_w_time = "2024/1/1 12:00";    // 最新天気取得時刻

//----------------------------------------------------------------
// 時報表示用
//----------------------------------------------------------------
Timezone myTZ;                                // タイムゾーン設定
const String ntp_server = "ntp.nict.jp";      // NTPサーバアドレス
const uint16_t timesync_sec = 60;             // 時刻同期間隔(秒)
uint8_t now_hour = 12;                        // 時刻(h)
const String fp_clockgif = "/gifs/clock.gif"; // 時計GIFのパスを指定
const uint8_t clock_brightness = 128;         // 時報イルミネーションの明るさを指定
const String dt_format = "Y/m/d H:i";         // 時刻表記のフォーマット
String uptime = "2024/1/1 12:00";             // システム起動時刻

//----------------------------------------------------------------
// わんこ表示用
//----------------------------------------------------------------
#define WANKO_STAT_MOVE 0
#define WANKO_STAT_SLEEP 1
const int16_t wanko_init_xy[] = { 48, 3 };         // わんこの初期位置
const int16_t wanko_mv_xywh[] = { 44, 0, 20, 32 }; // わんこの動ける範囲
const int16_t wanko_sp_xywh[] = { 44, 9, 7, 12 };  // わんこが眠れる範囲
const uint8_t wanko_sp_hour = 20;                  // わんこが眠る時間(h)
const uint8_t wanko_wk_hour = 6;                   // わんこが起きる時間(h)
const uint8_t wanko_meal_hours[] = { 7, 12, 18 };  // わんこの食事時間(h)
const uint8_t wanko_n = 5;                         // わんこが一度に歩く歩数
const uint16_t wanko_unko_steps = 30000;           // わんこはwanko_unko_steps歩に1度の確率でうんこをする
const uint16_t wanko_unko_maxcnt = 900;            // うんこを表示する最大秒数
const uint16_t wanko_meal_maxcnt = 900;            // わんこ食事最大秒数
int16_t wanko_xy[] = { 48, 3 };                    // わんこの現在位置
int8_t wanko_mv_xy[] = { 0, 0 };                   // わんこの移動方向
uint8_t wanko_cnt = 0;                             // わんこが歩いた歩数を数える
uint8_t wanko_stat = WANKO_STAT_MOVE;              // わんこ状態
bool wanko_unko = false;                           // うんこフラグ
uint16_t wanko_unko_cnt = 0;                       // うんこを表示する秒数カウンタ
int16_t wanko_unko_xy[] = { 44, 9 };               // うんこ表示位置
bool wanko_meal = false;                           // わんこ食事中フラグ
bool wanko_meal_comp = false;                      // わんこ食事完了フラグ
uint16_t wanko_meal_cnt = 0;                       // わんこ食事秒数カウンタ
int16_t wanko_meal_xy[] = { 44, 9 };               // わんこ食事表示位置
const float wanko_gx = wanko_mv_xywh[X] + (float)wanko_mv_xywh[W] / 2; // わんこの動ける範囲を2分割するX軸
const float wanko_gy = wanko_mv_xywh[Y] + (float)wanko_mv_xywh[H] / 2; // わんこの動ける範囲を2分割するY軸
const uint8_t num_meal = sizeof(wanko_meal_hours) / sizeof(int);       // わんこの１日の食事回数

//----------------------------------------------------------------
// おみくじ表示用
//----------------------------------------------------------------
#define O_DAIKICHI 0 // 大吉
#define O_CHUKICHI 1 // 中吉
#define O_SHOKICHI 2 // 小吉
#define O_KICHI    3 // 吉
#define O_SUEKICHI 4 // 末吉
const uint8_t show_omikuji_sec = 10;   // おみくじ表示時間(秒)
const int16_t omikuji_xy[] = { 0, 0 }; // おみくじの表示位置
bool show_omikuji = false;             // おみくじ表示フラグ
uint8_t show_omikuji_cnt = 0;          // おみくじ表示カウンタ
uint8_t omikuji_result = O_DAIKICHI;   // おみくじ結果

//----------------------------------------------------------------
// ディスプレイ初期化用
//----------------------------------------------------------------
void initMainDisplay() {
  // 時間(h)の取得
  now_hour = hour();
  // 夜間は暗めにする
  if (now_hour >= night_start_hour || now_hour < night_end_hour)
    d->setBrightness8(low_brightness);
  // 昼間は明るめにする
  else
    d->setBrightness8(high_brightness);
  d->clearScreen();
}

//----------------------------------------------------------------
// ベース描写用
//----------------------------------------------------------------
void showBase() {
  // 長い縦棒
  d->drawLine(43, 1, 43, 30, c_line);
  // 短い縦棒
  d->drawLine(30, 13, 30, 17, c_line);
  // 度C
  drawMonoColorBitmap(35, 3, IMG_L_DC, IMG_L_DC_W, IMG_L_DC_H, c_char);
  // m/s
  drawMonoColorBitmap(29, 25, IMG_MPS, IMG_MPS_W, IMG_MPS_H, c_char);
}

//----------------------------------------------------------------
// APモード用
//----------------------------------------------------------------
// Wi-Fi設定用のSSIDと設定用URLの表示
void apModeDiscription(WiFiManager* myWiFiManager) {
  // SSIDとパスワードを表示
  drawMonoColorBitmap(show_ap_xy[X], show_ap_xy[Y], IMG_AP_MODE, IMG_AP_MODE_W, IMG_AP_MODE_H, c_char);
  // 設定用URLへのWeb接続を促す
  drawIPAddr(show_apip_xy[X], show_apip_xy[Y], WiFi.softAPIP().toString(), c_char);
  // 画面更新
  d->flipDMABuffer();
  // 画面バッファ消去
  d->clearScreen();
}

// Wi-Fiリセット処理(別のWi-Fiにつなぎたい場合など)
void checkResetWifi() {
  // リセットスイッチの状態を確認
  uint8_t bt = digitalRead(RESET_SW);
  if (bt == LOW)
    reset_cnt++;
  else
    reset_cnt = 0;
  // 3秒長押しを検知し、古いWi-Fi情報を削除し、再起動。再起動後はAPモードへ移行する。
  if (reset_cnt > 3) {
    WiFi.disconnect(true);
    WiFi.begin("0", "0");
    ESP.restart();
    delay(1000);
  }
}

// Wi-Fi再接続処理
void reconnectWifi(){
  unsigned long currentMillis = millis();
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= interval)) {
    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
    wifiuptime = dateTime(dt_format);
  }
}

//----------------------------------------------------------------
// IPアドレス表示用
//----------------------------------------------------------------
// 秒が変わったら検知
int8_t wasSecond = -1;
bool mySecondChanged() {
  if (second() != wasSecond) {
    wasSecond = second();
    return true;
  }
  else
    return false;
}

// IPアドレス表示処理
void showIPAddr() {
  // Wi-Fi接続が完了している場合
  if ((WiFi.status() == WL_CONNECTED)) {
    // IPアドレス表示
    drawIPAddr(show_ip_xy[X], show_ip_xy[Y], show_ip_addr + ":" + String(WEB_PORT), c_char);
    // ezTimeの定期実行
    events();
    // 残り秒数
    int8_t show_sec = show_ip_sec - show_ip_cnt;
    // 秒が変わったら
    if (mySecondChanged()) {
      // カウントが完了した場合
      if (show_sec <= 0) {
        // メイン処理ループへ
        show_ip_flg = false;
      }
      show_ip_cnt++;
    }
    // 残り秒を表示
    drawNumS(show_ip_sec_xy[X], show_ip_sec_xy[Y], show_sec, c_char);
  }
  // Wi-Fi接続なしの場合
  else {
    // メイン処理ループへ
    show_ip_flg = false;
  }
}

//----------------------------------------------------------------
// 天気予報表示用(Wi-Fi未接続の場合は、サンプルの結果を表示)
//----------------------------------------------------------------
// 天気、風向、風速表示
void showOutWeather() {
  // 天気予報表示
  uint8_t iwc = 0;
  switch (wcode) {
  case 1:
  case 2:
  case 3:
    iwc = 1;
    break;
  case 45:
  case 48:
    iwc = 2;
    drawRGBBitmap(weather_xy[X], weather_xy[Y], IMG_W3, IMG_W_W, IMG_W_H);
    break;
  case 51:
  case 53:
  case 55:
  case 81:
  case 82:
  case 56:
  case 57:
  case 80:
  case 85:
  case 86:
    iwc = 3;
    drawRGBBitmap(weather_xy[X], weather_xy[Y], IMG_W4, IMG_W_W, IMG_W_H);
    break;
  case 61:
  case 63:
  case 65:
  case 95:
  case 96:
  case 99:
    iwc = 4;
    drawRGBBitmap(weather_xy[X], weather_xy[Y], IMG_W5, IMG_W_W, IMG_W_H);
    break;
  case 66:
  case 67:
  case 71:
  case 73:
  case 75:
  case 77:
    iwc = 5;
    drawRGBBitmap(weather_xy[X], weather_xy[Y], IMG_W6, IMG_W_W, IMG_W_H);
    break;
  }
  // 晴れかつ夜間の場合は月を表示
  if (iwc == 0 && (now_hour >= 18 || now_hour <= 5))
    drawRGBBitmap(weather_xy[X], weather_xy[Y], IMG_W7, IMG_W_W, IMG_W_H);
  // 晴れかつ昼間の場合は太陽を表示
  else if (iwc == 0 && (now_hour < 18 && now_hour > 5))
    drawRGBBitmap(weather_xy[X], weather_xy[Y], IMG_W1, IMG_W_W, IMG_W_H);
  // 晴れ時々曇りかつ夜間の場合は月と雲を表示
  else if (iwc == 1 && (now_hour >= 18 || now_hour <= 5))
    drawRGBBitmap(weather_xy[X], weather_xy[Y], IMG_W8, IMG_W_W, IMG_W_H);
  // 晴れ時々曇りかつ昼間の場合は太陽と雲を表示
  else if (iwc == 1 && (now_hour < 18 && now_hour > 5))
    drawRGBBitmap(weather_xy[X], weather_xy[Y], IMG_W2, IMG_W_W, IMG_W_H);

  // 風向表示
  if (windr > 337 || windr <= 22)
    drawRGBBitmap(wind_xy[X], wind_xy[Y], IMG_F000, IMG_F_W, IMG_F_H);
  else if (windr > 22 && windr <= 67)
    drawRGBBitmap(wind_xy[X], wind_xy[Y], IMG_F045, IMG_F_W, IMG_F_H);
  else if (windr > 67 && windr <= 112)
    drawRGBBitmap(wind_xy[X], wind_xy[Y], IMG_F090, IMG_F_W, IMG_F_H);
  else if (windr > 112 && windr <= 157)
    drawRGBBitmap(wind_xy[X], wind_xy[Y], IMG_F135, IMG_F_W, IMG_F_H);
  else if (windr > 157 && windr <= 202)
    drawRGBBitmap(wind_xy[X], wind_xy[Y], IMG_F180, IMG_F_W, IMG_F_H);
  else if (windr > 202 && windr <= 247)
    drawRGBBitmap(wind_xy[X], wind_xy[Y], IMG_F225, IMG_F_W, IMG_F_H);
  else if (windr > 247 && windr <= 292)
    drawRGBBitmap(wind_xy[X], wind_xy[Y], IMG_F270, IMG_F_W, IMG_F_H);
  else if (windr > 292 && windr <= 337)
    drawRGBBitmap(wind_xy[X], wind_xy[Y], IMG_F315, IMG_F_W, IMG_F_H);
  // 現在気温表示
  drawNumL(temp_now_xy[X], temp_now_xy[Y], temp, c_char);
  // 最高気温表示
  drawNumS(temp_max_xy[X], temp_max_xy[Y], max_temp, c_max_temp);
  // 最低気温表示
  drawNumS(temp_min_xy[X], temp_min_xy[Y], min_temp, c_min_temp);
  // 風速表示
  drawNumL(wind_mps_xy[X], wind_mps_xy[Y], winsp, c_char);
}

// 天気予報APIでJSON取得
void getWeather() {
  if ((WiFi.status() == WL_CONNECTED)) {
    if (weather_cnt >= update_sec) {
      uint8_t errorcnt = 0;
      do {
        http.begin(om_url);
        uint16_t httpCode = http.GET();
        if (httpCode > 0) {
          if (httpCode == HTTP_CODE_OK) {
            String json = http.getString();
            DeserializationError error = deserializeJson(doc, json);
            // 正常な場合は値を取得。小数は整数表示(四捨五入)
            if (!error) {
              temp = (int)((double)doc["current"]["temperature_2m"] + 0.5);
              winsp = (int)((double)doc["current"]["wind_speed_10m"] + 0.5);
              windr = doc["current"]["wind_direction_10m"];
              wcode = doc["current"]["weather_code"];
              max_temp = (int)((double)doc["daily"]["temperature_2m_max"][0] + 0.5);
              min_temp = (int)((double)doc["daily"]["temperature_2m_min"][0] + 0.5);
              weather_ok = true;
              last_w_time = dateTime(dt_format);
            }
          }
        }
        http.end();
        delay(100);
        errorcnt++;
      } while (!weather_ok && errorcnt <= 3);
      weather_ok = false;
      weather_cnt = 0;
    }
    weather_cnt++;
  }
}

//----------------------------------------------------------------
// 時報表示用(Wi-Fi接続時のみ)
//----------------------------------------------------------------
// 初期化用
void initTimeSignal() {
  if ((WiFi.status() == WL_CONNECTED)) {
    initGifAnime(d, clock_brightness);
    if (!myTZ.setCache(64)){
      myTZ.setLocation(F("Asia/Tokyo")); // Timezone の一時設定。日本時間に変更
      myTZ.setDefault();                 // Timezone の恒久設定
    }
    setServer(ntp_server);             // NTPサーバの設定
    setInterval(timesync_sec);         // NTPデータの更新の時間間隔
    waitForSync();
  }
}

// 更新用
void updateTimeSignal() {
  if ((WiFi.status() == WL_CONNECTED)) {
    // ezTimeの定期実行
    events();

    // 分が変わった場合に実行
    if (minuteChanged()) {
      now_hour = hour() == 0 ? 24 : hour();
      // 毎時0分に時報を実行
      if (minute() == 0) {
        d->flipDMABuffer();                     // 画面開放
        ShowGIF(fp_clockgif, now_hour * 4 - 1); // Gif表示
        initMainDisplay();                      // メイン画面用にディプレイを初期化する
      }
    }
  }
}

//----------------------------------------------------------------
// わんこ表示用
//----------------------------------------------------------------
// 当たり判定
bool atari(int16_t o1_x, int16_t o1_y, int16_t o1_w, int16_t o1_h, int16_t o2_x, int16_t o2_y, int16_t o2_w, int16_t o2_h) {
  float d_x = (o2_x + (float)o2_w / 2) - (o1_x + (float)o1_w / 2);
  float d_y = (o2_y + (float)o2_h / 2) - (o1_y + (float)o1_h / 2);
  d_x = d_x < 0 ? (-1) * d_x : d_x;
  d_y = d_y < 0 ? (-1) * d_y : d_y;
  float th_x = (float)(o1_w + o2_w) / 2;
  float th_y = (float)(o1_h + o2_h) / 2;
  if (d_x < th_x && d_y < th_y)
    return true;
  return false;
}

// わんこ表示
void showWanko() {
  // わんこが動いている場合
  if (wanko_stat == WANKO_STAT_MOVE) {
    // WANKO_N回に一度わんこを動かす方向を決める
    if (wanko_cnt >= wanko_n) {
      wanko_mv_xy[X] = random(-1, 2);
      wanko_mv_xy[Y] = random(-1, 2);
      wanko_cnt = 0;
    }

    // わんこの位置が範囲外になる場合は跳ね返る
    // 左の壁
    if (wanko_xy[X] + wanko_mv_xy[X] < wanko_mv_xywh[X])
      wanko_mv_xy[X] *= -1;
    // 上の壁
    if (wanko_xy[Y] + wanko_mv_xy[Y] < wanko_mv_xywh[Y])
      wanko_mv_xy[Y] *= -1;
    // 右の壁
    if (wanko_xy[X] + wanko_mv_xy[X] + IMG_WANKO_W > wanko_mv_xywh[X] + wanko_mv_xywh[W])
      wanko_mv_xy[X] *= -1;
    // 下の壁
    if (wanko_xy[Y] + wanko_mv_xy[Y] + IMG_WANKO_H > wanko_mv_xywh[Y] + wanko_mv_xywh[H])
      wanko_mv_xy[Y] *= -1;

    // 眠る時間になり、眠れる範囲に来たら眠る
    if (now_hour >= wanko_sp_hour || now_hour < wanko_wk_hour) {
      if (wanko_xy[X] > wanko_sp_xywh[X] && wanko_xy[X] <= wanko_sp_xywh[X] + wanko_sp_xywh[W]) {
        if (wanko_xy[Y] > wanko_sp_xywh[Y] && wanko_xy[Y] <= wanko_sp_xywh[Y] + wanko_sp_xywh[H]) {
          wanko_stat = WANKO_STAT_SLEEP;
          return;
        }
      }
    }

    // 時間が来たら食事を用意
    uint8_t meal_cnt = 0;
    for (uint8_t i = 0; i < num_meal; i++) {
      // 食事の時間が来た場合
      if (wanko_meal_hours[i] == now_hour) {
        // 食事中でない＆うんこ表示中でない＆食事完了後でない場合
        if (!wanko_meal && !wanko_unko && !wanko_meal_comp) {
          // 食事中フラグを立てる
          wanko_meal = true;
          // 食事の表示位置を決める(わんこの対角に配置)
          if ((wanko_xy[X] + (float)IMG_WANKO_W / 2 - wanko_gx) * (-1) < 0)
            wanko_meal_xy[X] = wanko_mv_xywh[X] + 1;
          else
            wanko_meal_xy[X] = wanko_mv_xywh[X] + wanko_mv_xywh[W] - IMG_WANKO_E_W - 1;
          if ((wanko_xy[Y] + (float)IMG_WANKO_H / 2 - wanko_gy) * (-1) < 0)
            wanko_meal_xy[Y] = wanko_mv_xywh[Y] + 1;
          else
            wanko_meal_xy[Y] = wanko_mv_xywh[Y] + wanko_mv_xywh[H] - IMG_WANKO_E_H - 1;
        }
        break;
      }
      meal_cnt++;
    }
    // 食事時間外になった場合
    if (wanko_meal_comp && meal_cnt == num_meal) {
      // 食事完了フラグを下げる
      wanko_meal_comp = false;
    }
    // 食事中の場合
    if (wanko_meal && !wanko_meal_comp) {
      // 食事を表示
      drawRGBBitmap(wanko_meal_xy[X], wanko_meal_xy[Y], IMG_WANKO_E, IMG_WANKO_E_W, IMG_WANKO_E_H);
      // わんこが食事に当たっていたら立ち止まる
      if (atari(wanko_xy[X], wanko_xy[Y], IMG_WANKO_W, IMG_WANKO_H, wanko_meal_xy[X], wanko_meal_xy[Y], IMG_WANKO_E_W, IMG_WANKO_E_H)) {
        wanko_mv_xy[X] = 0;
        wanko_mv_xy[Y] = 0;
        // 食事中カウンタを進める
        wanko_meal_cnt++;
      }
    }
    // 食事中カウンタが指定値を超えた場合
    if (wanko_meal_cnt > wanko_meal_maxcnt) {
      // 食事中フラグをリセット
      wanko_meal = false;
      // 食事完了フラグをセット
      wanko_meal_comp = true;
      // 食事中カウンタをリセット
      wanko_meal_cnt = 0;
    }

    // わんこは、たまにうんこをする
    if (!wanko_unko && !wanko_meal && random(0, wanko_unko_steps) == wanko_unko_steps / 2) {
      // フラグをセット
      wanko_unko = true;
      // うんこの表示位置を決める(わんこの対角に配置)
      if ((wanko_xy[X] + (float)IMG_WANKO_W / 2 - wanko_gx) * (-1) < 0)
        wanko_unko_xy[X] = wanko_mv_xywh[X] + 1;
      else
        wanko_unko_xy[X] = wanko_mv_xywh[X] + wanko_mv_xywh[W] - IMG_WANKO_U_W - 1;
      if ((wanko_xy[Y] + (float)IMG_WANKO_H / 2 - wanko_gy) * (-1) < 0)
        wanko_unko_xy[Y] = wanko_mv_xywh[Y] + 1;
      else
        wanko_unko_xy[Y] = wanko_mv_xywh[Y] + wanko_mv_xywh[H] - IMG_WANKO_U_H - 1;
    }
    if (wanko_unko) {
      // うんこを表示
      drawRGBBitmap(wanko_unko_xy[X], wanko_unko_xy[Y], IMG_WANKO_U, IMG_WANKO_U_W, IMG_WANKO_U_H);
      // わんこがうんこに当たっていたら立ち止まる
      if (atari(wanko_xy[X] + wanko_mv_xy[X], wanko_xy[Y] + wanko_mv_xy[Y], IMG_WANKO_W, IMG_WANKO_H, wanko_unko_xy[X], wanko_unko_xy[Y], IMG_WANKO_U_W, IMG_WANKO_U_H)) {
        wanko_mv_xy[X] = 0;
        wanko_mv_xy[Y] = 0;
      }
      // うんこカウンタを進める
      wanko_unko_cnt++;
    }
    // うんこカウンタが指定値を超えた場合
    if (wanko_unko_cnt > wanko_unko_maxcnt) {
      // フラグをリセット
      wanko_unko = false;
      // うんこカウンタをリセット
      wanko_unko_cnt = 0;
    }

    // わんこの位置を変更する
    wanko_xy[X] += wanko_mv_xy[X];
    wanko_xy[Y] += wanko_mv_xy[Y];

    // 移動したわんこを表示する
    // 右に向かう場合 | 食事中＆食事が右側にある場合は右を向く
    if (wanko_mv_xy[X] == 1 || (wanko_meal_cnt > 0 && wanko_meal_xy[X] + (float)IMG_WANKO_E_W / 2 > wanko_gx))
      drawRGBBitmap(wanko_xy[X], wanko_xy[Y], IMG_WANKO_R, IMG_WANKO_W, IMG_WANKO_H);
    // 上下または左に向かう場合は左を向く
    else
      drawRGBBitmap(wanko_xy[X], wanko_xy[Y], IMG_WANKO_L, IMG_WANKO_W, IMG_WANKO_H);

    // 食事中はハートマークを点滅する
    if (wanko_meal_cnt > 0 && wanko_meal_cnt % 2 == 0) {
      if (wanko_meal_xy[X] + (float)IMG_WANKO_E_W / 2 > wanko_gx)
        drawRGBBitmap(wanko_xy[X], wanko_xy[Y], IMG_WANKO_HT, IMG_WANKO_HT_W, IMG_WANKO_HT_H);
      else
        drawRGBBitmap(wanko_xy[X] + 10, wanko_xy[Y], IMG_WANKO_HT, IMG_WANKO_HT_W, IMG_WANKO_HT_H);
    }
  }
  // わんこが眠っている場合
  else if (wanko_stat == WANKO_STAT_SLEEP) {
    // 眠っているわんこを表示
    drawRGBBitmap(wanko_xy[X], wanko_xy[Y], IMG_WANKO_S, IMG_WANKO_S_W, IMG_WANKO_S_H);

    // ZZZを表示
    if (wanko_cnt % 2 == 0)
      drawRGBBitmap(wanko_xy[X] + 7, wanko_xy[Y] - 8, IMG_WANKO_Z1, IMG_WANKO_Z1_W, IMG_WANKO_Z1_H);
    else
      drawRGBBitmap(wanko_xy[X] + 7, wanko_xy[Y] - 8, IMG_WANKO_Z2, IMG_WANKO_Z2_W, IMG_WANKO_Z2_H);

    // 時間が来たら起きる
    if (now_hour >= wanko_wk_hour && now_hour < wanko_sp_hour)
      wanko_stat = WANKO_STAT_MOVE;

    // WANKO_CNTをリセット
    if (wanko_cnt >= 9)
      wanko_cnt = 0;
  }
  wanko_cnt++;
}

//----------------------------------------------------------------
// おみくじ表示用
//----------------------------------------------------------------
// おみくじ結果表示
void showOmikujiResult() {
  // おみくじフラグONの場合
  if (show_omikuji) {
    // おみくじ結果を表示
    drawOmikuji(omikuji_xy[X], omikuji_xy[Y], omikuji_result);
    // ezTimeの定期実行
    events();
    // 残り秒数
    int8_t show_sec = show_omikuji_sec - show_omikuji_cnt;
    // 秒が変わったら
    if (mySecondChanged()) {
      // カウントが完了した場合
      if (show_sec < 0) {
        // おみくじフラグOFFへ
        show_omikuji = false;
        // カウンタ初期化
        show_omikuji_cnt = 0;
      }
      show_omikuji_cnt++;
    }
  }
}

// おみくじ処理を実施(大吉、中吉、小吉、吉、末吉)
void getWebOmikuji() {
  // ランダムにおみくじを引く
  omikuji_result = random(O_DAIKICHI, O_SUEKICHI + 1);
  // おみくじフラグをONにする
  show_omikuji = true;
}

//----------------------------------------------------------------
// Webサーバの準備
//----------------------------------------------------------------
// デフォルトハンドラ用
void onRequest(AsyncWebServerRequest* request) {
  //Handle Unknown Request
  request->send(404);
}

void onBody(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
  //Handle body
}

void onUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
  //Handle upload
}

String template_proc(const String& v) {
  if (v == "PLACE_INFO")
    return String(wp.latitude) + ", " + String(wp.longitude);
  if (v == "FREE_MEM")
    return String(esp_get_free_heap_size());
  if (v == "CPU_TEMP")
    return String(temperatureRead(), 1);
  if (v == "UP_TIME")
    return uptime;
  if (v == "WIFI_UP_TIME")
    return wifiuptime;
  if (v == "NOW_TIME")
    return dateTime(dt_format);
  if (v == "LAST_W_TIME")
    return last_w_time;
  if (v == "MAC_ADDR")
    return String(mac_addr);
  return String();
}

void initWebServer() {
  if ((WiFi.status() == WL_CONNECTED)) {
    // EEPROM初期化
    EEPROM.begin(64);

    // 保存した位置情報をメモリ上に取得
    EEPROM.get<weather_place>(0, wp);
    // デフォルト値の場合
    if (strcmp(wp.check, NO_DEFAULT)) {
      strcpy(wp.latitude, latitude);
      strcpy(wp.longitude, longitude);
    }

    // tenkiset.htmlを返す
    aws.on("/tenkiset", HTTP_GET, [](AsyncWebServerRequest* r) {
      r->send(SPIFFS, "/view/tenkiset.html", String(), false, template_proc);
      });

    // systeminfo.htmlを返す
    aws.on("/systeminfo", HTTP_GET, [](AsyncWebServerRequest* r) {
      r->send(SPIFFS, "/view/systeminfo.html", String(), false, template_proc);
      });

    // おみくじAPI
    aws.on("/omikuji", HTTP_GET, [](AsyncWebServerRequest* r) {
      if (!show_omikuji)
        getWebOmikuji();
      r->send(200);
      });

    // 再起動API
    aws.on("/restart", HTTP_GET, [](AsyncWebServerRequest* r) {
      ESP.restart();
      r->send(200);
      });

    // 位置情報を更新
    aws.on("/setplace", HTTP_GET, [](AsyncWebServerRequest* r) {
      // 緯度と経度を取得し、更新
      AsyncWebParameter* p1 = r->getParam("lat");
      String lat = p1->value();
      AsyncWebParameter* p2 = r->getParam("lon");
      String lon = p2->value();
      strcpy(wp.latitude, lat.c_str());
      strcpy(wp.longitude, lon.c_str());
      strcpy(wp.check, NO_DEFAULT);
      EEPROM.put<weather_place>(0, wp);
      EEPROM.commit();
      // Open-MeteoのURL再作成
      snprintf_P(om_url, sizeof om_url, PSTR("https://api.open-meteo.com/v1/forecast?latitude=%s&longitude=%s&current=temperature_2m,weather_code,wind_speed_10m,wind_direction_10m&daily=temperature_2m_max,temperature_2m_min&wind_speed_unit=ms&timezone=Asia%%2FTokyo&forecast_days=1"), wp.latitude, wp.longitude);
      // 天気予報を再取得
      weather_cnt = update_sec;
      r->send(200);
      });

    // 静的ファイルを返す(TOPとおみくじ用)
    aws.serveStatic("/", SPIFFS, "/static/").setDefaultFile("index.html").setCacheControl("max-age=600");

    // デフォルトハンドラ(その他リクエストの処理用)
    aws.onNotFound(onRequest);
    aws.onFileUpload(onUpload);
    aws.onRequestBody(onBody);

    // Webサーバ開始
    aws.begin();
    Serial.println(F("Web Server is Started."));
  }
}

//----------------------------------------------------------------
// OTA用
//----------------------------------------------------------------
void initOTA() {
  ArduinoOTA.setHostname("lmap2024");
  ArduinoOTA.onStart([]() {
    // OTA開始時にパネル表示用オブジェクトを消去してOTAにRAMを譲る
    d->stopDMAoutput();
    delete d;
  });
  ArduinoOTA.onEnd([]() {});
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {});
  ArduinoOTA.onError([](ota_error_t error) {});
  ArduinoOTA.begin();
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
  //mxconfig.clkphase = false; // x=0 to x=63の問題がある場合はコメントアウトを外す
  d->begin(mxconfig);
  d->setBrightness8(high_brightness);

  // 描写用初期化
  initDraw(d);

  // Wi-Fi接続(SSID設定されていなければ、APモードへ)
  wifiManager.setAPCallback(apModeDiscription);
  wifiManager.autoConnect(ssid, password);
  show_ip_addr = WiFi.localIP().toString();

  // MACアドレスを取得
  byte ma[6];
  WiFi.macAddress(ma);
  sprintf_P(mac_addr, PSTR("%02X:%02X:%02X:%02X:%02X:%02X"), ma[0], ma[1], ma[2], ma[3], ma[4], ma[5]);

  // 時報用の初期化
  initTimeSignal();
  uptime = dateTime(dt_format);
  wifiuptime = dateTime(dt_format);

  // ディスプレイ初期化
  initMainDisplay();

  // Webサーバ初期化
  initWebServer();

  // OTA初期化
  initOTA();

  // Open-MeteoのURL作成
  snprintf_P(om_url, sizeof om_url, PSTR("https://api.open-meteo.com/v1/forecast?latitude=%s&longitude=%s&current=temperature_2m,weather_code,wind_speed_10m,wind_direction_10m&daily=temperature_2m_max,temperature_2m_min&wind_speed_unit=ms&timezone=Asia%%2FTokyo&forecast_days=1"), wp.latitude, wp.longitude);

  // 天気予報の初回取得
  getWeather();
}

//----------------------------------------------------------------
// メインループ
//----------------------------------------------------------------
void loop() {
  // OTA処理
  ArduinoOTA.handle();

  // 起動直後のIPアドレス表示
  if (show_ip_flg) {
    // IPアドレス表示
    showIPAddr();

    // ループディレイ
    delay(20);
  }
  // おみくじ結果を表示する
  else if (show_omikuji) {
    // おみくじ結果表示
    showOmikujiResult();

    // ループディレイ
    delay(1000);
  }
  // メイン処理ループ
  else {
    // Wi-Fiリセットボタンの3秒押しチェック
    checkResetWifi();

    // 時報表示
    updateTimeSignal();

    // ベース描写
    showBase();

    // 天気予報表示
    getWeather();
    showOutWeather();

    // わんこ表示
    showWanko();

    // ループディレイ
    delay(600);
  }
  
  // 画面更新
  d->flipDMABuffer();

  // 画面バッファ消去
  d->clearScreen();

  // Wi-Fi接続が切れたら再接続を行う
  reconnectWifi();
}