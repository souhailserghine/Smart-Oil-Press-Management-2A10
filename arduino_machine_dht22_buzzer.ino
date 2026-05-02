#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT22
#define BUZZER_PIN 7

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.println("Systeme temperature machine");
  Serial.println("< 30 C = bip normal | >= 30 C = bip accelere | >= 40 C = son continu");
}

void loop() {
  float t = dht.readTemperature();

  if (isnan(t)) {
    Serial.println("TEMP:ERR;STATE:ERR");
    noTone(BUZZER_PIN);
    delay(1000);
    return;
  }

  String state = "NORMAL";

  if (t >= 40.0) {
    state = "DANGER";
    Serial.print("TEMP:");
    Serial.print(t);
    Serial.println(";STATE:DANGER");
    tone(BUZZER_PIN, 1500);   // son continu
    delay(500);
  }
  else if (t >= 30.0) {
    state = "ALERTE";
    Serial.print("TEMP:");
    Serial.print(t);
    Serial.println(";STATE:ALERTE");
    tone(BUZZER_PIN, 1500, 120); // bip accelere
    delay(220);
  }
  else {
    state = "NORMAL";
    Serial.print("TEMP:");
    Serial.print(t);
    Serial.println(";STATE:NORMAL");
    tone(BUZZER_PIN, 1200, 180); // bip normal
    delay(1000);
  }
}
