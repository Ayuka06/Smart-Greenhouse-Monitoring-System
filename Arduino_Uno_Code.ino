#include <LiquidCrystal.h>

// --- Pin Definitions ---
#define LDR_PIN A1
#define LED_PIN 10      
#define TRIG_PIN 6
#define ECHO_PIN 7

// --- Thresholds ---
#define LDR_THRESHOLD 500 
#define TANK_EMPTY_CM 40.0    // Distance in cm that means the tank is empty

// --- Object Initializations ---
// Assuming a 16x4 LCD based on previous designs. (Works on 16x2 as well)
LiquidCrystal lcd(12, 11, 5, 4, 3, 2); 

// --- State Variables ---
bool currentlyDark = false;
bool isTankEmpty = false;
float waterDistance = 0.0;

// --- Timers ---
unsigned long previousFastMillis = 0;
const long fastInterval = 200;   // 200ms updates for LDR and LED
unsigned long previousSlowMillis = 0;
const long slowInterval = 2000;  // 2s updates for Ultrasonic to prevent echoes

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  lcd.begin(16, 4);      
  
  // Hardware Serial to send data to the Mega
  Serial.begin(9600);    

  lcd.setCursor(0, 0);
  lcd.print("SMART GREENHOUSE");
  lcd.setCursor(0, 1);
  lcd.print("SYSTEM STARTING");
  delay(2000);
  lcd.clear();
}

void loop() {
  unsigned long currentMillis = millis();

  // ==========================================
  // TASK 1: SLOW SENSORS (Ultrasonic)
  // ==========================================
  if (currentMillis - previousSlowMillis >= slowInterval) {
    previousSlowMillis = currentMillis;

    // Send a 10-microsecond pulse to trigger the sensor
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    // Read the echo bounce time
    long duration = pulseIn(ECHO_PIN, HIGH);
    
    // Calculate distance (Speed of sound is 0.034 cm/us)
    waterDistance = duration * 0.034 / 2; 
    
    // Determine if tank is empty
    isTankEmpty = (waterDistance >= TANK_EMPTY_CM);
  }

  // ==========================================
  // TASK 2: FAST SENSORS, LOGIC & LCD (200ms)
  // ==========================================
  if (currentMillis - previousFastMillis >= fastInterval) {
    previousFastMillis = currentMillis;

    // 1. Read LDR
    int ldrValue = analogRead(LDR_PIN);
    currentlyDark = (ldrValue < LDR_THRESHOLD);

    // 2. Control LED (PWM Dimming)
    int ledBrightness = 0;
    if (currentlyDark) {
      ledBrightness = map(ldrValue, LDR_THRESHOLD, 0, 50, 255);
      ledBrightness = constrain(ledBrightness, 0, 255);
    } 
    analogWrite(LED_PIN, ledBrightness);

    // 3. Update LCD
    updateLCD(ldrValue, currentlyDark, ledBrightness, isTankEmpty, waterDistance);

    // 4. Send Data Packet to Mega!
    // Format: "DarkStatus,TankEmptyStatus\n" (e.g., "1,0")
    Serial.print(currentlyDark ? 1 : 0);
    Serial.print(",");
    Serial.println(isTankEmpty ? 1 : 0);
  }
}

// ==========================================
// CUSTOM FUNCTIONS
// ==========================================

void updateLCD(int ldrRaw, bool isDark, int brightness, bool empty, float distance) {
  // Row 1: Light Status
  lcd.setCursor(0, 0);
  lcd.print("LIGHT: ");
  lcd.print(isDark ? "DARK  " : "BRIGHT");

  // Row 2: LED Dimming %
  lcd.setCursor(0, 1);
  lcd.print("LED PWR: ");
  int brightPct = map(brightness, 0, 255, 0, 100);
  if(brightPct < 100) lcd.print(" ");
  if(brightPct < 10)  lcd.print(" ");
  lcd.print(brightPct);
  lcd.print("%  "); 

  // Row 3: Tank Status
  lcd.setCursor(0, 2);
  lcd.print("TANK: ");
  lcd.print(empty ? "EMPTY! " : "FULL     ");

  // Row 4: Raw Water Distance
  lcd.setCursor(0, 3);
  lcd.print("DIST: ");
  lcd.print(distance, 1);
  lcd.print("CM   "); // Padding to clear old numbers
}