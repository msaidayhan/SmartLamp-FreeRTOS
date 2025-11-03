#  SmartLamp – FreeRTOS Tabanlı Akıllı Lamba

Bu proje, Necmettin Erbakan Üniveristesi Seydişehir Ahmet Cengiz Mühendislik Fakültesi İşletim Sistemleri dersi kapsamında, **FreeRTOS’un görev yönetimi ve senkronizasyon** özelliklerini ESP32 üzerinde uygulamalı olarak göstermek amacıyla geliştirilmiştir.  
Sistem; **DHT22 nem/sıcaklık sensörü**, **WS2812 LED şeridi** ve **ESP32'nin Wi-Fi tabanlı web arayüzü** üzerinden kullanıcı etkileşimi sağlar.

## 🚀 Özellikler
- **FreeRTOS Task’ları:** Sensör okuma, LED kontrolü ve web sunucusu ayrı görevlerde çalışır.
- **Queue & Semaphore Kullanımı:** Görevler arası iletişim için veri kuyruğu ve senkronizasyon yapıları.
- **Web Arayüzü:** Anlık sıcaklık/nem verilerini gösterir, LED rengini manuel veya otomatik modda değiştirir.
- **Wi-Fi Bağlantısı:** ESP32 Wi-Fi üzerinden tarayıcıya gerçek zamanlı veri gönderir.

## 🧩 Donanım Bileşenleri
- ESP32 DevKit v1  
- DHT22 Sensörü  
- WS2812B LED Şeridi  
- 5V Güç Kaynağı  

## ⚙️ Yazılım Mimarisi
- ESP-IDF 5.5  
- FreeRTOS Kernel  
- HTTP Web Server  
- DHT22 Kütüphanesi (`esp-idf-lib/dht`)  

## 📸 Sistem Diyagramı
ESP32 – Sensör – LED şeridi bağlantı diyagramı aşağıdaki gibidir:
*(Rapor versiyonundaki şemayı buraya da koyabilirsin)*

## 👨‍💻 Geliştirici
**Mustafa Said Dayhan**  
📘 Öğrenci No: 22370031086  
📚 Necmettin Erbakan Üniversitesi – Bilgisayar Mühendisliği  

