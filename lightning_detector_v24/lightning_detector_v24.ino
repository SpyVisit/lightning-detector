// Lightning detector 2.4
// Board: TTGO-WEMOS-ESP32-OLED
// Sensor: WCMCU-3935 (AS3935) on I2C
// to upload: press+hold [boot] click [reset] release [boot]
//
// Wiring:
//   OLED  : SDA=GPIO5, SCL=GPIO4, RST=GPIO16 (built-in)
//   AS3935: SDA=GPIO5, SCL=GPIO4, IRQ=GPIO13
//   AS3935: VCC+SI+A0+A1+EN_V → 3.3V | GND+CS → GND
//
// Libraries: Adafruit SSD1306, Adafruit GFX (install via Library Manager)
// AS3935: direct Wire register access, no external library needed

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>      // для отключения WiFi/BT

// ===== OLED =====
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_SDA      5
#define OLED_SCL      4
#define OLED_RST      16
#define OLED_ADDRESS  0x3C

// ===== AS3935 =====
#define AS3935_ADDR   0x03
#define IRQ_PIN       13

// Регистры AS3935
#define REG_AFE_GB    0x00  // AFE gain, power down
#define REG_NF_LEV    0x01  // Noise floor level, watchdog
#define REG_STAT      0x02  // Statistics, spike rejection
#define REG_INT       0x03  // Interrupt source
#define REG_ENERGY_L  0x04  // Energy LSB
#define REG_ENERGY_M  0x05  // Energy MSB
#define REG_ENERGY_MM 0x06  // Energy MMSB
#define REG_DISTANCE  0x07  // Distance estimation
#define REG_TUNING    0x08  // Antenna tuning, display bits
#define REG_RESET     0x3C  // PRESET_DEFAULT command
#define REG_CALIB     0x3D  // CALIB_RCO command

// Прерывания
#define INT_NH        0x01  // Noise too high
#define INT_DIST      0x04  // Disturber detected
#define INT_LIGHT     0x08  // Lightning detected

// Таймауты
#define LIGHTNING_TIMEOUT  10000  // 10 сек после молнии → Quiet
#define DISTURBER_TIMEOUT   5000  //  5 сек после помехи/шума → Quiet

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

// Глобальные переменные
volatile bool irqTriggered     = false;
int lastDistance               = 0;
long lastEnergy                = 0;
uint8_t noiseLevel             = 0;    // уровень шума 0-7
uint16_t disturberCount        = 0;    // счётчик помех за сессию
String lastStatus              = "Waiting...";
unsigned long lightningTimer   = 0;
unsigned long disturberTimer   = 0;
bool hadLightning              = false;
bool hadDisturber              = false;

// ===== AS3935 функции =====

void as3935Write(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(AS3935_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission(false);
}

uint8_t as3935Read(uint8_t reg) {
  Wire.beginTransmission(AS3935_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(AS3935_ADDR, 1);
  return Wire.available() ? Wire.read() : 0;
}

void as3935WriteField(uint8_t reg, uint8_t mask, uint8_t shift, uint8_t value) {
  uint8_t current = as3935Read(reg);
  current &= ~mask;
  current |= (value << shift) & mask;
  as3935Write(reg, current);
}

uint8_t as3935GetNoiseLevel() {
  // REG0x01[6:4] — текущий уровень шума 0-7
  return (as3935Read(REG_NF_LEV) >> 4) & 0x07;
}

void as3935Init() {
  Serial.println("AS3935 init...");

  // 1. Сброс всех регистров в заводские значения
  Wire.beginTransmission(AS3935_ADDR);
  Wire.write(REG_RESET);
  Wire.write(0x96);
  Wire.endTransmission(false);
  delay(2);

  // 2. Включить чип (PWD=0)
  as3935WriteField(REG_AFE_GB, 0x01, 0, 0);

  // 3. Калибровка RC генераторов
  Wire.beginTransmission(AS3935_ADDR);
  Wire.write(REG_CALIB);
  Wire.write(0x96);
  Wire.endTransmission(false);
  delay(2);

  // 4. DISP_SRCO=1 → delay → DISP_SRCO=0 (завершение калибровки SRCO)
  as3935WriteField(REG_TUNING, 0x20, 5, 1);
  delay(2);
  as3935WriteField(REG_TUNING, 0x20, 5, 0);

  // 5. Indoor режим (AFE_GB = 10010 = 0x12)
  //    Для улицы: as3935WriteField(REG_AFE_GB, 0x3E, 1, 0x0E);
  as3935WriteField(REG_AFE_GB, 0x3E, 1, 0x12);

  // 6. Уровень шума по умолчанию (NF_LEV = 010)
  as3935WriteField(REG_NF_LEV, 0x70, 4, 0x02);

  // 7. Watchdog threshold (WDTH = 0010)
  as3935WriteField(REG_NF_LEV, 0x0F, 0, 0x02);

  // 8. Spike rejection (SREJ = 0010)
  as3935WriteField(REG_STAT, 0x0F, 0, 0x02);

  // 9. Минимум 1 молния для срабатывания
  as3935WriteField(REG_STAT, 0x30, 4, 0x00);

  // 10. Очистить статистику (high → low → high)
  as3935WriteField(REG_STAT, 0x40, 6, 1);
  as3935WriteField(REG_STAT, 0x40, 6, 0);
  as3935WriteField(REG_STAT, 0x40, 6, 1);

  delay(100);

  // Вывод регистров для диагностики
  Serial.println("AS3935 initialized OK");
  Serial.print("REG0x00: 0x"); Serial.println(as3935Read(0x00), HEX);
  Serial.print("REG0x01: 0x"); Serial.println(as3935Read(0x01), HEX);
  Serial.print("REG0x02: 0x"); Serial.println(as3935Read(0x02), HEX);
  Serial.print("REG0x08: 0x"); Serial.println(as3935Read(0x08), HEX);
}

int as3935GetDistance() {
  return as3935Read(REG_DISTANCE) & 0x3F;
}

long as3935GetEnergy() {
  long l  = as3935Read(REG_ENERGY_L);
  long m  = as3935Read(REG_ENERGY_M);
  long mm = as3935Read(REG_ENERGY_MM) & 0x1F;
  return (mm << 16) | (m << 8) | l;
}

uint8_t as3935GetInterrupt() {
  delay(10);  // минимум 2мс по даташиту, 10мс для надёжности
  return as3935Read(REG_INT) & 0x0F;
}

// ===== IRQ обработчик =====
void IRAM_ATTR as3935IRQ() {
  irqTriggered = true;
}

// ===== OLED =====
void drawInterface() {
  display.clearDisplay();

  // Заголовок
  display.setTextSize(1);
  display.setCursor(15, 0);
  display.println("Lightning Detector");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // Расстояние (крупно)
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.print("Dist:");
  display.setTextSize(2);
  display.setCursor(40, 18);
  if      (lastDistance == 63) display.print("-- km");
  else if (lastDistance == 1)  display.print("HERE!");
  else if (lastDistance == 0)  display.print("---");
  else { display.print(lastDistance); display.print(" km"); }

  display.drawLine(0, 36, 127, 36, SSD1306_WHITE);

  // Адаптивная строка Info
  display.setTextSize(1);
  display.setCursor(0, 40);
  if (lastStatus == "Noise!") {
    // Показываем уровень шума
    display.print("Noise lvl: ");
    display.print(noiseLevel);
    display.print("/7");
  } else if (lastStatus == "Disturber") {
    // Показываем счётчик помех
    display.print("Disturb:   ");
    display.print(disturberCount);
    display.print("x");
  } else {
    // Quiet или Lightning — показываем энергию
    display.print("Energy: ");
    display.print(lastEnergy);
  }

  // Статус
  display.setCursor(0, 52);
  display.print("Status: ");
  display.print(lastStatus);

  display.display();
}

void goQuiet() {
  lastStatus   = "Quiet";
  lastDistance = 0;
  lastEnergy   = 0;
  noiseLevel   = 0;
  // disturberCount НЕ сбрасываем — это счётчик всей сессии
  hadLightning = false;
  hadDisturber = false;
  drawInterface();
  Serial.println("Back to Quiet");
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(500);

  // Отключение WiFi и Bluetooth для снижения помех на AS3935
  // Раскомментировать если датчик шумит без физического разноса
  // WiFi.mode(WIFI_OFF);
  // btStop();

  Serial.println("Lightning Detector 2.4 starting...");

  // Инициализация OLED
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);  // без этого OLED не стартует!

  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED not found!");
    while (true);
  }
  display.setRotation(0);  // 0=normal, 2=180 градусов

  // Заставка
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(15, 20);
  display.println("Lightning Detector");
  display.setCursor(30, 35);
  display.println("Initializing...");
  display.display();
  delay(1000);

  // Инициализация AS3935
  as3935Init();

  // Подключение прерывания
  pinMode(IRQ_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(IRQ_PIN), as3935IRQ, RISING);

  lastStatus = "Quiet";
  drawInterface();
  Serial.println("Ready. Waiting for lightning...");
}

// ===== LOOP =====
void loop() {

  unsigned long now = millis();

  // --- Таймаут молнии (10 сек) ---
  if (hadLightning && (now - lightningTimer > LIGHTNING_TIMEOUT)) {
    hadLightning = false;
    if (!hadDisturber) goQuiet();
  }

  // --- Таймаут помехи/шума (5 сек) ---
  if (hadDisturber && (now - disturberTimer > DISTURBER_TIMEOUT)) {
    hadDisturber = false;
    if (!hadLightning) goQuiet();
  }

  // --- Обработка прерывания от AS3935 ---
  if (irqTriggered) {
    irqTriggered = false;

    uint8_t intSrc = as3935GetInterrupt();

    if (intSrc & INT_LIGHT) {
      // Молния обнаружена
      lastDistance   = as3935GetDistance();
      lastEnergy     = as3935GetEnergy();
      lastStatus     = "Lightning!";
      lightningTimer = millis();
      hadLightning   = true;

      Serial.print("LIGHTNING! Dist: ");
      if (lastDistance == 63)     Serial.println("Out of range");
      else if (lastDistance == 1) Serial.println("Overhead!");
      else { Serial.print(lastDistance); Serial.println(" km"); }
      Serial.print("Energy: "); Serial.println(lastEnergy);

    } else if (intSrc & INT_DIST) {
      // Помеха обнаружена
      disturberCount++;
      lastStatus     = "Disturber";
      disturberTimer = millis();
      hadDisturber   = true;
      Serial.print("Disturber #"); Serial.println(disturberCount);

    } else if (intSrc & INT_NH) {
      // Уровень шума слишком высокий
      noiseLevel     = as3935GetNoiseLevel();
      lastStatus     = "Noise!";
      disturberTimer = millis();
      hadDisturber   = true;
      Serial.print("Noise level: ");
      Serial.print(noiseLevel);
      Serial.println("/7");

    } else {
      // Ложное прерывание — игнорируем
      Serial.println("Spurious IRQ ignored");
    }

    drawInterface();
  }
}
