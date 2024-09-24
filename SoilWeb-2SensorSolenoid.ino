#include <WiFi.h>
#include <HTTPClient.h>

// Informasi jaringan Wi-Fi
const char* ssid = "darly";     // Ganti dengan SSID Wi-Fi Anda
const char* password = "password"; // Ganti dengan password Wi-Fi Anda

// Alamat server Flask
const char* serverName = "http://172.20.10.5:5000/sensor-data";  // Ganti dengan alamat server Flask Anda

// Pin untuk sensor kelembapan tanah
const int soilMoisturePin1 = 34;  // Pin analog untuk sensor kelembapan tanah 1
const int soilMoisturePin2 = 35;  // Pin analog untuk sensor kelembapan tanah 2

// Pin untuk solenoid valve dan pompa air
const int solenoidValvePin1 = 25;  // Pin untuk solenoid valve baris 1
const int solenoidValvePin2 = 26;  // Pin untuk solenoid valve baris 2
const int pumpPin = 27;            // Pin untuk pompa air

// Ambang batas kelembapan tanah (%)
const float moistureThreshold = 50.0;  // Di bawah 40% dianggap butuh penyiraman

// Tambahkan variabel untuk status penyiraman
String isWatering1 = "false";
String isWatering2 = "false";

void setup() {
  Serial.begin(115200);

  // Inisialisasi pin solenoid valve dan pompa air sebagai output
  pinMode(solenoidValvePin1, OUTPUT);
  pinMode(solenoidValvePin2, OUTPUT);
  pinMode(pumpPin, OUTPUT);

  // Memulai koneksi ke Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan ke Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nTerhubung ke Wi-Fi");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) { // Memastikan koneksi Wi-Fi tersambung
    HTTPClient http;

    // Membaca data kelembapan tanah dari kedua sensor
    int soilMoistureValue1 = analogRead(soilMoisturePin1);
    int soilMoistureValue2 = analogRead(soilMoisturePin2);

    // Konversi nilai kelembapan tanah menjadi persentase (dibalik)
    float moisturePercentage1 = map(soilMoistureValue1, 0, 4095, 100, 0);
    float moisturePercentage2 = map(soilMoistureValue2, 0, 4095, 100, 0);

    // Mencetak nilai kelembapan tanah ke Serial Monitor
    Serial.print("Kelembapan Baris 1: ");
    Serial.print(moisturePercentage1);
    Serial.println("%");
    
    Serial.print("Kelembapan Baris 2: ");
    Serial.print(moisturePercentage2);
    Serial.println("%");

    // Logika untuk mengaktifkan solenoid valve dan pompa
    bool isValve1Open = false;
    bool isValve2Open = false;

    // Cek apakah kelembapan di bawah ambang batas untuk baris 1
    if (moisturePercentage1 < moistureThreshold) {
      digitalWrite(solenoidValvePin1, LOW);  // Buka solenoid valve baris 1
      isValve1Open = true;
      isWatering1 = "true";  // Menandai bahwa baris 1 sedang disiram
      Serial.println("Solenoid valve baris 1 dibuka.");
    } else {
      digitalWrite(solenoidValvePin1, HIGH);   // Tutup solenoid valve baris 1
      isWatering1 = "false";  // Menandai bahwa baris 1 tidak disiram
      Serial.println("Solenoid valve baris 1 ditutup.");
    }

    // Cek apakah kelembapan di bawah ambang batas untuk baris 2
    if (moisturePercentage2 < moistureThreshold) {
      digitalWrite(solenoidValvePin2, LOW);  // Buka solenoid valve baris 2
      isValve2Open = true;
      isWatering2 = "true";  // Menandai bahwa baris 2 sedang disiram
      Serial.println("Solenoid valve baris 2 dibuka.");
    } else {
      digitalWrite(solenoidValvePin2, HIGH);   // Tutup solenoid valve baris 2
      isWatering2 = "false";  // Menandai bahwa baris 2 tidak disiram
      Serial.println("Solenoid valve baris 2 ditutup.");
    }

    // Jika salah satu solenoid terbuka, nyalakan pompa air
    if (isValve1Open || isValve2Open) {
      digitalWrite(pumpPin, LOW);   // Nyalakan pompa air
      Serial.println("Pompa air dinyalakan.");
    } else {
      digitalWrite(pumpPin, HIGH);    // Matikan pompa air
      Serial.println("Pompa air dimatikan.");
    }

    // Mengirim data kelembapan ke server Flask
    http.begin(serverName);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // Format data POST untuk dua sensor
    String httpRequestData = "sensorValue1=" + String(moisturePercentage1) + "&sensorValue2=" + String(moisturePercentage2) + "&isWatering1=" + isWatering1 + "&isWatering2=" + isWatering2;
    int httpResponseCode = http.POST(httpRequestData);

    // Jika ada respons dari server, cetak responsnya
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Respons dari server: " + response);
    } else {
      Serial.println("Error mengirim data ke server, Kode respons: " + String(httpResponseCode));
    }

    // Mengakhiri koneksi HTTP
    http.end();
  } else {
    Serial.println("Tidak terhubung ke Wi-Fi");
  }

  // Tunggu 5 detik sebelum membaca kembali data
  delay(5000);
}
