#include <Arduino.h>
#include <driver/i2s.h>
#include "LissenceBlePeripheral.h"

namespace {

constexpr i2s_port_t MicI2SPort = I2S_NUM_0;
constexpr int MicSckPin = 1;
constexpr int MicWsPin = 2;
constexpr int MicSdPin = 3;
constexpr uint32_t MicSampleRate = 16000;
constexpr size_t MicSampleCount = 256;
constexpr uint32_t MicPrintIntervalMs = 500;

uint32_t lastMicPrintMillis = 0;
bool isMicReady = false;

void beginMicrophone() {
  const i2s_config_t i2sConfig = {
      .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = MicSampleRate,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 4,
      .dma_buf_len = 256,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0,
      .mclk_multiple = I2S_MCLK_MULTIPLE_DEFAULT,
      .bits_per_chan = I2S_BITS_PER_CHAN_DEFAULT,
  };

  const i2s_pin_config_t pinConfig = {
      .mck_io_num = I2S_PIN_NO_CHANGE,
      .bck_io_num = MicSckPin,
      .ws_io_num = MicWsPin,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = MicSdPin,
  };

  esp_err_t result = i2s_driver_install(MicI2SPort, &i2sConfig, 0, nullptr);
  if (result != ESP_OK) {
    Serial.print("[MIC] i2s_driver_install failed: ");
    Serial.println(esp_err_to_name(result));
    return;
  }

  result = i2s_set_pin(MicI2SPort, &pinConfig);
  if (result != ESP_OK) {
    Serial.print("[MIC] i2s_set_pin failed: ");
    Serial.println(esp_err_to_name(result));
    i2s_driver_uninstall(MicI2SPort);
    return;
  }

  i2s_zero_dma_buffer(MicI2SPort);
  isMicReady = true;

  Serial.println("[MIC] INMP441 I2S microphone ready");
  Serial.print("[MIC] SCK GPIO");
  Serial.print(MicSckPin);
  Serial.print(", WS GPIO");
  Serial.print(MicWsPin);
  Serial.print(", SD GPIO");
  Serial.println(MicSdPin);
}

void printMicrophoneLevels() {
  if (!isMicReady) {
    return;
  }

  const uint32_t now = millis();
  if (now - lastMicPrintMillis < MicPrintIntervalMs) {
    return;
  }

  lastMicPrintMillis = now;

  int32_t samples[MicSampleCount] = {};
  size_t bytesRead = 0;
  const esp_err_t result = i2s_read(
      MicI2SPort,
      samples,
      sizeof(samples),
      &bytesRead,
      pdMS_TO_TICKS(20));

  if (result != ESP_OK || bytesRead == 0) {
    Serial.print("[MIC] i2s_read failed: ");
    Serial.println(esp_err_to_name(result));
    return;
  }

  const size_t sampleCount = bytesRead / sizeof(samples[0]);
  int64_t sumSquares = 0;
  int32_t peak = 0;

  for (size_t i = 0; i < sampleCount; ++i) {
    const int32_t sample = samples[i] >> 8;
    const int32_t magnitude = abs(sample);
    peak = max(peak, magnitude);
    sumSquares += static_cast<int64_t>(sample) * sample;
  }

  const int32_t rms = static_cast<int32_t>(sqrt(static_cast<double>(sumSquares) / sampleCount));

  Serial.print("Mic RMS: ");
  Serial.print(rms);
  Serial.print(", Peak: ");
  Serial.println(peak);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("Lissence ESP32-C3 BLE + INMP441 microphone test boot");

  LissenceBlePeripheral::begin();
  beginMicrophone();
}

void loop() {
  LissenceBlePeripheral::loop();
  printMicrophoneLevels();
}
