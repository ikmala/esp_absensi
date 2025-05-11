#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <PN532_I2C.h>
#include <NfcAdapter.h>
#include <Wire.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

#define PASSWORD_ADDR 0
#define USER_DATA_ADDR 32

const char* ssid = "";
const char* password = "";
String adminUsername = "admin";  // Default username login admin
String adminPassword = "1234";  // Default password login admin
IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1); 
IPAddress subnet(255, 255, 255, 0);

ESP8266WebServer server(80);
DNSServer dnsServer;
LiquidCrystal_I2C lcd(0x27, 16, 2);
PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);
String uuid = "";
bool isNFCTapped = false;

void saveLoginData(const char* password) {
  EEPROM.begin(512);
  Serial.println("Saving password to EEPROM...");
  for (int i = 0; i < 32; i++) {
    if (password[i] != '\0') {
      EEPROM.write(PASSWORD_ADDR + i, password[i]);
    } else {
      EEPROM.write(PASSWORD_ADDR + i, '\0');
      break;
    }
  }
  EEPROM.commit();
}

String loadPassword() {
  char password[32] = {0};
  EEPROM.begin(512);
  Serial.println("Loading password from EEPROM...");
  for (int i = 0; i < 32; i++) {
    password[i] = EEPROM.read(PASSWORD_ADDR + i);
    if (password[i] == '\0') break;
  }
  String loadedPassword = String(password);
  Serial.print("password login ");
  Serial.println(loadedPassword);
  return loadedPassword;
}

void resetLoginData() {
  String defaultPassword = adminPassword;
  saveLoginData(defaultPassword.c_str());
  Serial.println("Data reset to default.");
}

void checkAndLoadLoginData() {
  String storedPassword = loadPassword();
  if (storedPassword == adminPassword) {
    if (storedPassword == adminPassword) {
      saveLoginData(adminPassword.c_str());
    }
  }
  if (storedPassword == "") {
    saveLoginData(adminPassword.c_str());
  }
}

void readNFC() {
  if (nfc.tagPresent()) {
    NfcTag tag = nfc.read();
    uuid = tag.getUidString();
    Serial.print("UUID yang terbaca: ");
    Serial.println(uuid);
    delay(2000);
    lcd.clear();
    isNFCTapped = true;
  } else if (!nfc.tagPresent() && isNFCTapped) {
    Serial.println("Tidak ada NFC yang terbaca, siap absen.");
    isNFCTapped = false;
  }
  delay(2000);
}

String readUserDataFromEEPROM(int& addr) {
  String data = "";
  char c;
  String jsonString = "";
  while ((c = EEPROM.read(addr++)) != '\0') {
    jsonString += c;
  }
  return jsonString;
}

void saveUserDataToEEPROM(String newuid, String name, String position, String department, String phone) {
  int addr = USER_DATA_ADDR;
  DynamicJsonDocument doc(1024);
  JsonObject user = doc.createNestedObject();
  user["uid"] = newuid;
  user["name"] = name;
  user["position"] = position;
  user["department"] = department;
  user["phone"] = phone;
  String jsonString;
  serializeJson(doc, jsonString);
  for (int i = 0; i < jsonString.length(); i++) {
    EEPROM.write(addr++, jsonString[i]);
  }
  EEPROM.write(addr++, '\0');
  EEPROM.commit();
  Serial.println("User data saved to EEPROM as JSON successfully!");
}

bool isUUIDExist(String newUUID) {
  int addr = USER_DATA_ADDR;
  char c;
  String jsonString = "";
  
  while (true) {
    jsonString = "";
    while ((c = EEPROM.read(addr++)) != '\0') {
      jsonString += c;
    }
    if (jsonString == "") {
      break;
    }
    
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, jsonString);
    if (error) {
      Serial.println("Failed to parse JSON");
      return false;
    }
    
    String uuidFromEEPROM = doc["uid"];
    if (uuidFromEEPROM == newUUID) {
      return true;
    }
  }
  return false;
}

String getNameByUUID(String newUUID) {
  int addr = USER_DATA_ADDR;
  char c;
  String uuidFromEEPROM = "";
  String name = "";
  while ((c = EEPROM.read(addr++)) != '\0') {
    uuidFromEEPROM += c;
  }
  if (uuidFromEEPROM == newUUID) {
    while ((c = EEPROM.read(addr++)) != '\0') {
      name += c;
    }
    return name;
  }
  return "";
}

void handleBackup() {
  int addr = USER_DATA_ADDR;
  DynamicJsonDocument doc(1024);
  JsonArray users = doc.createNestedArray("users");
  while (true) {
    String newUUID = "";
    String name = "", position = "", department = "", phone = "";
    char c;
    while ((c = EEPROM.read(addr++)) != '\0') {
      newUUID += c;
    }
    if (newUUID == "") {
      break;
    }
    while ((c = EEPROM.read(addr++)) != '\0') {
      name += c;
    }
    while ((c = EEPROM.read(addr++)) != '\0') {
      position += c;
    }
    while ((c = EEPROM.read(addr++)) != '\0') {
      department += c;
    }
    while ((c = EEPROM.read(addr++)) != '\0') {
      phone += c;
    }
    JsonObject user = users.createNestedObject();
    user["uid"] = newUUID;
    user["name"] = name;
    user["position"] = position;
    user["department"] = department;
    user["phone"] = phone;
  }
  String jsonOutput;
  serializeJson(doc, jsonOutput);
  server.sendHeader("Content-Type", "application/json");
  server.sendHeader("Content-Disposition", "attachment; filename=user_data_backup.json");
  server.send(200, "application/json", jsonOutput);
}

void handleRestore() {
  HTTPUpload& upload = server.upload(); 
  if (upload.status == UPLOAD_FILE_START) {
    Serial.println("Starting file upload...");
    String filename = upload.filename;
    Serial.print("Filename: ");
    Serial.println(filename);
    File file = SPIFFS.open("/" + filename, "w");
    if (!file) {
      server.send(500, "text/plain", "Failed to open file for writing");
      return;
    }
  } 
  else if (upload.status == UPLOAD_FILE_WRITE) {
    File file = SPIFFS.open("/" + upload.filename, "a");
    if (file) {
      file.write(upload.buf, upload.currentSize);
    }
  } 
  else if (upload.status == UPLOAD_FILE_END) {
    File file = SPIFFS.open("/" + upload.filename, "r");
    if (!file) {
      server.send(500, "text/plain", "Failed to read uploaded file");
      return;
    }
    String jsonString = file.readString();
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, jsonString);
    if (error) {
      server.send(400, "text/plain", "Failed to parse JSON");
      return;
    }
    JsonArray users = doc["users"].as<JsonArray>();
    for (JsonObject user : users) {
      String newUUID = user["uid"];
      String name = user["name"];
      String position = user["position"];
      String department = user["department"];
      String phone = user["phone"];
      saveUserDataToEEPROM(newUUID, name, position, department, phone);
    }
    server.send(200, "text/plain", "Data restored successfully");
  }
}

void handleReset() {
  EEPROM.begin(512);
  saveLoginData(adminPassword.c_str());
  int addr = USER_DATA_ADDR;
  for (int i = 0; i < 512; i++) {
    EEPROM.write(addr++, 0);
  }
  EEPROM.commit();
  server.send(200, "text/plain", "Settings reset to default. Restarting...");
  ESP.restart();
}

void absensi() {
 if (isNFCTapped) {
    int addr = USER_DATA_ADDR;
    String userData = readUserDataFromEEPROM(addr);
    Serial.println("data di eepromnya: ");
    Serial.print(userData);
    if (userData != "") {
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, userData);
      
      if (error) {
        Serial.println("Failed to parse JSON");
        return;
      }
      JsonArray users = doc.as<JsonArray>();
      for (JsonObject user : users) {
        String storedUid = user["uid"]; 
        String storedNama = user["name"]; 
        if (storedUid == uuid) {
          Serial.println("Absen Oke ");
          Serial.print(storedNama);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(storedNama);
          lcd.setCursor(0, 1);
          lcd.print("Absen");
          return;
        }
      }
      Serial.println("Belum Terdaftar");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Belum Terdaftar");
    } else {
      Serial.println("Data di EEPROM kosong.");
    }
  } else {
    Serial.println("Menunggu Tap NFC...");
  }
}

void handleScanWiFi() {
  int n = WiFi.scanNetworks();
  String wifiList = "[";
  if (n == 0) {
    wifiList += "]";
    Serial.println("No WiFi networks found.");
  } else {
    for (int i = 0; i < n; i++) {
      wifiList += "\"" + WiFi.SSID(i) + "\"";
      if (i < n - 1) {
        wifiList += ",";
      }
    }
    wifiList += "]";
  }
  server.send(200, "application/json", wifiList);
}

void handleWiFiSubmit() {
  String ssid = server.arg("ssid");
  String pass = server.arg("password");
  String ipMode = server.arg("ipMode");
  String staticIp = server.arg("staticIp");
  String gateway = server.arg("gateway");
  String subnet = server.arg("subnet");
  if (ipMode == "static") {
    IPAddress ip;
    ip.fromString(staticIp);
    IPAddress gw;
    gw.fromString(gateway);
    IPAddress mask;
    mask.fromString(subnet);
    WiFi.config(ip, gw, mask, IPAddress(8, 8, 8, 8), IPAddress(8, 8, 4, 4));
  }
  WiFi.begin(ssid.c_str(), pass.c_str());
    int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    String ipAddress = WiFi.localIP().toString();
    server.send(200, "text/plain", "WiFi Connected: " + ipAddress);
  } else {
    server.send(200, "text/plain", "WiFi Connection Failed");
  }
}

void handleWiFiConfig() {
  String html = "<html><body>";
  html += "<h1>WiFi Scan</h1>";
  html += "<form action='/submit' method='POST'>";
  html += "<label for='ssid'>SSID:</label><select name='ssid' id='ssid'></select><br/>";
  html += "<label for='password'>Password:</label><input type='password' name='password' id='password' required/><br/>";
  html += "<input type='submit' value='Connect'/>";
  html += "</form></body></html>";
  server.send(200, "text/html", html);
}

void handleRoot() {
  String deviceId = String(ESP.getChipId());
  String wifiConnection = "-"; 
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnection = WiFi.SSID();
  }
  File file = SPIFFS.open("/index.html", "r");
  String html = "";
  while (file.available()) {
    html += char(file.read());
  }
  html.replace("%newUUID%", uuid);
  html.replace("%DEVICE_ID%", deviceId); 
  html.replace("%WIFI_CONNECTION%", wifiConnection);
  html.replace("%ADMIN_PASSWORD%", loadPassword());
  
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  Wire.begin();
  checkAndLoadLoginData();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Init...");
  String deviceId = String(ESP.getChipId());

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.softAPConfig(local_IP, gateway, subnet);
    WiFi.softAP("Absensi_" + deviceId, "12345678");
    dnsServer.start(53, "*", local_IP);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Mode AP: Ready");
    lcd.setCursor(0, 1);
    lcd.print(local_IP);
  } else {
    String ipAddress = WiFi.localIP().toString();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected");
    lcd.setCursor(0, 1);
    lcd.print("IP: " + ipAddress);
    WiFi.softAPdisconnect(true);
  }
  server.begin();
  nfc.begin();

  if (!SPIFFS.begin()) {
    Serial.println("Error mounting SPIFFS");
    return;
  }
  if (!SPIFFS.exists("/index.html")) {
    Serial.println("index.html does not exist.");
    return;
  }

  server.on("/favicon.ico", HTTP_GET, [](){
    server.send(204, "text/plain", "");
  });

  server.on("/", HTTP_GET, handleRoot);  
  server.on("/login", HTTP_POST, []() {
    String username = server.arg("username");
    String password = server.arg("password");
    String storedPassword = loadPassword();
    if (username == adminUsername && password == storedPassword) {
      server.send(200, "text/plain", "Login Success");
    } else {
      server.send(401, "text/plain", "Invalid login credentials");
    }
  });

  server.on("/wifi", HTTP_GET, handleWiFiConfig);


  server.on("/save-settings", HTTP_POST, []() {
    String newPasswordlogin = server.arg("newPasswordlogin");
    saveLoginData(newPasswordlogin.c_str());
    server.send(200, "text/plain", "Settings saved successfully!");
  });

  server.on("/reset-settings", HTTP_POST, []() {
    resetLoginData();
    server.send(200, "text/plain", "Settings reset to default.");
  });

  server.on("/getUUID", HTTP_GET, []() {
    String jsonResponse = "{\"uuid\": \"" + uuid + "\"}";
    server.send(200, "application/json", jsonResponse);
  });

  server.on("/save-user", HTTP_POST, []() {
    String newuid = server.arg("newUUID");
    String name = server.arg("new-name");
    String position = server.arg("new-position");
    String department = server.arg("new-department");
    String phone = server.arg("new-phone"); 
    if (isUUIDExist(newuid)) {
      server.send(400, "text/plain", "UUID sudah ada dengan nama: " + name);
      return;
    }
    saveUserDataToEEPROM(newuid, name, position, department, phone);
    server.send(200, "text/plain", "User data saved successfully!");
  });

  server.on("/get-user-data", HTTP_GET, []() {
    int addr = USER_DATA_ADDR;
    String userData = readUserDataFromEEPROM(addr);
    String jsonResponse = "{\"user_data\": \"" + userData + "\"}";
    server.sendHeader("Content-Type", "application/json; charset=utf-8");
    server.send(200, "application/json", jsonResponse);
  });

  server.on("/backup", HTTP_GET, handleBackup);  
  server.on("/restore", HTTP_POST, handleRestore);
  server.on("/reset", HTTP_POST, handleReset);
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  readNFC();
  yield();
  lcd.setCursor(0, 0);
  lcd.print("NFC Ready!");
  delay(2000);
  lcd.clear();
  lcd.print("Absen siap");
  absensi();
  delay(3000);
}
