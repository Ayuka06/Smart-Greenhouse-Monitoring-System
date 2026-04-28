#include <LiquidCrystal.h>
#include <DHT.h>

// --- Pin Definitions ---
#define DHTPIN 7
#define FAN_PIN 8
#define PUMP_PIN 9
#define PH_PIN A0

// --- Sensor Types & Thresholds ---
#define DHTTYPE DHT11
#define TEMP_THRESHOLD 24.0   
#define HUM_THRESHOLD 50.0    

// --- Object Initializations ---
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// --- Timer Variables ---
unsigned long previousMillis = 0;
const long interval = 2000; 

// SMS tracking 
unsigned long lastSmsTime = 0;
const long smsInterval = 3600000; 
bool smsSent = false;

// --- LCD State Tracking Variables ---
float prevTemp = -999.0;
float prevHum = -999.0;
float prevPh = -999.0;
int prevFan = -1; 
int prevPump = -1;

void setup() {
  pinMode(FAN_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);

  dht.begin();
  lcd.begin(16, 4); 
  
  Serial.begin(9600);    
  Serial1.begin(9600);   

  // Startup Message
  lcd.setCursor(0, 0);
  lcd.print("SMART GREENHOUSE");
  lcd.setCursor(0, 1);
  lcd.print("SYSTEM STARTING");
  delay(2000);
  lcd.clear();

  // --- DRAW STATIC LCD TEMPLATE ---
  lcd.setCursor(0, 0); lcd.print("T:   C  H:   %"); 
  lcd.setCursor(0, 1); lcd.print("SOIL PH: ");        
  lcd.setCursor(0, 2); lcd.print("FAN:    PUMP:");    
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // 1. Read Sensors
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    int phRawValue = analogRead(PH_PIN);
    float voltage = phRawValue * (5.0 / 1023.0);
    float phValue = 3.5 * voltage; 

    // --- Control Logic ---
    
    // Fan Control
    bool fanStatus = (temperature > TEMP_THRESHOLD);
    digitalWrite(FAN_PIN, fanStatus ? HIGH : LOW);

    // Pump Control
    bool pumpStatus = (humidity < HUM_THRESHOLD);
    digitalWrite(PUMP_PIN, pumpStatus ? HIGH : LOW);

    // --- GSM SMS Logic ---
    if ((phValue < 5.5 || phValue > 7.5) && (currentMillis - lastSmsTime >= smsInterval || lastSmsTime == 0)) {
      sendSMS(phValue);
      lastSmsTime = currentMillis;
    }

    // --- Update LCD Display ---
    updateLCD(temperature, humidity, phValue, fanStatus, pumpStatus);
  }
}

// --- HIGH SPEED LCD UPDATE FUNCTION ---
void updateLCD(float temp, float hum, float ph, bool fanON, bool pumpON) {
  
  if (temp != prevTemp && !isnan(temp)) {
    lcd.setCursor(2, 0);
    lcd.print(temp, 0);
    prevTemp = temp;
  }

  if (hum != prevHum && !isnan(hum)) {
    lcd.setCursor(10, 0);
    lcd.print(hum, 0);
    prevHum = hum;
  }

  if (ph != prevPh) {
    lcd.setCursor(9, 1);
    lcd.print(ph, 2);
    lcd.print(" "); 
    prevPh = ph;
  }

  if (fanON != prevFan) {
    lcd.setCursor(4, 2);
    lcd.print(fanON ? "ON " : "OFF");
    prevFan = fanON;
  }

  if (pumpON != prevPump) {
    lcd.setCursor(13, 2);
    lcd.print(pumpON ? "ON " : "OFF");
    prevPump = pumpON;
  }
}

void sendSMS(float currentPh) {
  Serial.println("Sending SMS Alert...");
  Serial1.println("AT+CMGF=1"); 
  delay(1000);
  Serial1.println("AT+CMGS=\"+1234567890\""); 
  delay(1000);
  Serial1.print("Greenhouse Alert! Soil pH is abnormal: ");
  Serial1.print(currentPh);
  delay(100);
  Serial1.write(26); 
  delay(1000);
  Serial.println("SMS Sent.");
}