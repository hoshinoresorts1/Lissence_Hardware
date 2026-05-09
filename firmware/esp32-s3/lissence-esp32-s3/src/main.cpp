#include <Arduino.h>
#include "LissenceBlePeripheral.h"
#if ENABLE_MICROPHONE || ENABLE_AUDIO_STREAMING
#include <driver/i2s.h>
#endif

namespace {

#if ENABLE_MICROPHONE || ENABLE_AUDIO_STREAMING
constexpr i2s_port_t MicI2SPort = I2S_NUM_0;
constexpr int MicSckPin = 26;
constexpr int MicWsPin = 25;
constexpr int MicSdPin = 33;
constexpr uint32_t MicSampleRate = 8000;
constexpr size_t MicSampleCount = 256;
constexpr uint32_t MicPrintIntervalMs = 500;
#endif

#if ENABLE_AUDIO_STREAMING
constexpr size_t AudioStreamSamplesPerChunk = 160;
constexpr size_t AudioStreamBytesPerChunk = AudioStreamSamplesPerChunk * sizeof(int16_t);
constexpr size_t AudioStreamPacketPayloadSize = 160;
constexpr uint32_t AudioStreamDurationMs = 3000;
#endif

#if ENABLE_MICROPHONE || ENABLE_AUDIO_STREAMING
uint32_t lastMicPrintMillis = 0;
#endif
#if ENABLE_AUDIO_STREAMING
uint32_t audioStreamStartedMillis = 0;
uint16_t audioStreamSequence = 0;
#endif
#if ENABLE_MICROPHONE || ENABLE_AUDIO_STREAMING
bool isMicReady = false;
#endif
#if ENABLE_AUDIO_STREAMING
bool isAudioStreaming = false;

int16_t convertToPcm16(int32_t sample) {
  const int32_t scaledSample = sample >> 8;
  if (scaledSample > INT16_MAX) {
    return INT16_MAX;
  }

  if (scaledSample < INT16_MIN) {
    return INT16_MIN;
  }

  return static_cast<int16_t>(scaledSample);
}
#endif

#if ENABLE_MICROPHONE || ENABLE_AUDIO_STREAMING
void beginMicrophone() {
  const i2s_config_t i2sConfig = {
      .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = MicSampleRate,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
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
#endif

#if ENABLE_MICROPHONE
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

  LissenceBlePeripheral::sendMicLevel(rms, peak);
}
#endif

#if ENABLE_AUDIO_STREAMING
void startAudioStream() {
  if (!isMicReady) {
    Serial.println("[AUDIO] Cannot start stream: microphone is not ready");
    return;
  }

  isAudioStreaming = true;
  LissenceBlePeripheral::setAudioStreamingActive(true);
  audioStreamStartedMillis = millis();
  audioStreamSequence = 0;
  Serial.println("[AUDIO] 3 second PCM stream started at 8kHz");
}

void stopAudioStream(const char* reason) {
  if (!isAudioStreaming) {
    return;
  }

  isAudioStreaming = false;
  LissenceBlePeripheral::setAudioStreamingActive(false);
  Serial.print("[AUDIO] PCM stream stopped: ");
  Serial.println(reason);
}

void processAudioStreamCommands() {
  if (LissenceBlePeripheral::consumeAudioStreamStartRequest()) {
    startAudioStream();
  }

  if (LissenceBlePeripheral::consumeAudioStreamStopRequest()) {
    stopAudioStream("write command");
  }
}

void sendAudioStreamChunk() {
  int32_t rawSamples[AudioStreamSamplesPerChunk] = {};
  int16_t pcmSamples[AudioStreamSamplesPerChunk] = {};
  size_t bytesRead = 0;

  const esp_err_t result = i2s_read(
      MicI2SPort,
      rawSamples,
      sizeof(rawSamples),
      &bytesRead,
      pdMS_TO_TICKS(30));

  if (result != ESP_OK || bytesRead == 0) {
    Serial.print("[AUDIO] i2s_read failed during stream: ");
    Serial.println(esp_err_to_name(result));
    stopAudioStream("i2s read failure");
    return;
  }

  const size_t sampleCount = min(bytesRead / sizeof(rawSamples[0]), AudioStreamSamplesPerChunk);
  for (size_t i = 0; i < sampleCount; ++i) {
    pcmSamples[i] = convertToPcm16(rawSamples[i]);
  }

  const size_t audioBytes = sampleCount * sizeof(pcmSamples[0]);
  const uint8_t packetCount = static_cast<uint8_t>(
      (audioBytes + AudioStreamPacketPayloadSize - 1) / AudioStreamPacketPayloadSize);
  const uint8_t* audioBytesPointer = reinterpret_cast<const uint8_t*>(pcmSamples);
  bool didDropPacket = false;

  for (uint8_t packetIndex = 0; packetIndex < packetCount; ++packetIndex) {
    const size_t offset = packetIndex * AudioStreamPacketPayloadSize;
    const uint16_t payloadSize = static_cast<uint16_t>(
        min(AudioStreamPacketPayloadSize, audioBytes - offset));
    const bool didNotify = LissenceBlePeripheral::sendAudioStreamPacket(
        audioStreamSequence,
        packetIndex,
        packetCount,
        audioBytesPointer + offset,
        payloadSize);

    didDropPacket = didDropPacket || !didNotify;
    vTaskDelay(pdMS_TO_TICKS(3));
  }

  if (audioStreamSequence % 25 == 0) {
    Serial.print("[AUDIO] Stream chunk seq=");
    Serial.print(audioStreamSequence);
    Serial.print(", bytes=");
    Serial.print(audioBytes);
    Serial.print(", packets=");
    Serial.print(packetCount);
    if (didDropPacket) {
      Serial.print(", notifyDrop=true");
    }
    Serial.println();
  }

  audioStreamSequence += 1;
}

void processAudioStream() {
  if (!isAudioStreaming) {
    return;
  }

  if (millis() - audioStreamStartedMillis >= AudioStreamDurationMs) {
    stopAudioStream("3 second limit");
    return;
  }

  sendAudioStreamChunk();
}
#endif

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("[BOOT] ESP32-WROOM-32E boot");
  Serial.println("[BOOT] BLE + INMP441 microphone mode");
  LissenceBlePeripheral::begin();
#if ENABLE_MICROPHONE || ENABLE_AUDIO_STREAMING
  beginMicrophone();
#endif
}

void loop() {
  LissenceBlePeripheral::loop();
#if ENABLE_AUDIO_STREAMING
  processAudioStreamCommands();
  processAudioStream();
#endif
#if ENABLE_MICROPHONE
  printMicrophoneLevels();
#endif
}
