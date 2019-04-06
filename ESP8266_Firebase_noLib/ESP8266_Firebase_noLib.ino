#include <ESP8266WiFi.h>

//--------------------------------------------------------
#define SSID          "----------" // ชื่อ Internet WiFi Router
#define PASSWORD      "----------" // รหัสลับของ WiFi Router

#define FIREBASE_HOST "https://--------------------------/"
#define FIREBASE_AUTH "***********************************"
//--------------------------------------------------------
/* ฟังกชั่น สำหรับ ESP8266 รับและส่งข้อมูลไปยัง Firebase*/
String  TD_Get_Firebase(String path );                                // สำหรับรับ
int     TD_Set_Firebase(String path, String value, bool push=false ); // สำหรับส่ง
int     TD_Push_Firebase(String path, String value ); // สำหรับส่งแบบ Pushing data
//--------------------------------------------------------

void setup() {
  Serial.begin(115200); Serial.println();
  WiFi.begin(SSID, PASSWORD);             // ทำการเชื่อมต่อ WiFi
  while(!WiFi.isConnected()) delay(400);  // รอจนกว่าจะเชื่อมต่อ WiFi ได้
  Serial.println(WiFi.localIP());         // แสดงค่า IP หลังเชื่อมต่อ WiFi สำเร็จ
}
int cnt;
uint32_t timer;
void loop() {
  if( millis() -timer > 3000) {       // ทำการอ่านค่าและส่งทุกๆ 3 วินาที
    timer = millis();
    
    float t = (float)random(2000,4000)/100;  // อ่านค่า อุณหภูมิ (สมมติใช้ค่า random แทน)
    float h = (float)random(1000,9000)/100;  // อ่านค่า ความชื้น (สมมติใช้ค่า random แทน)

    TD_Set_Firebase("Sensor/Temp", String(t));  // ตั้งค่า อุณหภูมิไปยัง Firebase ที่ Sensor/Temp
    TD_Set_Firebase("Sensor/Humid", String(h)); // ตั้งค่า อุณหภูมิไปยัง Firebase ที่ Sensor/Humid
    Serial.println(TD_Get_Firebase("Sensor/Temp"));  // อ่านค่า Sensor/Temp จาก Firebase มาแสดง
    Serial.println(TD_Get_Firebase("Sensor/Humid")); // อ่านค่า Sensor/Humid จาก Firebase มาแสดง
    Serial.printf("(%d) Temp : %.2f ; Humid : %.2f\n", ++cnt, t, h);  // แสดงค่าที่อ่านได้ทาง Serial Monitor
  }
}

/**********************************************************
 * ฟังกชั่น TD_Set_Firebase 
 * ใช้สำหรับ ESP8266 กำหนด ค่า value ให้ path ของ Firebase
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
#define FIREBASE_FINGERPRINT "E2 34 53 7A 1E D9 7D B8 C5 02 36 0D B2 77 9E 5E 0F 32 71 17"
#endif

int TD_Set_Firebase(String path, String value , bool push) {
  WiFiClientSecure ssl_client;
  String host = String(FIREBASE_HOST); host.replace("https://", "");
  if(host[host.length()-1] == '/' ) host = host.substring(0,host.length()-1);
  String resp = "";
  int httpCode = 404; // Not Found

  String firebase_method = (push)? "POST " : "PUT ";
  if( ssl_client.connect( host, 443)){
    if( ssl_client.verify( FIREBASE_FINGERPRINT, host.c_str())){
      String uri = ((path[0]!='/')? String("/"):String("")) + path + String(".json?auth=") + String(FIREBASE_AUTH);      
      String request = "";
            request +=  firebase_method + uri +" HTTP/1.1\r\n";
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
 * ใช้สำหรับ ESP8266  กำหนด ค่า value ให้ path ของ Firebase แบบ Pushing Data (POST method)
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
 * ใช้สำหรับ ESP8266  รับ ค่า value ของ path ที่อยู่บน Firebase
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
  if( ssl_client.connect( host, 443)){
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
