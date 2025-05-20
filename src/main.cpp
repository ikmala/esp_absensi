#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <FS.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <PN532_I2C.h>
#include <NfcAdapter.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#define EEPROM_SIZE 96
#define WIFI_SSID_ADDR 0
#define WIFI_PASS_ADDR 32
#define MAX_SSID_LEN 32
#define MAX_PASS_LEN 32

const char* backendIP = "192.168.1.8";

IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

ESP8266WebServer server(80);
DNSServer dnsServer;
LiquidCrystal_I2C lcd(0x27, 16, 2);
PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc(pn532_i2c);
String uuid = "";
bool isNFCTapped = false;
unsigned long lastNfcCheck = 0;
const unsigned long nfcInterval = 500;
char wifiSSID[MAX_SSID_LEN];
char wifiPass[MAX_PASS_LEN];
unsigned long lastModalCheck = 0;
const unsigned long modalCheckInterval = 3000;
unsigned long lastScanCheck = 0;
const unsigned long scanInterval = 5000;
unsigned long lastConfigPoll = 0;
const unsigned long configPollInterval = 5300;
bool modalActive = false;
bool uuidTerdaftar = false;


void resetEEPROM() {
  Serial.println("Reset EEPROM: menulis 0 ke semua alamat EEPROM WiFi...");
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  Serial.println("Reset EEPROM selesai. Restarting...");
  delay(1000);
  ESP.restart();
}

// --- Fungsi baca tulis EEPROM ---
void readWiFiConfigFromEEPROM(char* ssid, char* pass) {
  for (int i = 0; i < MAX_SSID_LEN; i++) {
    ssid[i] = EEPROM.read(WIFI_SSID_ADDR + i);
    if (ssid[i] == 0xFF) ssid[i] = 0;
  }
  ssid[MAX_SSID_LEN - 1] = 0;
  for (int i = 0; i < MAX_PASS_LEN; i++) {
    pass[i] = EEPROM.read(WIFI_PASS_ADDR + i);
    if (pass[i] == 0xFF) pass[i] = 0;
  }
  pass[MAX_PASS_LEN - 1] = 0;
}

void saveWiFiConfigToEEPROM(const char* ssid, const char* pass) {
  for (int i = 0; i < MAX_SSID_LEN; i++) {
    EEPROM.write(WIFI_SSID_ADDR + i, ssid[i]);
  }
  for (int i = 0; i < MAX_PASS_LEN; i++) {
    EEPROM.write(WIFI_PASS_ADDR + i, pass[i]);
  }
  EEPROM.commit();
}

// --- Scan WiFi dan cek apakah ssid tersimpan ada ---
bool isStoredSSIDAvailable(const char* ssid) {
  Serial.println("Melakukan scan WiFi...");
  int n = WiFi.scanNetworks();
  Serial.printf("Ditemukan %d jaringan WiFi\n", n);
  for (int i = 0; i < n; i++) {
    String scannedSSID = WiFi.SSID(i);
    Serial.printf("SSID[%d]: %s\n", i, scannedSSID.c_str());
    if (scannedSSID.equals(String(ssid))) {
      Serial.println("SSID tersimpan ditemukan di scan WiFi.");
      return true;
    }
  }
  Serial.println("SSID tersimpan tidak ditemukan.");
  return false;
}

// --- Mode AP ---
void startAPMode() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  String apName = "Absensi_" + String(ESP.getChipId(), HEX);
  WiFi.softAP(apName.c_str(), "12345678");
  dnsServer.start(53, "*", local_IP);
  lcd.clear();
  lcd.print("Mode AP aktif");
  lcd.setCursor(0, 1);
  lcd.print(local_IP);
  Serial.println("Mode AP aktif: " + apName);
}

// --- Koneksi WiFi ---
bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  Serial.printf("Mencoba konek ke WiFi: %s\n", wifiSSID);
  WiFi.begin(wifiSSID, wifiPass);
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi berhasil konek!");
    lcd.clear();
    lcd.print("WiFi Connected");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    return true;
  } else {
    Serial.println("Gagal konek WiFi");
    return false;
  }
}

// --- Handler root ---
void handleRoot() {
  if (WiFi.getMode() == WIFI_AP) {
    if (SPIFFS.exists("/index.html")) {
      File f = SPIFFS.open("/index.html", "r");
      server.streamFile(f, "text/html");
      f.close();
    } else {
      server.send(500, "text/plain", "File index.html tidak ditemukan");
    }
  } else {
    server.sendHeader("Location", "http://localhost:5000", true);
    server.send(302, "text/plain", "");
  }
}

// --- Handler simpan WiFi dengan scan SSID ---
void handleSaveWiFi() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    if (ssid.length() > 0 && ssid.length() < MAX_SSID_LEN && password.length() < MAX_PASS_LEN) {
      ssid.toCharArray(wifiSSID, MAX_SSID_LEN);
      password.toCharArray(wifiPass, MAX_PASS_LEN);
      saveWiFiConfigToEEPROM(wifiSSID, wifiPass);
      WiFi.softAPdisconnect(true);
      server.send(200, "text/html", "<html><body><h2>WiFi disimpan. Restart...</h2></body></html>");
      delay(2000);
      ESP.restart();
    } else {
      server.send(200, "text/html", "<script>alert('SSID atau password tidak valid'); window.history.back();</script>");
    }
  } else {
    server.send(200, "text/html", "<script>alert('SSID dan password harus diisi'); window.history.back();</script>");
  }
}

// --- Handler daftar SSID scan ---
void handleScanWiFi() {
  int n = WiFi.scanNetworks();
  String wifiList = "[";
  for (int i = 0; i < n; i++) {
    wifiList += "\"" + WiFi.SSID(i) + "\"";
    if (i < n - 1) wifiList += ",";
  }
  wifiList += "]";
  server.send(200, "application/json", wifiList);
}

void getModalStatusFromServer() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;
    String url = String("http://") + backendIP + ":5000/api/modal-status";
    http.begin(client, url);
    int httpCode = http.GET();
    if (httpCode == 200) {
      String payload = http.getString();
      StaticJsonDocument<256> doc;
      auto err = deserializeJson(doc, payload);
      if (!err) {
        bool newStatus = doc["isOpen"];
        if (modalActive != newStatus) {
          modalActive = newStatus;
          Serial.print("Status modal diperbarui dari server: ");
          Serial.println(modalActive ? "OPEN" : "CLOSED");
        }
      } else {
        Serial.println("Gagal parse JSON response");
      }
    } else {
      Serial.printf("GET modal-status gagal, code: %d\n", httpCode);
    }
    http.end();
  } else {
    Serial.println("WiFi belum terkoneksi, tidak bisa polling status modal");
  }
}

void postAbsensi(const String& uuid) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;
    String urlAbsensi = String("http://") + backendIP + ":5000/api/absensi/scan";
    http.begin(client, urlAbsensi);
    http.addHeader("Content-Type", "application/json");
    String body = "{\"UUIDguru\":\"" + uuid + "\"}";
    int httpResponseCode = http.POST(body);
    if (httpResponseCode == 200) {
      String response = http.getString();
      Serial.println("Absensi berhasil dikirim ke server:");
      Serial.println(response);
      lcd.clear();
      lcd.print("Absensi Tercatat");
    } else {
      Serial.printf("Gagal kirim absensi, kode: %d\n", httpResponseCode);
      lcd.clear();
      lcd.print("Absensi sudah lengkap");
    }
    http.end();
  } else {
    Serial.println("WiFi belum terkoneksi, gagal kirim absensi");
    lcd.clear();
    lcd.print("WiFi belum konek");
  }
}

void sendUUIDtoServer(const String& uuid) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;
    String urlCheckUUID = String("http://") + backendIP + ":5000/api/esp/check-uuid";
    http.begin(client, urlCheckUUID);
    http.addHeader("Content-Type", "application/json");
    String body = "{\"UUIDguru\":\"" + uuid + "\"}";
    int httpResponseCode = http.POST(body);
    if (httpResponseCode == 200) {
      String response = http.getString();
      Serial.println("UUID terdaftar di server:");
      Serial.println(response);
      uuidTerdaftar = true;
      postAbsensi(uuid);
    } else if (httpResponseCode == 404) {
      Serial.println("UUID belum terdaftar di server.");
        uuidTerdaftar = false;
        lcd.clear();
        lcd.print("Belum Terdaftar");
    } else {
      Serial.printf("Error saat POST: %d\n", httpResponseCode);
      uuidTerdaftar = false;
    }
    http.end();
  } else {
    Serial.println("WiFi belum terkoneksi");
  }
}

void registerUUIDtoServer(const String& uuid) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;
    String urlRegisterUUID = String("http://") + backendIP + ":5000/api/esp/send-uuid";
    http.begin(client, urlRegisterUUID);
    http.addHeader("Content-Type", "application/json");
    String body = "{\"UUIDguru\":\"" + uuid + "\"}";
    int httpResponseCode = http.POST(body);
    if (httpResponseCode == 200) {
      Serial.println("UUID yang mau didaftarkan dikirim ke server:");
      Serial.println(uuid);
      lcd.clear();
      lcd.print("UUID Didaftarkan");
    } else {
      Serial.printf("Gagal kirim UUID baru, code: %d\n", httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi belum terkoneksi");
  }
}

bool shouldScanSSIDFromServer() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;
    String urlScanWifi = String("http://") + backendIP + ":5000/api/command/scan-wifi";
    http.begin(client, urlScanWifi);
    int httpCode = http.GET();
    if (httpCode == 200) {
      String response = http.getString();
      StaticJsonDocument<128> doc;
      if (!deserializeJson(doc, response)) {
        bool shouldScan = doc["scan"];
        http.end();
        return shouldScan;
      }
    }
    http.end();
  }
  return false;
}

void sendScanResultsToBackend(JsonArray ssidArray) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;
    String urlSendResults = String("http://") + backendIP + ":5000/api/wifi/results";
    http.begin(client, urlSendResults);
    http.addHeader("Content-Type", "application/json");
    StaticJsonDocument<256> doc;
    doc["ssids"] = ssidArray;
    String body;
    serializeJson(doc, body);
    int code = http.POST(body);
    if (code == 200) {
      Serial.println("Hasil scan SSID berhasil dikirim ke backend.");
    } else {
      Serial.printf("Gagal kirim hasil scan, code: %d\n", code);
    }
    http.end();
  }
}

void saveNewWiFiConfigToEEPROM(const String& ssid, const String& password) {
  char ssidArr[MAX_SSID_LEN];
  char passArr[MAX_PASS_LEN];
  ssid.toCharArray(ssidArr, MAX_SSID_LEN);
  password.toCharArray(passArr, MAX_PASS_LEN);
  saveWiFiConfigToEEPROM(ssidArr, passArr);
  Serial.println("SSID & password baru berhasil disimpan di EEPROM");
}

void pollWifiConfigFromBackend() {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  WiFiClient client;
  String url = String("http://") + backendIP + ":5000/api/wifi/config";
  http.begin(client, url);
  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    StaticJsonDocument<512> doc;
    auto err = deserializeJson(doc, payload);
    if (!err) {
      if (!doc["config"].isNull()) {
        JsonObject config = doc["config"];
        String ssid = config["ssid"] | "";
        String password = config["password"] | "";
        String ipMode = config["ipMode"] | "Dynamic";
        String ipAddr = config["ipAddress"] | "";
        String gateway = config["gateway"] | "";
        String subnet = config["subnetMask"] | "";
        Serial.printf("Polling WiFi config ditemukan: SSID=%s, IP Mode=%s\n", ssid.c_str(), ipMode.c_str());
        WiFi.disconnect(true);
        delay(1000);
        if (ipMode == "Static" && ipAddr.length() > 0 && gateway.length() > 0 && subnet.length() > 0) {
          IPAddress localIP, gatewayIP, subnetMask;
          localIP.fromString(ipAddr);
          gatewayIP.fromString(gateway);
          subnetMask.fromString(subnet);
          if (!WiFi.config(localIP, gatewayIP, subnetMask)) {
            Serial.println("Gagal set static IP!");
          } else {
            Serial.print("Static IP berhasil diset: ");
            Serial.println(localIP);
          }
        } else {
          Serial.println("Mode DHCP (dynamic), tidak set IP statis");
        }
        WiFi.begin(ssid.c_str(), password.c_str());
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
          delay(500);
          Serial.print(".");
        }
        Serial.println();
        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("WiFi berhasil connect ke jaringan baru.");
          Serial.print("IP Address: ");
          Serial.println(WiFi.localIP());
          lcd.clear();
          lcd.print("WiFi Connected");
          lcd.setCursor(0, 1);
          lcd.print(WiFi.localIP());
          saveNewWiFiConfigToEEPROM(ssid, password);
          String urlApplied = String("http://") + backendIP + ":5000/api/wifi/config/applied";
          http.begin(client, urlApplied);
          http.addHeader("Content-Type", "application/json");
          int code = http.POST("{}");
          if (code == 200) {
            Serial.println("Backend dikabari konfigurasi sudah diterapkan");
          }
          http.end();
        } else {
          Serial.println("Gagal connect WiFi.");
          lcd.clear();
          lcd.print("Gagal Connect");
        }
      } else {
        Serial.println("Tidak ada konfigurasi wifi baru");
      }
    } else {
      Serial.println("Error parsing JSON polling konfigurasi");
    }
  } else {
    Serial.printf("Polling konfigurasi wifi gagal, code: %d\n", httpCode);
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  // resetEEPROM();
  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Menghubungkan...");
  if (!SPIFFS.begin()) {
    Serial.println("Gagal mount SPIFFS");
  }
  nfc.begin();
  readWiFiConfigFromEEPROM(wifiSSID, wifiPass);
  bool ssidAvailable = false;
  if (strlen(wifiSSID) > 0) {
    Serial.printf("Mengecek SSID tersimpan: %s\n", wifiSSID);
    ssidAvailable = isStoredSSIDAvailable(wifiSSID);
  }
  if (ssidAvailable) {
    Serial.println("SSID ditemukan, mencoba konek WiFi...");
    if (!connectWiFi()) {
      Serial.println("Koneksi WiFi gagal, mulai mode AP...");
      startAPMode();
    } else {
      Serial.println("WiFi Connected, matikan AP jika aktif");
      if (WiFi.getMode() & WIFI_AP) {
        WiFi.softAPdisconnect(true);
        delay(1000);
      }
    }
  } else {
    Serial.println("SSID tidak ditemukan di scan WiFi, mulai mode AP...");
    startAPMode();
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/saveWiFi", HTTP_POST, handleSaveWiFi);
  server.on("/scanWiFi", HTTP_GET, handleScanWiFi);
  server.onNotFound([]() {
    server.send(404, "text/plain", "404: Not Found");
  });
  server.begin();
  Serial.println("HTTP server mulai berjalan");

}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  if (millis() - lastModalCheck > modalCheckInterval) {
    lastModalCheck = millis();
    getModalStatusFromServer();
  }

  if (millis() - lastNfcCheck > nfcInterval) {
    lastNfcCheck = millis();
    if (nfc.tagPresent()) {
      NfcTag tag = nfc.read();
      uuid = tag.getUidString();
      Serial.print("UID NFC terbaca: ");
      Serial.println(uuid);
      isNFCTapped = true;
      if (modalActive) {
        if (!uuidTerdaftar) {
          registerUUIDtoServer(uuid);
        } else {
          Serial.println("UUID sudah terdaftar, tidak perlu daftar ulang");
        }
      } else {
        sendUUIDtoServer(uuid);
      }
    } else if (isNFCTapped) {
      Serial.println("Tidak ada NFC, siap absen");
      lcd.clear();
      lcd.print("Siap Absen");
      isNFCTapped = false;
      uuid = "";
      uuidTerdaftar = false;
    }
  }

  if (millis() - lastScanCheck > scanInterval) {
    lastScanCheck = millis();
    if (shouldScanSSIDFromServer()) {
      Serial.println("ESP diminta scan SSID oleh backend...");
      int n = WiFi.scanNetworks();
      StaticJsonDocument<256> doc;
      JsonArray arr = doc.to<JsonArray>();
      for (int i = 0; i < n; i++) {
        arr.add(WiFi.SSID(i));
        Serial.println(WiFi.SSID(i));
      }
      sendScanResultsToBackend(arr);
    }
  }

  if (millis() - lastConfigPoll > configPollInterval) {
    lastConfigPoll = millis();
    pollWifiConfigFromBackend();
  }
}
