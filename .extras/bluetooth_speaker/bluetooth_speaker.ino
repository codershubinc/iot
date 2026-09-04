#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

AnalogAudioStream out;
BluetoothA2DPSink a2dp_sink(out);

void setup()
{
    Serial.begin(115200);
    Serial.println("Starting Bluetooth Speaker...");

    // Mix both Left and Right channels into a single Mono signal
    a2dp_sink.set_mono_downmix(true);

    // Start the Bluetooth device
    a2dp_sink.start("ESP32_Boombox");
}

void loop()
{
    delay(100);
}
