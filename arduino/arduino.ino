// Arduino Uno - Water Level Sensor + LED
// Sensor on A0, LED on pin 8
// Sends percentage over Serial for Qt integration

#define SENSOR_PIN A1
#define LED_PIN    8

// Calibration values - ADJUST THESE based on your sensor and cup!
const int DRY_VALUE   = 0;    // Analog reading when cup is completely empty (dry)
const int FULL_VALUE  = 650;  // Analog reading when cup is completely full (test !)

int percentage = 0;
bool isFull = false;

void setup() {
  pinMode(LED_PIN, OUTPUT); 
  digitalWrite(LED_PIN, LOW);   // LED starts OFF
  
  Serial.begin(9600);           // Important: Qt will read at this baud rate
  // Optional: Serial.println("Water Level Monitor Ready");
  
  // Give sensor a moment to stabilize
  delay(500);
}

void loop() {
  // Read the raw analog value from the water level sensor
  int sensorValue = analogRead(SENSOR_PIN);
  
  // Map the sensor value to a percentage (0-100%)
  // constrain() prevents values going below 0 or above 100
  percentage = map(sensorValue, DRY_VALUE, FULL_VALUE, 0, 100);
  percentage = constrain(percentage, 0, 100);
  
  // Decide if the cup is "full" (you can adjust the threshold, e.g. 90 or 95)
  isFull = (percentage >= 90);   // Change 90 to whatever % you consider "full"
  
  // Control the LED
  if (isFull) {
    digitalWrite(LED_PIN, HIGH);   // LED ON when full
  } else {
    digitalWrite(LED_PIN, LOW);    // LED OFF otherwise
  }
  
  // Send data to Qt in a simple, easy-to-parse format
  // Example output: "PERCENT:75\n"
  Serial.print("PERCENT:");
  Serial.println(percentage);
  
  // Small delay to avoid flooding the serial port
  delay(500);   // Update every 0.5 seconds - you can change this
}