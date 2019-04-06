#include <ESP8266WiFi.h>
#include <DHT.h>                // ใช้ของ Adafruit

//--------------------------------------------------------
#define WIFI_SSID             "---------------"   // ชื่อ WiFi Router
#define WIFI_PASSWORD         "---------------"   // รหัส WiFi
#define FIREBASE_HOST         "https://--------------.firebaseio.com/"
#define FIREBASE_AUTH         "--------------------------------------"
#define LINE_TOKEN            "---------------"   // รหัส Line Token
//--------------------------------------------------------
/* ฟังกชั่น สำหรับ รับและส่งข้อมูลไปยัง Firebase*/
String  TD_Get_Firebase(String path );                                // สำหรับรับ
int     TD_Set_Firebase(String path, String value, bool push=false ); // สำหรับส่ง
int     TD_Push_Firebase(String path, String value ); // สำหรับส่งแบบ Pushing data
// ฟังกชั่นสำหรับส่ง LineNotify ใช้ร่วมกับ การส่ง Firebase ได้สบายๆ
bool    TD_LineNotify(String message);                // ส่งสำเร็จจะคืนค่าเป็น true
//--------------------------------------------------------

DHT dht( D2, DHT11);         // ประกาศตัวแปรเซนเซอร์ dht ที่ขา GPIO5 แบบ DHT11

void setup() {
  Serial.begin(115200); Serial.println();
  dht.begin(); 
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);    // เชื่อมต่อไวไฟ
  while(!WiFi.isConnected()) delay(400);   // รอจนกว่าเชื่อมต่อสำเร็จ
  Serial.println(WiFi.localIP());          // แสดงค่า IP หลังเชื่อมต่อได้

  TD_LineNotify("Hello");                  // ทดสอบส่ง LineNotify ออกไป
}

uint32_t timer;
uint16_t cnt=0;
void loop() {
  if( millis() -timer > 3000) {                  // กำหนดส่งทุกๆ 3 วินาที
    timer = millis();
    
    float temperature = dht.readTemperature();   // อ่านค่า อุณหภูมิ DHT
    float humidity = dht.readHumidity();         // อ่านค่า ความชื้น DHT
  
    if (isnan(temperature) || isnan(humidity)) return;   // หากอ่าน DHT ไม่ได้ ให้เริ่มรอบใหม่
  
    // เตรียมข้อมูล Json
    char buf[100];
    sprintf(buf, "{\"temperature\":%.2f,\"humidity\":%.2f}", temperature, humidity);
    String logDHTJson = String(buf);

    // จัดแสดงค่าเซนเซอร์ที่อ่านได้ ในรูปแบบ json และ ส่งค่านี้ไปตามโปรโตคอลต่างๆ
    Serial.printf("[%d] %s\n", ++cnt, logDHTJson.c_str()); // แสดง ค่าเซนเซอร์ที่อ่านได้ ออกทาง Serial Monitor
    TD_Set_Firebase( "logDHT", logDHTJson);                // ส่ง ไปยัง Firebase ที่ logDHT
    TD_LineNotify(logDHTJson);                             // ส่ง ไปยัง Line
  } 
}

//----------- ข้างล่างนี้ไม่ต้องแก้ไขอะไร-----------------------------------
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
#ifndef FIREBASE_FINGERPRINT  
// Fingerprint ของ https://www.firebaseio.com 
// ใช้ได้ตั้งแต่ 01/08/2018 17:21:49 GMT ถึง หมดอายุสิ้นสุด 27/03/2019 00:00:00 GMT
#define FIREBASE_FINGERPRINT "E2 34 53 7A 1E D9 7D B8 C5 02 36 0D B2 77 9E 5E 0F 32 71 17"
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
/**********************************************************
 * ฟังกชั่น TD_LineNotify สำหรับส่งเตือนข้อความไปทาง Line
 * หากส่งสำเร็จ คืนค่าเป็น true หากไม่ คืนค่าเป็น false
 **********************************************************/
bool TD_LineNotify(String message){
  WiFiClientSecure ssl_client;

  if (!ssl_client.connect("notify-api.line.me", 443)) {
    Serial.println("[LineNotify] can't connect to notify-api.line.me");
    return false;
  }
  int    httpCode = 404;
  String body = "message=" + message;
  String request = "";
        request += "POST /api/notify HTTP/1.1\r\n";
        request += "Host: notify-api.line.me\r\n";
        request += "Authorization: Bearer " + String(LINE_TOKEN) + "\r\n";
        request += "Cache-Control: no-cache\r\n";
        request += "User-Agent: ESP8266/ESP32\r\n";
        request += "Connection: close\r\n";
        request += "Content-Type: application/x-www-form-urlencoded\r\n";
        request += "Content-Length: " + String(body.length()) + "\r\n\r\n" + body;
  ssl_client.print(request);

  while( ssl_client.connected() && !ssl_client.available()) delay(10);
  if( ssl_client.connected() && ssl_client.available() ) {
    String resp = ssl_client.readStringUntil('\n');
    httpCode    = resp.substring(resp.indexOf(" ")+1, resp.indexOf(" ", resp.indexOf(" ")+1)).toInt();
  }
  ssl_client.stop();    
  return (httpCode == 200);
}
