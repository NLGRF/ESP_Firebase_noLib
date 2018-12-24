#include <WiFiClientSecure.h>

//--------------------------------------------------------
#define SSID                  "---------------"  // ชื่อ Internet WiFi Router
#define PASSWORD              "---------------"  // รหัสลับของ WiFi Router

#define FIREBASE_HOST         "https://------------.firebaseio.com/"  // กำหนด host ของฐานข้อมูลบน Firebase
#define FIREBASE_AUTH         "----xxxxxx-xxxx--xxxxx---xxxx-xxxx--"  
//--------------------------------------------------------
/* ฟังกชั่น สำหรับ รับและส่งข้อมูลไปยัง Firebase ใช้สำหรับ ESP32 */
String  TD32_Get_Firebase(String path );               // รับค่า path จาก Firebase
int     TD32_Set_Firebase(String path, String value, bool push=false ); // ส่งค่าขึ้น Firebase  (ทับข้อมูลเดิมใน path เดิม)
int     TD32_Push_Firebase(String path, String value); // ส่งค่าขึ้น Firebase แบบ Pushing data  (เพิ่มเข้าไปใหม่เรื่อยๆใน path เดิม)
//--------------------------------------------------------

void setup() {
  Serial.begin(115200); Serial.println();
  WiFi.begin(SSID, PASSWORD);                       // ทำการเชื่อมต่อ WiFi
  while(WiFi.status()!= WL_CONNECTED ) delay(400);  // รอจนกว่าจะเชื่อมต่อ WiFi ได้
  Serial.println();
  Serial.println(WiFi.localIP());                   // แสดงค่า IP หลังเชื่อมต่อ WiFi สำเร็จ

  TD32_Set_Firebase("name", "\"Trident_ESP32\"");   // กำหนด ค่า "Trident_ESP32" ให้กับ ตัวแปร name บน Firebase
  Serial.println( TD32_Get_Firebase("name"));       // ดึงค่าของ name บน Firebase กลับลงมาแสดงบน ESP32
}

int cnt;
uint32_t timer;
void loop() {
  if( millis() -timer > 3000) {       // ทำการอ่านค่าและส่งทุกๆ 3 วินาที
    timer = millis();
    
    float t = (float)random(2000,4000)/100;  // อ่านค่า อุณหภูมิ (สมมติใช้ค่า random แทน)
    float h = (float)random(1000,9000)/100;  // อ่านค่า ความชื้น (สมมติใช้ค่า random แทน)

    TD32_Set_Firebase("Sensor/Temp", String(t));  // ตั้งค่า อุณหภูมิไปยัง Firebase ที่ Sensor/Temp
    TD32_Push_Firebase("Sensor/Humid", String(h)); // ตั้งค่า อุณหภูมิไปยัง Firebase ที่ Sensor/Humid แบบ Pushing data

    /* ตัวอย่าง อ่านค่า จาก Firebase ลงมายัง ESP32 */
    Serial.println(TD32_Get_Firebase("Sensor/Temp"));  // อ่านค่า Sensor/Temp จาก Firebase มาแสดง
    Serial.println(TD32_Get_Firebase("Sensor/Humid")); // อ่านค่า Sensor/Humid จาก Firebase มาแสดง
    Serial.printf("(%d) Temp : %.2f ; Humid : %.2f\n", ++cnt, t, h);  // แสดงค่าที่อ่านได้ทาง Serial Monitor
  }
}

/**********************************************************
 * ฟังกชั่น TD32_Set_Firebase
 * สำหรับ ESP32 ใช้กำหนด ค่า value ให้ path ของ Firebase
 * โดย path อยู่ใน รูปแบบ เช่น "Room/Sensor/DHT/Humid" เป็นต้น
 * 
 * ทั้ง path และ  value ต้องเป็น ข้อมูลประเภท String
 * และ คืนค่าฟังกชั่น กลับมาด้วย http code
 * 
 * เช่น หากเชื่อมต่อไม่ได้ จะคืนค่าด้วย 404 
 * หากกำหนดลงที่ Firebase สำเร็จ จะคืนค่า 200 เป็นต้น
 * 
 **********************************************************/
// Root CA ของ https://www.firebaseio.com  
// ใช้ได้ตั้งแต่ 01/08/2018 17:21:49 GMT ถึง หมดอายุสิ้นสุด 27/03/2019 00:00:00 GMT
const char* FIREBASE_ROOT_CA= \
        "-----BEGIN CERTIFICATE-----\n" \
        "MIIEXDCCA0SgAwIBAgINAeOpMBz8cgY4P5pTHTANBgkqhkiG9w0BAQsFADBMMSAw\n" \
        "HgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMjETMBEGA1UEChMKR2xvYmFs\n" \
        "U2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjAeFw0xNzA2MTUwMDAwNDJaFw0yMTEy\n" \
        "MTUwMDAwNDJaMFQxCzAJBgNVBAYTAlVTMR4wHAYDVQQKExVHb29nbGUgVHJ1c3Qg\n" \
        "U2VydmljZXMxJTAjBgNVBAMTHEdvb2dsZSBJbnRlcm5ldCBBdXRob3JpdHkgRzMw\n" \
        "ggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDKUkvqHv/OJGuo2nIYaNVW\n" \
        "XQ5IWi01CXZaz6TIHLGp/lOJ+600/4hbn7vn6AAB3DVzdQOts7G5pH0rJnnOFUAK\n" \
        "71G4nzKMfHCGUksW/mona+Y2emJQ2N+aicwJKetPKRSIgAuPOB6Aahh8Hb2XO3h9\n" \
        "RUk2T0HNouB2VzxoMXlkyW7XUR5mw6JkLHnA52XDVoRTWkNty5oCINLvGmnRsJ1z\n" \
        "ouAqYGVQMc/7sy+/EYhALrVJEA8KbtyX+r8snwU5C1hUrwaW6MWOARa8qBpNQcWT\n" \
        "kaIeoYvy/sGIJEmjR0vFEwHdp1cSaWIr6/4g72n7OqXwfinu7ZYW97EfoOSQJeAz\n" \
        "AgMBAAGjggEzMIIBLzAOBgNVHQ8BAf8EBAMCAYYwHQYDVR0lBBYwFAYIKwYBBQUH\n" \
        "AwEGCCsGAQUFBwMCMBIGA1UdEwEB/wQIMAYBAf8CAQAwHQYDVR0OBBYEFHfCuFCa\n" \
        "Z3Z2sS3ChtCDoH6mfrpLMB8GA1UdIwQYMBaAFJviB1dnHB7AagbeWbSaLd/cGYYu\n" \
        "MDUGCCsGAQUFBwEBBCkwJzAlBggrBgEFBQcwAYYZaHR0cDovL29jc3AucGtpLmdv\n" \
        "b2cvZ3NyMjAyBgNVHR8EKzApMCegJaAjhiFodHRwOi8vY3JsLnBraS5nb29nL2dz\n" \
        "cjIvZ3NyMi5jcmwwPwYDVR0gBDgwNjA0BgZngQwBAgIwKjAoBggrBgEFBQcCARYc\n" \
        "aHR0cHM6Ly9wa2kuZ29vZy9yZXBvc2l0b3J5LzANBgkqhkiG9w0BAQsFAAOCAQEA\n" \
        "HLeJluRT7bvs26gyAZ8so81trUISd7O45skDUmAge1cnxhG1P2cNmSxbWsoiCt2e\n" \
        "ux9LSD+PAj2LIYRFHW31/6xoic1k4tbWXkDCjir37xTTNqRAMPUyFRWSdvt+nlPq\n" \
        "wnb8Oa2I/maSJukcxDjNSfpDh/Bd1lZNgdd/8cLdsE3+wypufJ9uXO1iQpnh9zbu\n" \
        "FIwsIONGl1p3A8CgxkqI/UAih3JaGOqcpcdaCIzkBaR9uYQ1X4k2Vg5APRLouzVy\n" \
        "7a8IVk6wuy6pm+T7HT4LY8ibS5FEZlfAFLSW8NwsVz9SBK2Vqn1N0PIMn5xA6NZV\n" \
        "c7o835DLAFshEWfC7TIe3g==\n" \
        "-----END CERTIFICATE-----\n";

int TD32_Set_Firebase(String path, String value, bool push ) {
  WiFiClientSecure ssl_client;
  String host = String(FIREBASE_HOST); host.replace("https://", "");
  if(host[host.length()-1] == '/' ) host = host.substring(0,host.length()-1);
  String resp = "";
  int httpCode = 404; // Not Found

  String firebase_method = (push)? "POST " : "PUT ";
  ssl_client.setCACert(FIREBASE_ROOT_CA);
  if( ssl_client.connect( host.c_str(), 443)){
    String uri = ((path[0]!='/')? String("/"):String("")) + path + String(".json?auth=") + String(FIREBASE_AUTH);      
    String request = "";
          request +=  firebase_method + uri +" HTTP/1.1\r\n";
          request += "Host: " + host + "\r\n";
          request += "User-Agent: TD_ESP32\r\n";
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
    ssl_client.stop();    
  }
  else {
    Serial.println("[Firebase] can't connect to Firebase Host");
  }
  return httpCode;
}

/**********************************************************
 * ฟังกชั่น TD32_Push_Firebase
 * สำหรับ ESP32 ใช้กำหนด ค่า value ให้ path ของ Firebase
 * แบบ Pushing data (เติมเข้าไปที่ path เรื่อยๆ ไม่ทับของเดิม)
 * โดย path อยู่ใน รูปแบบ เช่น "Room/Sensor/DHT/Humid" เป็นต้น
 * 
 * ทั้ง path และ  value ต้องเป็น ข้อมูลประเภท String
 * และ คืนค่าฟังกชั่น กลับมาด้วย http code
 * 
 * เช่น หากเชื่อมต่อไม่ได้ จะคืนค่าด้วย 404 
 * หากกำหนดลงที่ Firebase สำเร็จ จะคืนค่า 200 เป็นต้น
 * 
 **********************************************************/
int TD32_Push_Firebase(String path, String value){
    return TD32_Set_Firebase(path,value,true);
}
/**********************************************************
 * ฟังกชั่น TD32_Get_Firebase 
 * ใช้สำหรับ EPS32 รับ ค่า value ของ path ที่อยู่บน Firebase
 * โดย path อยู่ใน รูปแบบ เช่น "Room/Sensor/DHT/Humid" เป็นต้น
 * 
 * path เป็น ข้อมูลประเภท String
 * คืนค่าของฟังกชั่น คือ value ของ path ที่กำหนด ในข้อมูลประเภท String
 * 
 **********************************************************/
String TD32_Get_Firebase(String path ) {
  WiFiClientSecure ssl_client;
  String host = String(FIREBASE_HOST); host.replace("https://", "");
  if(host[host.length()-1] == '/' ) host = host.substring(0,host.length()-1);
  String resp = "";
  String value = "";
  ssl_client.setCACert(FIREBASE_ROOT_CA);
  if( ssl_client.connect( host.c_str(), 443)){
    String uri = ((path[0]!='/')? String("/"):String("")) + path + String(".json?auth=") + String(FIREBASE_AUTH);      
    String request = "";
          request += "GET "+ uri +" HTTP/1.1\r\n";
          request += "Host: " + host + "\r\n";
          request += "User-Agent: TD_ESP32\r\n";
          request += "Connection: close\r\n";
          request += "Accept-Encoding: identity;q=1,chunked;q=0.1,*;q=0\r\n\r\n";

    ssl_client.print( request);
    while( ssl_client.connected() && !ssl_client.available()) delay(10);
    if( ssl_client.connected() && ssl_client.available() ) {
      while( ssl_client.available()) resp += (char)ssl_client.read();
      value = resp.substring( resp.lastIndexOf('\n')+1, resp.length()-1);
    }
    ssl_client.stop();    
  } else {
    Serial.println("[Firebase] can't connect to Firebase Host");
  }
  return value;
}
