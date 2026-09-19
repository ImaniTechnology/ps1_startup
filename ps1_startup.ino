// @file      ps1_startup.ino
// @brief     Playstation logo startup
// @author    ImaniTechnology
// @date      2026-19-09

#include <SPI.h>
#include "images.h"

#define TFT_CS    10
#define TFT_RST   8
#define TFT_DC    9
#define TFT_MOSI  11
#define TFT_SCLK  12
#define TFT_BL    18

#define TFT_WIDTH  160
#define TFT_HEIGHT 80
#define PIXELS     (TFT_WIDTH * TFT_HEIGHT)

#define ST77XX_SWRESET 0x01
#define ST77XX_SLPOUT  0x11
#define ST77XX_NORON   0x13
#define ST77XX_INVOFF  0x20
#define ST77XX_INVON   0x21
#define ST77XX_DISPON  0x29
#define ST77XX_CASET   0x2A
#define ST77XX_RASET   0x2B
#define ST77XX_RAMWR   0x2C
#define ST77XX_MADCTL  0x36
#define ST77XX_COLMOD  0x3A

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5
#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

SPISettings tftSPI(80000000, MSBFIRST, SPI_MODE0);

uint8_t frameBufferA[PIXELS * 2];
uint8_t frameBufferB[PIXELS * 2];

uint8_t* prepareBuffer = frameBufferA;
uint8_t* drawBuffer = frameBufferB;

TaskHandle_t TaskCore0;
SemaphoreHandle_t semFrameReady;
SemaphoreHandle_t semStartPrepare;

volatile int currentFrameIdx = 0;

void TFT_Select() {
  digitalWrite(TFT_CS, LOW);
}

void TFT_Deselect() {
  digitalWrite(TFT_CS, HIGH);
}

void TFT_Command(uint8_t cmd) {
  digitalWrite(TFT_DC, LOW);
  SPI.transfer(cmd);
}

void TFT_Data(uint8_t data) {
  digitalWrite(TFT_DC, HIGH);
  SPI.transfer(data);
}

void TFT_DataBuffer(const uint8_t* data, size_t len) {
  digitalWrite(TFT_DC, HIGH);
  SPI.writeBytes(data, len);
}

void TFT_WriteCommand(uint8_t cmd, const uint8_t* data, uint8_t len) {
  TFT_Select();
  TFT_Command(cmd);
  if (len > 0) TFT_DataBuffer(data, len);
  TFT_Deselect();
}

void TFT_WriteCommand(uint8_t cmd) {
  TFT_Select();
  TFT_Command(cmd);
  TFT_Deselect();
}

void TFT_Init() {
  pinMode(TFT_CS, OUTPUT);
  pinMode(TFT_DC, OUTPUT);
  pinMode(TFT_RST, OUTPUT);

  digitalWrite(TFT_CS, HIGH);
  digitalWrite(TFT_DC, HIGH);

  digitalWrite(TFT_RST, HIGH);
  delay(10);
  digitalWrite(TFT_RST, LOW);
  delay(20);
  digitalWrite(TFT_RST, HIGH);
  delay(150);

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  SPI.beginTransaction(tftSPI);

  TFT_Select();
  TFT_Command(ST77XX_SWRESET);
  TFT_Deselect();
  delay(150);

  TFT_Select();
  TFT_Command(ST77XX_SLPOUT);
  TFT_Deselect();
  delay(255);

  uint8_t colmod = 0x05;
  TFT_WriteCommand(ST77XX_COLMOD, &colmod, 1);
  delay(10);

  uint8_t frmctr1[] = {0x01, 0x2C, 0x2D};
  TFT_WriteCommand(ST7735_FRMCTR1, frmctr1, 3);

  uint8_t frmctr2[] = {0x01, 0x2C, 0x2D};
  TFT_WriteCommand(ST7735_FRMCTR2, frmctr2, 3);

  uint8_t frmctr3[] = {0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D};
  TFT_WriteCommand(ST7735_FRMCTR3, frmctr3, 6);

  uint8_t invctr = 0x07;
  TFT_WriteCommand(ST7735_INVCTR, &invctr, 1);

  uint8_t pwctr1[] = {0xA2, 0x02, 0x84};
  TFT_WriteCommand(ST7735_PWCTR1, pwctr1, 3);

  uint8_t pwctr2[] = {0xC5};
  TFT_WriteCommand(ST7735_PWCTR2, pwctr2, 1);

  uint8_t pwctr3[] = {0x0A, 0x00};
  TFT_WriteCommand(ST7735_PWCTR3, pwctr3, 2);

  uint8_t pwctr4[] = {0x8A, 0x2A};
  TFT_WriteCommand(ST7735_PWCTR4, pwctr4, 2);

  uint8_t pwctr5[] = {0x8A, 0xEE};
  TFT_WriteCommand(ST7735_PWCTR5, pwctr5, 2);

  uint8_t vmctr1 = 0x0E;
  TFT_WriteCommand(ST7735_VMCTR1, &vmctr1, 1);

  uint8_t caset[] = {0x00, 0x00, 0x00, 0x4F};
  TFT_WriteCommand(ST77XX_CASET, caset, 4);

  uint8_t raset[] = {0x00, 0x00, 0x00, 0x9F};
  TFT_WriteCommand(ST77XX_RASET, raset, 4);

  uint8_t gmctrp1[] = {
    0x02, 0x1C, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2D,
    0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10
  };
  TFT_WriteCommand(ST7735_GMCTRP1, gmctrp1, 16);

  uint8_t gmctrn1[] = {
    0x03, 0x1D, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D,
    0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10
  };
  TFT_WriteCommand(ST7735_GMCTRN1, gmctrn1, 16);

  uint8_t madctl = 0xA0;
  TFT_WriteCommand(ST77XX_MADCTL, &madctl, 1);

  TFT_WriteCommand(ST77XX_INVOFF);
  TFT_WriteCommand(ST77XX_NORON);
  delay(10);
  TFT_WriteCommand(ST77XX_DISPON);
  delay(100);

  TFT_Deselect();
  SPI.endTransaction();
}

void TFT_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  y0 += 24;
  y1 += 24;

  uint8_t data[4];

  data[0] = x0 >> 8;
  data[1] = x0 & 0xFF;
  data[2] = x1 >> 8;
  data[3] = x1 & 0xFF;
  TFT_Command(ST77XX_CASET);
  TFT_DataBuffer(data, 4);

  data[0] = y0 >> 8;
  data[1] = y0 & 0xFF;
  data[2] = y1 >> 8;
  data[3] = y1 & 0xFF;
  TFT_Command(ST77XX_RASET);
  TFT_DataBuffer(data, 4);

  TFT_Command(ST77XX_RAMWR);
}

void TFT_DrawFrame(uint8_t* buffer) {
  SPI.beginTransaction(tftSPI);
  TFT_Select();
  TFT_SetAddressWindow(0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
  TFT_DataBuffer(buffer, PIXELS * 2);
  TFT_Deselect();
  SPI.endTransaction();
}

void Core0Worker(void* pvParameters) {
  for (;;) {
    xSemaphoreTake(semStartPrepare, portMAX_DELAY);

    const uint16_t* rawFrame = frames[currentFrameIdx];

    for (int y = 0; y < TFT_HEIGHT; y++) {
      int lineOffset = y * TFT_WIDTH;

      for (int x = 0; x < TFT_WIDTH; x++) {
        uint16_t pixel = pgm_read_word(&(rawFrame[lineOffset + x]));

        uint16_t corrected =
          ((pixel & 0x001F) << 11) |
          (pixel & 0x07E0) |
          ((pixel & 0xF800) >> 11);

        int bufferIndex = (lineOffset + x) * 2;

        prepareBuffer[bufferIndex] = corrected >> 8;
        prepareBuffer[bufferIndex + 1] = corrected & 0xFF;
      }
    }

    xSemaphoreGive(semFrameReady);
  }
}

void setup() {
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  TFT_Init();

  semFrameReady = xSemaphoreCreateBinary();
  semStartPrepare = xSemaphoreCreateBinary();

  xTaskCreatePinnedToCore(
    Core0Worker,
    "FramePrep",
    8192,
    NULL,
    1,
    &TaskCore0,
    0
  );

  xSemaphoreGive(semStartPrepare);
}
void loop() {
  static uint32_t nextFrameTime = micros();

  xSemaphoreTake(semFrameReady, portMAX_DELAY);

  uint8_t* temp = prepareBuffer;
  prepareBuffer = drawBuffer;
  drawBuffer = temp;

  int frameThatWasDrawn = currentFrameIdx;

  currentFrameIdx = (currentFrameIdx + 1) % TOTAL_FRAMES;
  xSemaphoreGive(semStartPrepare);

  TFT_DrawFrame(drawBuffer);
  if (frameThatWasDrawn == 146) {
    delay(10000); 
    nextFrameTime = micros(); 
  }
  nextFrameTime += 66666;

  int32_t waitTime = (int32_t)(nextFrameTime - micros());

  if (waitTime > 0) {
    delayMicroseconds(waitTime);
  } else {
    nextFrameTime = micros();
  }
  
}