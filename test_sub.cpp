#include <Arduino.h>
void setup() {
  String a = "Hello";
  String b = a.substring(0, 16);
  Serial.begin(115200);
  Serial.println(b);
}
void loop() {}
