#define FPS 30
#define MJPEG_BUFFER_SIZE (288 * 240 * 2 / 8)
#define AUDIOASSIGNCORE 1
#define DECODEASSIGNCORE 0
#define DRAWASSIGNCORE 1

#include <WiFi.h>
#include <FS.h>
#include <SD_MMC.h>
#include <Preferences.h>

Preferences preferences;
#define APP_NAME "video_player"
#define K_VIDEO_INDEX "video_index"
#define BASE_PATH "/Videos/"
#define AAC_FILENAME "/44100.aac"
#define MJPEG_FILENAME "/288_30fps.mjpeg"
#define VIDEO_COUNT 20

#define SDMMC_D2 2
#define SDMMC_D3 3
#define SDMMC_CMD 4
#define SDMMC_CLK 5
#define SDMMC_D0 0
#define SDMMC_D1 1

#define I2S_DOUT 7
#define I2S_BCLK 13
#define I2S_LRC 14

#define I2C_SDA 8
#define I2C_SCL 9
#define TP_INT 38
#define TP_RST 39

/* Audio */
#include "esp32_audio_task.h"

/* MJPEG Video */
#include "mjpeg_decode_draw_task.h"

/* LCD */
#include "LovyanGFX_Driver.h"
LGFX tft;

/* Touch */
#include "CST816T.h"
CST816T touch(I2C_SDA, I2C_SCL,TP_RST,TP_INT); 

/* Variables */
static int next_frame = 0;
static int skipped_frames = 0;
static unsigned long start_ms, curr_ms, next_frame_ms;
static unsigned int video_idx = 1;

// pixel drawing callback
static int drawMCU(JPEGDRAW *pDraw) {
  unsigned long s = millis();
  if (tft.getStartCount() == 0) {
    tft.endWrite();
  }
  tft.pushImageDMA(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, (lgfx::rgb565_t *)pDraw->pPixels);
  total_show_video_ms += millis() - s;
  return 1;
} /* drawMCU() */

void setup() {
  disableCore0WDT();

  WiFi.mode(WIFI_OFF);
  Serial.begin(115200);

  // Init Display
  tft.init();
  tft.initDMA();
  tft.startWrite();
  tft.fillScreen(TFT_BLACK);

  xTaskCreate(
    touchTask,
    "touchTask",
    2000,
    NULL,
    1,
    NULL);

  Serial.println("Init I2S");

  esp_err_t ret_val = i2s_init(I2S_NUM_0, 44100, -1, I2S_BCLK, I2S_LRC, I2S_DOUT, -1);
  if (ret_val != ESP_OK) {
    Serial.printf("i2s_init failed: %d\n", ret_val);
    tft.println("i2s_init failed");
    return;
  }
  i2s_zero_dma_buffer(I2S_NUM_0);

  Serial.println("Init FS");

  SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_D0, SDMMC_D1, SDMMC_D2, SDMMC_D3);
  if (!SD_MMC.begin("/root")) /* 4-bit SD bus mode */
  {
    Serial.println("ERROR: File system mount failed!");
    tft.println("ERROR: File system mount failed!");
    return;
  }

  preferences.begin(APP_NAME, false);
  video_idx = preferences.getUInt(K_VIDEO_INDEX, 1);
  Serial.printf("videoIndex: %d\n", video_idx);

  tft.setCursor(20, 20);
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(3);
  tft.printf("CH %d", video_idx);
  delay(1000);

  playVideoWithAudio(video_idx);
}

void loop() {
}

void touchTask(void *parameter) {
  touch.begin();
  bool FingerNum;
  uint8_t gesture;
  uint16_t touchX,touchY;  
  while (1) {
    FingerNum=touch.getTouch(&touchX,&touchY,&gesture);  
    if (FingerNum) {
      switch (gesture) {
        case None:
          Serial.println("None");
          break;
        case SlideUp:
          Serial.println("SlideUp(RIGHT)");
          videoController(-1);
          break;
        case SlideDown:
          Serial.println("SlideDown(LEFT)");
          videoController(1);
          break;
      }
    }
    vTaskDelay(1000);
  }
}

void playVideoWithAudio(int channel) {

  char aFilePath[40];
  sprintf(aFilePath, "%s%d%s", BASE_PATH, channel, AAC_FILENAME);

  File aFile = SD_MMC.open(aFilePath);
  if (!aFile || aFile.isDirectory()) {
    Serial.printf("ERROR: Failed to open %s file for reading\n", aFilePath);
    tft.printf("ERROR: Failed to open %s file for reading\n", aFilePath);
    return;
  }

  char vFilePath[40];
  sprintf(vFilePath, "%s%d%s", BASE_PATH, channel, MJPEG_FILENAME);

  File vFile = SD_MMC.open(vFilePath);
  if (!vFile || vFile.isDirectory()) {
    Serial.printf("ERROR: Failed to open %s file for reading\n", vFilePath);
    tft.printf("ERROR: Failed to open %s file for reading\n", vFilePath);
    return;
  }

  Serial.println("Init video");

  mjpeg_setup(&vFile, MJPEG_BUFFER_SIZE, drawMCU, false, DECODEASSIGNCORE, DRAWASSIGNCORE);

  Serial.println("Start play audio task");

  BaseType_t ret = aac_player_task_start(&aFile, AUDIOASSIGNCORE);

  if (ret != pdPASS) {
    Serial.printf("Audio player task start failed: %d\n", ret);
    tft.printf("Audio player task start failed: %d\n", ret);
  }

  Serial.println("Start play video");

  start_ms = millis();
  curr_ms = millis();
  next_frame_ms = start_ms + (++next_frame * 1000 / FPS / 2);
  while (vFile.available() && mjpeg_read_frame())  // Read video
  {
    total_read_video_ms += millis() - curr_ms;
    curr_ms = millis();

    if (millis() < next_frame_ms)  // check show frame or skip frame
    {
      // Play video
      mjpeg_draw_frame();
      total_decode_video_ms += millis() - curr_ms;
      curr_ms = millis();
    } else {
      ++skipped_frames;
      //Serial.println("Skip frame");
    }

    while (millis() < next_frame_ms) {
      vTaskDelay(pdMS_TO_TICKS(1));
    }

    curr_ms = millis();
    next_frame_ms = start_ms + (++next_frame * 1000 / FPS);
  }
  int time_used = millis() - start_ms;
  int total_frames = next_frame - 1;
  Serial.println("AV end");
  vFile.close();
  aFile.close();

  videoController(1);
}

void videoController(int next) {

  video_idx += next;
  if (video_idx <= 0) {
    video_idx = VIDEO_COUNT;
  } else if (video_idx > VIDEO_COUNT) {
    video_idx = 1;
  }
  Serial.printf("video_idx : %d\n", video_idx);
  preferences.putUInt(K_VIDEO_INDEX, video_idx);
  preferences.end();
  esp_restart();
}
