// ----------------------------
// BLYNK CONFIG
#define BLYNK_TEMPLATE_ID "TMPL39lbbiK0_"
#define BLYNK_TEMPLATE_NAME "AIR QUALITY MONITORING"
#define BLYNK_AUTH_TOKEN "0EC-4WMANYyy_yUqoCmlEK6D_ip9hUCE"

#define BLYNK_PRINT Serial

// ----------------------------
// Include libraries
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <LiquidCrystal_PCF8574.h>
#include <Wire.h>
#include <ESP_Mail_Client.h>

// ----------------------------
// WiFi Credentials
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "SGK";
char pass[] = "123456789";

// ----------------------------
// Sensor & LCD setup
DHT dht(D4, DHT11);
int gasPin = A0;
LiquidCrystal_PCF8574 lcd(0x27);
BlynkTimer timer;

// ----------------------------
// Email Configuration
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
#define AUTHOR_EMAIL "kaggasaigopalakrishna@gmail.com"
#define AUTHOR_PASSWORD "RaidMachine5"

SMTPSession smtp;
Session_Config config;
SMTP_Message message;

// ----------------------------
// Determine air quality status
String airQualityStatus(int gasValue) {
  if (gasValue < 200) return "Good";
  else if (gasValue < 400) return "Moderate";
  else if (gasValue < 700) return "Poor";
  else return "Danger!";
}

// ----------------------------
// Send email alert function
void sendEmailSelf(String temp, String hum, String gas, String quality) {
  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;
  config.time.ntp_server = "pool.ntp.org";
  config.time.gmt_offset = 5.5;

  message.sender.name = "ESP8266 Air Quality";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = "Air Quality Alert";
  message.addRecipient("Recipient", AUTHOR_EMAIL);

  String body = "Temperature: " + temp + " °C\n" +
                "Humidity: " + hum + " %\n" +
                "Gas Value: " + gas + "\n" +
                "Air Quality: " + quality;

  message.text.content = body.c_str();

  if (!smtp.connect(&config)) {
    Serial.println("SMTP Connect Failed!");
    return;
  }

  if (!MailClient.sendMail(&smtp, &message)) {
    Serial.print("Email send failed: ");
    Serial.println(smtp.errorReason());
  } else {
    Serial.println("✅ Email Sent!");
  }

  smtp.closeSession();
}

// ----------------------------
// Read sensor data and process
void sendSensorData() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int gasValue = analogRead(gasPin);
  String quality = airQualityStatus(gasValue);

  if (isnan(temp) || isnan(hum)) {
    Serial.println("DHT Read Failed!");
    return;
  }

  // Send data to Blynk
  Blynk.virtualWrite(V0, temp);
  Blynk.virtualWrite(V1, hum);
  Blynk.virtualWrite(V2, gasValue);

  // Clear LCD screen
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("                ");

  // Display on LCD
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temp, 1);
  lcd.print("C H:");
  lcd.print(hum, 0);
  lcd.print("%");

  lcd.setCursor(0, 1);
  lcd.print("Gas:");
  lcd.print(gasValue);
  lcd.print(" ");
  lcd.print(quality);

  // Serial output for debugging
  Serial.printf("Temp: %.1f°C | Hum: %.1f%% | Gas: %d | %s\n",
                temp, hum, gasValue, quality.c_str());

  // Send email alert only if "Danger!"
  if (quality == "Danger!") {
    sendEmailSelf(String(temp), String(hum), String(gasValue), quality);
  }
}

// ----------------------------
// Setup
void setup() {
  Serial.begin(9600);
  Wire.begin(D2, D1);
  lcd.begin(16, 2);
  lcd.setBacklight(HIGH);
  lcd.clear();
  lcd.print("Air Quality Init");

  dht.begin();

  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  lcd.setCursor(0, 1);
  lcd.print("WiFi Connected");

  Blynk.config(auth);

  timer.setInterval(5000L, sendSensorData);
}

// ----------------------------
// Main loop
void loop() {
  Blynk.run();
  timer.run();
}
