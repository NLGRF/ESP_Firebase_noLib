#include <ESP8266WiFi.h>
#include <DHT.h>                // ใช้ของ Adafruit https://github.com/adafruit/DHT-sensor-library
#include <Wire.h>
#include <LiquidCrystal_I2C.h>  // ใช้ของ DFRobot.com

//--------------------------------------------------------
#define WIFI_SSID             "--------------------"  // WiFi Router
#define WIFI_PASSWORD         "--------------------"  // รหัส WiFi
#define FIREBASE_HOST         "----------------------------"  // Host Firebase
#define FIREBASE_AUTH         "----------------------------"  // รหัสของฐานข้อมูลข้างต้น
//--------------------------------------------------------
/* ฟังกชั่น สำหรับ รับและส่งข้อมูลไปยัง Firebase*/
String  TD_Get_Firebase(String path );                //  รับจาก Firebase
int     TD_Set_Firebase(String path, String value, bool push=false ); // ส่งไปยัง Firebase
int     TD_Push_Firebase(String path, String value ); //  ส่งไปยัง Firebase แบบ Pushing Data
//--------------------------------------------------------
LiquidCrystal_I2C lcd(0x27,20,4);    // ประกาศตัวแปรสำหรับจอ lcd
int Soil = A0;                       // ขา pin ของ soil sensor
//DHT dht( D3, DHT11);               // หากใช้ DHT11 ให้กำหนดแบบตัวอย่างนี้
DHT dht( D3, DHT22);                 // ขา pin ของ DHT22 ตัวแรก
DHT dht2( D4, DHT22);                // ขา pin ของ DHT22 ตัวที่ 2

//--------------------------------------------------------

void setup() {
  Serial.begin(115200); Serial.println();
  dht.begin(); 
  dht2.begin();
  lcd.init(); lcd.backlight();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);    // เชื่อมต่อไวไฟ
  while(!WiFi.isConnected()) delay(400);   // รอจนกว่าเชื่อมต่อสำเร็จ
  Serial.println(WiFi.localIP());          // แสดงค่า IP หลังเชื่อมต่อได้
}

void loop() {
  float temperature = dht.readTemperature();   // อ่านค่า อุณหภูมิ DHT22 ตัวแรก
  float humidity = dht.readHumidity();         // อ่านค่า ความชื้น DHT22 ตัวแรก
  float temperature2 = dht2.readTemperature();
  float humidity2 = dht2.readHumidity();
  int sensorsoil  = map(analogRead(Soil), 0, 1023, 100, 0 ); // อ่านค่า soil sensor

  if (isnan(temperature) || isnan(humidity)) return;   // หากอ่าน DHT22 ตัวแรก ไม่ได้ ให้เริ่มใหม่
  if (isnan(temperature2) || isnan(humidity2)) return; // หากอ่าน DHT22 ตัวที่2  ไม่ได้ ให้เริ่มใหม่

  // แสดงค่าที่อ่านได้ทั้งหมดออกหน้าจอ Serial Monitor
  Serial.printf("T:%.2f ; H:%.2f ; T2:%.2f ; H2:%.2f ; soil:%d\n", 
           temperature, humidity, temperature2, humidity2 , sensorsoil);

  // ตัวอย่าง ค่าที่อ่านได้ ออกจอ lcd
  lcd.setCursor(0, 1); lcd.print("           ");
  lcd.setCursor(0, 1); lcd.print(temperature);
  lcd.setCursor(0, 2); lcd.print("           ");
  lcd.setCursor(0, 2); lcd.print(humidity);
  lcd.setCursor(0, 3); lcd.print("           ");
  lcd.setCursor(0, 3); lcd.print(sensorsoil);
  
  // เตรียมข้อมูล Json
  char buf[100];
  sprintf(buf, "{\"temperature\":%.2f,\"humidity\":%.2f}", temperature, humidity);
  String logDHTJson1 = String(buf);
  sprintf(buf, "{\"temperature\":%.2f,\"humidity\":%.2f}", temperature2, humidity2);
  String logDHTJson2 = String(buf);

  // ส่งข้อมูล json ขึ้น Firebase ด้วยฟังกชั่น TD_Set_Firebase() แบบไม่ต้องพึ่งไลบรารี่
  TD_Set_Firebase( "logDHT", logDHTJson1);    // ส่งขึ้น Firebase แบบปกติ
  TD_Set_Firebase( "logDHT2", logDHTJson2);   // ส่งขึ้น Firebase แบบปกติ
  delay(500);
}

//----------- ข้างล่างนี้ไม่ต้องแก้ไขอะไร-----------------------------------
/**********************************************************
 * ฟังกชั่น TD_Set_Firebase 
 * ใช้กำหนด ค่า value ให้ path ของ Firebase
 * โดย path อยู่ใน รูปแบบ เช่น "Room/Sensor/DHT/Humid" เป็นต้น
 * 
 * ทั้ง path และ  value ต้องเป็น ข้อมูลประเภท String
 * และ คืนค่าฟังกชั่น กลับมาด้วย http code
 * 
 * เช่น หากเชื่อมต่อไม่ได้ จะคินค่าด้วย 404 
 * หากกำหนดลงที่ Firebase สำเร็จ จะคืนค่า 200 เป็นต้น
 * 
 **********************************************************/
#ifndef FIREBASE_FINGERPRINT  // Fingerprint ของ https://www.firebaseio.com
// ใช้ได้ตั้งแต่ 01/08/2018 17:21:49 GMT ถึง หมดอายุสิ้นสุด 27/03/2019 00:00:00 GMT
#define FIREBASE_FINGERPRINT  "6F D0 9A 52 C0 E9 E4 CD A0 D3 02 A4 B7 A1 92 38 2D CA 2F 26"
#endif

int TD_Set_Firebase(String path, String value, bool push ) {
  WiFiClientSecure ssl_client;
  String host = String(FIREBASE_HOST); host.replace("https://", "");
  if(host[host.length()-1] == '/' ) host = host.substring(0,host.length()-1);
  String resp = "";
  int httpCode = 404; // Not Found
  String firebase_method = (push)? "POST " : "PUT ";
  if( ssl_client.connect( host.c_str(), 443)){
    if( ssl_client.verify( FIREBASE_FINGERPRINT, host.c_str())){
      String uri = ((path[0]!='/')? String("/"):String("")) + path + String(".json?auth=") + String(FIREBASE_AUTH);      
      String request = "";
            request += firebase_method + uri +" HTTP/1.1\r\n";
            request += "Host: " + host + "\r\n";
            request += "User-Agent: ESP8266\r\n";
            request += "Connection: close\r\n";
            request += "Accept-Encoding: identity;q=1,chunked;q=0.1,*;q=0\r\n";
            request += "Content-Length: "+String( value.length())+"\r\n\r\n";
            request += value;

      ssl_client.print(request);
      while( ssl_client.connected() && !ssl_client.available()) delay(10);
      if( ssl_client.connected() && ssl_client.available() ) {
        resp      = ssl_client.readStringUntil('\n');
        httpCode  = resp.substring(resp.indexOf(" ")+1, resp.indexOf(" ", resp.indexOf(" ")+1)).toInt();
      }
    }else{
      Serial.println("[Firebase] can't verify SSL fingerprint");
    }
    ssl_client.stop();    
  } else {
    Serial.println("[Firebase] can't connect to Firebase Host");
  }
  return httpCode;
}

/**********************************************************
 * ฟังกชั่น TD_Push_Firebase 
 * ใช้ กำหนด ค่า value ให้ path ของ Firebase แบบ Pushing Data (POST method)
 * โดย path อยู่ใน รูปแบบ เช่น "Room/Sensor/DHT/Humid" เป็นต้น
 * 
 * ทั้ง path และ  value ต้องเป็น ข้อมูลประเภท String
 * และ คืนค่าฟังกชั่น กลับมาด้วย http code
 * 
 * เช่น หากเชื่อมต่อไม่ได้ จะคินค่าด้วย 404 
 * หากกำหนดลงที่ Firebase สำเร็จ จะคืนค่า 200 เป็นต้น
 * 
 **********************************************************/
int TD_Push_Firebase(String path, String value ){
  return TD_Set_Firebase(path,value, true);
}
/**********************************************************
 * ฟังกชั่น TD_Get_Firebase
 * ใช้ รับ ค่า value ของ path ที่อยู่บน Firebase
 * โดย path อยู่ใน รูปแบบ เช่น "Room/Sensor/DHT/Humid" เป็นต้น
 * 
 * path เป็น ข้อมูลประเภท String
 * คืนค่าของฟังกชั่น คือ value ของ path ที่กำหนด ในข้อมูลประเภท String
 * 
 **********************************************************/
String TD_Get_Firebase(String path ) {
  WiFiClientSecure ssl_client;
  String host = String(FIREBASE_HOST); host.replace("https://", "");
  if(host[host.length()-1] == '/' ) host = host.substring(0,host.length()-1);
  String resp = "";
  String value = "";
  if( ssl_client.connect( host.c_str(), 443)){
    if( ssl_client.verify( FIREBASE_FINGERPRINT, host.c_str())){
      String uri = ((path[0]!='/')? String("/"):String("")) + path + String(".json?auth=") + String(FIREBASE_AUTH);      
      String request = "";
            request += "GET "+ uri +" HTTP/1.1\r\n";
            request += "Host: " + host + "\r\n";
            request += "User-Agent: ESP8266\r\n";
            request += "Connection: close\r\n";
            request += "Accept-Encoding: identity;q=1,chunked;q=0.1,*;q=0\r\n\r\n";

      ssl_client.print( request);
      while( ssl_client.connected() && !ssl_client.available()) delay(10);
      if( ssl_client.connected() && ssl_client.available() ) {
        while( ssl_client.available()) resp += (char)ssl_client.read();
        value = resp.substring( resp.lastIndexOf('\n')+1);
      }
    }else{
      Serial.println("[Firebase] can't verify SSL fingerprint");
    }
    ssl_client.stop();    
  } else {
    Serial.println("[Firebase] can't connect to Firebase Host");
  }
  return value;
}
