// ============================================================
// BICYCLE ALARM WITH EDGE AI - FUNCTIONAL VERSION
// Arduino UNO R4 WiFi + Simulated Edge AI + ThingSpeak
// Does NOT require TensorFlow Lite
// ============================================================
#include "modelo_arduino.h"

#include <WiFiS3.h>

// ================= WIFI CONFIGURATION ======================
const char* ssid = "AndroidAP9943";
const char* password = "029dda3eaac8";

// ================= THINGSPEAK CONFIGURATION ================
const char* server = "api.thingspeak.com";
String apiKey = "NUZ6SBEE1L0PF5UU";

// ================= PIN CONFIGURATION ======================
const int sensorPin = 2;     // SW-420 Vibration Sensor (DIGITAL)
const int analogPin = A0;    // Analog input for intensity
const int buzzerPin = 8;     // Buzzer
const int ledPin = 13;       // Built-in LED
const int ledEdgeAI = 4;     // Additional LED for Edge AI indication

// ================= GLOBAL VARIABLES ======================
bool alarmActive = false;
bool wifiEnabled = true;
bool wifiConnected = false;
bool edgeAIEnabled = true;   // TRUE = Edge AI activated
bool debugMode = true;       // TRUE = See detailed messages

// Thresholds and timings
const unsigned long minimumTimeBetweenAlarms = 5000;  // 5 seconds
const int intensityThreshold = 215;  // Analog value (0-1023) to consider real event
unsigned long lastAlarm = 0;
unsigned long lastThingSpeakConnection = 0;
const unsigned long thingSpeakInterval = 30000;      // 30 seconds

// History for Edge AI
int vibrationHistory[5] = {0, 0, 0, 0, 0};
int historyIndex = 0;
unsigned long lastSensorReading = 0;
const unsigned long readingInterval = 50;            // 50ms

// Statistics
int totalEvents = 0;
int confirmedEvents = 0;
int falsePositives = 0;

WiFiClient client;

// ================= FUNCTION PROTOTYPES =================
bool isRealEvent(int vibDigital, int vibAnalog);
void connectWiFi();
void testSystem();
void activateAlarm();
void sendThingSpeak(bool confirmedByAI, float intensity);
void showStatistics();

// ======================= SETUP =============================
void setup() {
  // Configure pins
  pinMode(sensorPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(ledEdgeAI, OUTPUT);
  pinMode(analogPin, INPUT);
  
  // Ensure everything is off
  digitalWrite(buzzerPin, LOW);
  digitalWrite(ledPin, LOW);
  digitalWrite(ledEdgeAI, LOW);
  
  // Initialize serial communication
  Serial.begin(9600);
  delay(2000);
  
  Serial.println("\n========================================");
  Serial.println("   BICYCLE ALARM EDGE AI v2.2");
  Serial.println("   Arduino UNO R4 WiFi");
  Serial.println("========================================\n");
  
  // Show Edge AI status
  if (edgeAIEnabled) {
    Serial.println("[EDGE AI] Simulated AI system ACTIVATED");
    digitalWrite(ledEdgeAI, HIGH);  // Edge AI LED on
  } else {
    Serial.println("[EDGE AI] Disabled - Basic mode");
  }
  
  // Connect WiFi
  if (wifiEnabled) {
    connectWiFi();
  } else {
    Serial.println("[WIFI] Disabled");
  }
  
  // System test
  testSystem();
  
  Serial.println("\n✅ System ready. Monitoring sensors...");
  Serial.println("========================================\n");
  
  if (debugMode) {
    Serial.println("[DEBUG] Detailed mode activated");
    Serial.println("[DEBUG] Edge AI messages visible");
  }
}

// ======================= LOOP ==============================
void loop() {
  unsigned long currentTime = millis();
  
  // 1. READ SENSORS EVERY 50ms (for Edge AI)
  if (currentTime - lastSensorReading >= readingInterval) {
    lastSensorReading = currentTime;
    
    int digitalVibration = digitalRead(sensorPin);
    int analogVibration = analogRead(analogPin);
    
    // Save to history (only analog value)
    vibrationHistory[historyIndex] = analogVibration;
    historyIndex = (historyIndex + 1) % 5;
    
    // Debug: show readings if there are changes
    if (debugMode && digitalVibration == HIGH) {
      Serial.print("[SENSOR] Digital: ");
      Serial.print(digitalVibration);
      Serial.print(" | Analog: ");
      Serial.println(analogVibration);
    }
  }
  
  // 2. MAIN DETECTION
  int movement = digitalRead(sensorPin);
  
  if (movement == HIGH && !alarmActive) {
    // Verify minimum time between alarms
    if (currentTime - lastAlarm > minimumTimeBetweenAlarms) {
      totalEvents++;
      
      // 3. CONSULT EDGE AI 
      int analogVib = analogRead(analogPin);
      bool isReal = true;  // Default yes
      
      if (edgeAIEnabled) {
        isReal = isRealEvent(movement, analogVib);
        
        if (debugMode) {
          Serial.print("[EDGE AI] Analysis: ");
          Serial.println(isReal ? "✅ REAL EVENT - THEFT SUSPECTED" : "❌ FALSE POSITIVE");
        }
        
        if (isReal) {
          confirmedEvents++;
          digitalWrite(ledEdgeAI, LOW);  // Fast blinking
          delay(50);
          digitalWrite(ledEdgeAI, HIGH);
        } else {
          falsePositives++;
        }
      }
      
      // 4. ACTIVATE RESPONSE BASED ON EDGE AI DECISION
      if (isReal) {
        Serial.println("\n🚨🚨🚨 ALARM ACTIVATED! POSSIBLE THEFT DETECTED!");
        
        alarmActive = true;
        lastAlarm = currentTime;
        
        Serial.println("[ALARM] Alarm activated for theft alert");
        
        // Activate alarm
        activateAlarm();
        
        // 5. SEND TO THINGSPEAK - THEFT EVENT
        if (wifiEnabled && wifiConnected) {
          if (currentTime - lastThingSpeakConnection >= thingSpeakInterval) {
            Serial.println("[THINGSPEAK] Sending THEFT data...");
            // Send as confirmed theft event
            sendThingSpeak(true, analogRead(analogPin) / 1023.0);
            lastThingSpeakConnection = currentTime;
          } else {
            Serial.println("[THINGSPEAK] Waiting 30s between sends...");
          }
        }
        
        // Show statistics
        showStatistics();
        
      } else {
        // FALSE POSITIVE - Different response
        Serial.println("[INFO] False positive - No alarm activated");
        
        // Short beep only
        tone(buzzerPin, 1200, 200);
        digitalWrite(ledPin, HIGH);
        delay(250);
        digitalWrite(ledPin, LOW);
        
        // Send to ThingSpeak as FALSE POSITIVE 
        if (wifiEnabled && wifiConnected && edgeAIEnabled) {
          Serial.println("[THINGSPEAK] Sending FALSE POSITIVE data...");
          sendThingSpeak(false, analogRead(analogPin) / 1023.0);
        }
      }
    }
  }
  
  // 6. REARM ALARM when sensor returns to LOW
  if (movement == LOW) {
    alarmActive = false;
    digitalWrite(ledEdgeAI, HIGH);  // Normal Edge AI LED
  }
}

// ================== SIMULATED EDGE AI FUNCTIONS =============

bool isRealEvent(int vibDigital, int vibAnalog) {
  /*
    WEIGHTED EDGE AI (SIMILAR TO ML)
    Each condition adds "confidence"
    If confidence exceeds threshold → real event (THEFT)
  */

  float score = 0.0;

  // WEIGHT 1: high analog vibration
  if (vibAnalog > intensityThreshold) {
    score += 0.4;   // strong evidence
  }

  // WEIGHT 2: active digital signal
  if (vibDigital == HIGH) {
    score += 0.2;   // medium evidence
  }

  // WEIGHT 3: comparison with history
  int sum = 0;
  for (int i = 0; i < 5; i++) {
    sum += vibrationHistory[i];
  }
  int average = sum / 5;

  if (vibAnalog > average * 1.5) {
    score += 0.3;   // anomaly detected
  }

  // WEIGHT 4: very high peak (clear event)
  if (vibAnalog > 700) {
    score += 0.3;   // strong impact
  }

  // DEBUG
  if (debugMode) {
    Serial.print("[EDGE AI] Score = ");
    Serial.print(score, 2);
    Serial.print(" | Threshold: 0.6 | Decision: ");
    Serial.println(score >= 0.6 ? "THEFT" : "FALSE POSITIVE");
  }

  // DECISION THRESHOLD (inference)
  return (score >= 0.6);
}

// ================= IMPROVED EXISTING FUNCTIONS ==========

void connectWiFi() {
  Serial.print("[WIFI] Connecting to: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
    digitalWrite(ledPin, !digitalRead(ledPin));  // Blinking LED
  }
  
  digitalWrite(ledPin, LOW);
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\n[WIFI] ✅ Connected!");
    Serial.print("[WIFI] IP: ");
    Serial.println(WiFi.localIP());
    
    // Confirmation beep
    tone(buzzerPin, 1500, 200);
    delay(250);
    
  } else {
    wifiConnected = false;
    Serial.println("\n[WIFI] ❌ Could not connect");
    Serial.println("[WIFI] Alarm will work without connection");
    
    // Error pattern
    for (int i = 0; i < 3; i++) {
      tone(buzzerPin, 800, 200);
      delay(300);
      digitalWrite(ledPin, HIGH);
      delay(200);
      digitalWrite(ledPin, LOW);
    }
  }
}

void testSystem() {
  Serial.println("[TEST] Testing components...");
  
  // Test Edge AI LED
  digitalWrite(ledEdgeAI, HIGH);
  delay(300);
  digitalWrite(ledEdgeAI, LOW);
  delay(300);
  digitalWrite(ledEdgeAI, HIGH);
  
  // Test buzzer (3 tones)
  for (int i = 0; i < 3; i++) {
    int frequency = 1500 + i * 200;
    tone(buzzerPin, frequency, 150);
    digitalWrite(ledPin, HIGH);
    delay(200);
    digitalWrite(ledPin, LOW);
    delay(100);
  }
  
  Serial.println("[TEST] ✅ System verified\n");
}

void activateAlarm() {
  Serial.println("[ALARM] 🔊 Activating loud alarm...");
  
  unsigned long alarmStart = millis();
  const unsigned long alarmDuration = 3000;  // 3 seconds
  
  while (millis() - alarmStart < alarmDuration) {
    // ANNOYING sound pattern
    tone(buzzerPin, 3500, 200);  // Very high pitch
    digitalWrite(ledPin, HIGH);
    digitalWrite(ledEdgeAI, LOW);
    delay(200);
    
    tone(buzzerPin, 2800, 150);  // Medium pitch
    delay(150);
    
    tone(buzzerPin, 4200, 100);  // Super high pitch
    digitalWrite(ledPin, LOW);
    digitalWrite(ledEdgeAI, HIGH);
    delay(100);
    
    delay(50);
    
    // If sensor calms down, end earlier
    if (digitalRead(sensorPin) == LOW) {
      break;
    }
  }
  
  // Ensure everything is off
  digitalWrite(ledPin, LOW);
  digitalWrite(ledEdgeAI, HIGH);
  
  Serial.println("[ALARM] ✅ Alarm finished");
}

void sendThingSpeak(bool confirmedByAI, float intensity) {
  if (!wifiConnected) {
    Serial.println("[THINGSPEAK] ❌ No WiFi connection");
    return;
  }
  
  Serial.println("[THINGSPEAK] Connecting to server...");
  
  if (client.connect(server, 80)) {
    Serial.println("[THINGSPEAK] ✅ Connected, sending data...");
    
    // 4 SECOND DELAY BEFORE SENDING
    Serial.println("[THINGSPEAK] Waiting 4 seconds before sending...");
    delay(4000);
    
    // Create data string for ThingSpeak with CLEAR DISTINCTION
    String data = "api_key=" + apiKey;
    
    if (confirmedByAI) {
      // REAL THEFT EVENT
      data += "&field1=100";      // Alarm activated with HIGH value for theft
      data += "&field2=1";        // Edge AI confirmed = 1 (YES)
      Serial.println("[THINGSPEAK] Sending as: REAL THEFT EVENT");
    } else {
      // FALSE POSITIVE
      data += "&field1=10";       // Lower value for false positive
      data += "&field2=0";        // Edge AI confirmed = 0 (NO)
      Serial.println("[THINGSPEAK] Sending as: FALSE POSITIVE");
    }
    
    // Common fields
    data += "&field3=" + String(intensity * 100, 1);  // Intensity (0-100)
    data += "&field4=" + String(totalEvents);         // Total events
    data += "&field5=" + String(confirmedEvents);     // Confirmed theft events
    data += "&field6=" + String(falsePositives);      // False positives
    
    //  event type classification
    data += "&field7=" + String(confirmedByAI ? "2" : "1");  // 2=Theft, 1=FalsePositive
    
    // Send HTTP request
    client.println("POST /update HTTP/1.1");
    client.println("Host: api.thingspeak.com");
    client.println("Connection: close");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println("Content-Length: " + String(data.length()));
    client.println();
    client.println(data);
    
    Serial.println("[THINGSPEAK] Data sent to fields:");
    Serial.println("  Field 1: Alarm Level (100=Theft, 10=False)");
    Serial.println("  Field 2: AI Confirmation (1=Yes, 0=No)");
    Serial.println("  Field 3: Intensity = " + String(intensity * 100, 1) + "%");
    Serial.println("  Field 4: Total events = " + String(totalEvents));
    Serial.println("  Field 5: Theft events = " + String(confirmedEvents));
    Serial.println("  Field 6: False positives = " + String(falsePositives));
    Serial.println("  Field 7: Event Type (2=Theft, 1=False)");
    
    // Wait for response
    unsigned long timeout = millis();
    while (client.connected() && millis() - timeout < 3000) {
      if (client.available()) {
        String line = client.readStringUntil('\n');
        if (debugMode && line.startsWith("HTTP/1.1")) {
          Serial.print("[THINGSPEAK] Response: ");
          Serial.println(line);
        }
      }
    }
    
    client.stop();
    Serial.println("[THINGSPEAK] ✅ Data sent successfully");
    
    // Confirmation beep
    tone(buzzerPin, confirmedByAI ? 2000 : 1500, 100);
    delay(150);
    
  } else {
    Serial.println("[THINGSPEAK] ❌ Connection error");
    // Error beep
    tone(buzzerPin, 600, 300);
    delay(400);
  }
}

void showStatistics() {
  Serial.println("\n📊 ========= EDGE AI STATISTICS =========");
  Serial.print("   Total events detected: ");
  Serial.println(totalEvents);
  Serial.print("   Theft events (alarm activated): ");
  Serial.println(confirmedEvents);
  Serial.print("   False positives (no alarm): ");
  Serial.println(falsePositives);
  
  if (totalEvents > 0) {
    float theftPercentage = (confirmedEvents * 100.0) / totalEvents;
    float falsePercentage = (falsePositives * 100.0) / totalEvents;
    
    Serial.print("   🚨 Theft events: ");
    Serial.print(theftPercentage, 1);
    Serial.println("%");
    Serial.print("   ⚠️  False alarms: ");
    Serial.print(falsePercentage, 1);
    Serial.println("%");
    
    if (edgeAIEnabled) {
      Serial.print("   ✅ AI accuracy: ");
      Serial.print(100 - falsePercentage, 1);
      Serial.println("%");
    }
  }
  Serial.println("========================================\n");
}