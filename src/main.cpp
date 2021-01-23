/*
ESP32-CAM Save a captured photo(Base64) to firebase. 
Author : ChungYi Fu (Kaohsiung, Taiwan)  2019-8-16 23:00
https://www.facebook.com/francefu
Arduino IDE Library
Firebase ESP32 Client by Mobizt version 3.2.1
ESP32-CAM How to save a captured photo to Firebase
https://youtu.be/Hx7bdpev1ug
How to set up Firebase
https://iotdesignpro.com/projects/iot-controlled-led-using-firebase-database-and-esp32
*/

#include <Arduino.h>
#include <string>

#include "credentials.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "Base64.h"
#include <base64.h>
#include "esp_camera.h"

// const char* ssid = "";
// const char* password = "";

//https://console.firebase.google.com/project/xxxxxxxxxx/settings/serviceaccounts/databasesecrets
// String FIREBASE_HOST = "";
// String FIREBASE_AUTH = "";

#include "FirebaseESP32.h"
FirebaseData firebaseData;

#include <FirebaseJson.h>
FirebaseJson json;

DynamicJsonDocument doc(4096);


// WARNING!!! Make sure that you have either selected ESP32 Wrover Module,
//            or another board which has PSRAM enabled

//CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

const char* firestoreURL = "https://firestore.googleapis.com/v1/projects/esp8266-f2775/databases/(default)/documents/timelapper/";
const char* epochTime = "160121";
char fullURL[128];

long prev_millis = 0;
int capture_interval = 30000;

camera_config_t config;

String urlencode(String str);
String Photo2Base64();
String httpGETRequest(char* url);
int httpPOSTRequest(char* json, char* url);
int httpPATCHRequest(String json, char* url);
void packageJson(DynamicJsonDocument doc, const String& base64image);

void configInitCamera(){
	config.ledc_channel = LEDC_CHANNEL_0;
	config.ledc_timer = LEDC_TIMER_0;
	config.pin_d0 = Y2_GPIO_NUM;
	config.pin_d1 = Y3_GPIO_NUM;
	config.pin_d2 = Y4_GPIO_NUM;
	config.pin_d3 = Y5_GPIO_NUM;
	config.pin_d4 = Y6_GPIO_NUM;
	config.pin_d5 = Y7_GPIO_NUM;
	config.pin_d6 = Y8_GPIO_NUM;
	config.pin_d7 = Y9_GPIO_NUM;
	config.pin_xclk = XCLK_GPIO_NUM;
	config.pin_pclk = PCLK_GPIO_NUM;
	config.pin_vsync = VSYNC_GPIO_NUM;
	config.pin_href = HREF_GPIO_NUM;
	config.pin_sscb_sda = SIOD_GPIO_NUM;
	config.pin_sscb_scl = SIOC_GPIO_NUM;
	config.pin_pwdn = PWDN_GPIO_NUM;
	config.pin_reset = RESET_GPIO_NUM;
	config.xclk_freq_hz = 20000000;
	config.pixel_format = PIXFORMAT_JPEG; //YUV422,GRAYSCALE,RGB565,JPEG

	// Select lower framesize if the camera doesn't support PSRAM
	if(psramFound()){
		Serial.println("PSRAM SUPPORTED");
		config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
		config.jpeg_quality = 10; //10-63 lower number means higher quality
		config.fb_count = 2;
	} else {
		Serial.println("PSRAM NOT SUPPORTED");
		config.frame_size = FRAMESIZE_SVGA;
		config.jpeg_quality = 12;
		config.fb_count = 1;
	}
	
	// Initialize the Camera
	esp_err_t err = esp_camera_init(&config);
	if (err != ESP_OK) {
		Serial.printf("Camera init failed with error 0x%x", err);
		delay(1000);
		ESP.restart();
	}

	sensor_t * s = esp_camera_sensor_get();
	// s->set_brightness(s, 0);     // -2 to 2
	// s->set_contrast(s, 0);       // -2 to 2
	// s->set_saturation(s, 0);     // -2 to 2
	// s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
	// s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
	// s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
	// s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
	// s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
	// s->set_aec2(s, 0);           // 0 = disable , 1 = enable
	// s->set_ae_level(s, 0);       // -2 to 2
	// s->set_aec_value(s, 300);    // 0 to 1200
	// s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
	// s->set_agc_gain(s, 0);       // 0 to 30
	// s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
	// s->set_bpc(s, 0);            // 0 = disable , 1 = enable
	// s->set_wpc(s, 1);            // 0 = disable , 1 = enable
	// s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
	// s->set_lenc(s, 1);           // 0 = disable , 1 = enable
	// s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
	// s->set_vflip(s, 0);          // 0 = disable , 1 = enable
	// s->set_dcw(s, 1);            // 0 = disable , 1 = enable
	// s->set_colorbar(s, 0);       // 0 = disable , 1 = enable

	// s->set_framesize(s, FRAMESIZE_QQVGA); // VGA|CIF|QVGA|HQVGA|QQVGA   ( UXGA? SXGA? XGA? SVGA? )
}


void setup() { 
	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

	// define flash LED
	pinMode(4, OUTPUT);
		
	Serial.begin(115200);
	Serial.setDebugOutput(true);
	Serial.println();
	Serial.println("ssid: " + (String)WIFI_SSID);
	Serial.println("password: " + (String)WIFI_PASS);
	
	WiFi.begin(WIFI_SSID, WIFI_PASS);

	long int StartTime = millis();
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		if ((StartTime+10000) < millis()) break;
	} 

	if (WiFi.status() == WL_CONNECTED) {
		Serial.println(""); 
		Serial.print("Camera Ready! Use 'http://");
		Serial.print(WiFi.localIP());
		Serial.println("' to connect");
		WiFi.softAP((WiFi.localIP().toString()+"_"+(String)AP_SSID).c_str(), AP_PASS);            
	}
	else {
		Serial.println("connection failed, restarting uwu");

		ESP.restart();
		// return;
	} 

	configInitCamera();

	
	Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
	Firebase.reconnectWiFi(true);
	Firebase.setMaxRetry(firebaseData, 3);
	Firebase.setMaxErrorQueue(firebaseData, 30); 
	Firebase.enableClassicRequest(firebaseData, true);
  
	strcpy(fullURL, firestoreURL);
	strcat(fullURL, epochTime);
} 
 
void loop() {

	// put your main code here, to run repeatedly:
	if ((millis() - prev_millis) > capture_interval) {
    	//Check WiFi connection status
		prev_millis = millis();
		if (WiFi.status() == WL_CONNECTED) {
			// Serial.println(httpGETRequest(serverName));
			// https://arduinojson.org/v6/how-to/use-arduinojson-with-httpclient/
			// https://www.arduino.cc/reference/en/language/variables/data-types/string/functions/reserve/
			// // doc["1610770215830"] = 'world';
			// // doc["name"] = "projects/esp8266-f2775/databases/(default)/documents/timelapper/";

			packageJson(doc, Photo2Base64());

			// String jsonstring;
			// serializeJson(doc, jsonstring);
			// Serial.println(jsonstring);

			// Serial.println(Photo2Base64());
			// char imgPayload[000] = "{\"fields\": { \"1610770215830\": {\"stringValue\": ";
			// // String imgPayload = "{\"fields\": { \"1610770215830\": {\"stringValue\": " + Photo2Base64() + "}}}";
			// strcat(imgPayload, Photo2Base64().c_str());
			// strcat(imgPayload, "}}}");

			// Serial.println(imgPayload);

			// Serial.println(imgPayload);
			// httpPATCHRequest(imgPayload, fullURL);
			doc.clear();
		}
		else {
			Serial.println("WiFi Disconnected");
		}
		
	}
	// json.add("photo", Photo2Base64());
	// String photoPath = "/esp32-cam";
	// if (Firebase.pushJSON(firebaseData, photoPath, json, 1.0)) {
	// 	Serial.println(firebaseData.dataPath());
	// 	Serial.println(firebaseData.pushName());
	// 	Serial.println(firebaseData.dataPath() + "/"+ firebaseData.pushName());
	// } else {
	// 	Serial.println(firebaseData.errorReason());
	// }
	// delay(5000);
}

void packageJson(DynamicJsonDocument doc, const String& base64image){
	Serial.println(base64image.c_str());
	// doc["fields"]["1610770215830"]["stringValue"] = base64image.c_str();
}

//https://github.com/zenmanenergy/ESP8266-Arduino-Examples/
String urlencode(String str) {
	String encodedString="";
	char c;
	char code0;
	char code1;
	char code2;
	for (int i =0; i < str.length(); i++){
		c=str.charAt(i);
		if (c == ' '){
			encodedString+= '+';
		} else if (isalnum(c)){
			encodedString+=c;
		} else{
			code1=(c & 0xf)+'0';
			if ((c & 0xf) >9){
				code1=(c & 0xf) - 10 + 'A';
			}
			c=(c>>4)&0xf;
			code0=c+'0';
			if (c > 9){
				code0=c - 10 + 'A';
			}
			code2='\0';
			encodedString+='%';
			encodedString+=code0;
			encodedString+=code1;
			//encodedString+=code2;
		}
		yield();
	}
	return encodedString;
}

String Photo2Base64() {
	digitalWrite(4, HIGH);
	camera_fb_t * fb = NULL;
	// delay(500);
	fb = esp_camera_fb_get();  
	if(!fb) {
		digitalWrite(4, LOW);
		Serial.println("Camera capture failed");
		// ESP.restart();
		return "";
	}
	digitalWrite(4, LOW);
	String imageFile = "data:image/jpeg;base64,";
	char *input = (char *)fb->buf;
	char output[base64_enc_len(3)];
	for (int i=0;i<fb->len;i++) {
		base64_encode(output, (input++), 3);
		if (i%3==0) imageFile += urlencode(String(output));
	}
	esp_camera_fb_return(fb);
	
	return imageFile;
}


String httpGETRequest(char* url) {
	HTTPClient http;
	// Your IP address with path or Domain name with URL path 
	http.begin(url);
	
	// Send HTTP POST request
	int httpResponseCode = http.GET();
	
	String payload = "{}";
	
	if (httpResponseCode > 0) {
		Serial.print("HTTP Response code: ");
		Serial.println(httpResponseCode);
		payload = http.getString();
	}
	else {
		Serial.print("Error code: ");
		Serial.println(httpResponseCode);
	}
	// Free resources
	http.end();

	return payload;
}

int httpPOSTRequest(String json, char* url){
	// String json;
	// serializeJson(jsonDoc, json);

	HTTPClient http;
    // Your Domain name with URL path or IP address with path
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    // String json = "{\"api_key\":\"tPmAT5Ab3j7F9\",\"sensor\":\"BME280\",\"value1\":\"24.25\",\"value2\":\"49.54\",\"value3\":\"1005.14\"}";
    int httpResponseCode = http.POST(json);
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
	Serial.println(http.getString()); // print response
    // Free resources	
    http.end();

	return httpResponseCode;
}

int httpPATCHRequest(String json, char* url){
	// String json;
	// serializeJson(jsonDoc, json);

	HTTPClient http;
    // Your Domain name with URL path or IP address with path
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    // String json = "{\"api_key\":\"tPmAT5Ab3j7F9\",\"sensor\":\"BME280\",\"value1\":\"24.25\",\"value2\":\"49.54\",\"value3\":\"1005.14\"}";
    int httpResponseCode = http.PATCH(json);
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
	Serial.println(http.getString()); // print response
    // Free resources	
    http.end();

	return httpResponseCode;
}





// void classifyImage() {
//   String response;
  
//   // Capture picture
//   camera_fb_t * fb = NULL;
//   fb = esp_camera_fb_get();
   
//   if(!fb) {
//     Serial.println("Camera capture failed");
//     return;
//   } else {
//     Serial.println("Camera capture OK");
//   }

//   size_t size = fb->len;
//   String buffer = base64::encode((uint8_t *) fb->buf, fb->len);
  
//   String imgPayload = "{\"fields\": { \"1610770215830\": {\"stringValue\": " + buffer + "\"}}}";

//   buffer = "";
//   // Uncomment this if you want to show the payload
//   Serial.println(imgPayload);

//   esp_camera_fb_return(fb);
  
//   // Generic model
//   String model_id = "General";

//   HTTPClient http;
//   http.begin("https://api.clarifai.com/v2/models/" + model_id + "/outputs");
//   http.addHeader("Content-Type", "application/json");     
//   http.addHeader("Authorization", "c7f894790533332388e23d4d21278321"); 
//   int httpResponseCode = http.POST(imgPayload);

//   if(httpResponseCode>0){
//     Serial.print(httpResponseCode);
//     Serial.print(" Returned String: ");
//     Serial.println(http.getString());
//   } else {      
//     Serial.print("POST Error: ");
//     Serial.print(httpResponseCode);
//   }                      
 
//   // Parse the json response: Arduino assistant
//   const int jsonSize = JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(20) + 3*JSON_OBJECT_SIZE(1) + 6*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + 20*JSON_OBJECT_SIZE(4) + 2*JSON_OBJECT_SIZE(6);
  
//   StaticJsonDocument<jsonSize> doc;
//   // Deserialize the JSON document
//   DeserializationError error = deserializeJson(doc, response); 
//   // Test if parsing succeeds.
//   if (error) {
//     Serial.print(F("deserializeJson() failed: "));
//     Serial.println(error.f_str());
//     return;
//   }  

//   Serial.println(jsonSize);
//   Serial.println(response);

//   for (int i=0; i < 10; i++) {
// //    const name = doc["outputs"][0]["data"]["concepts"][i]["name"];
// //    const float p = doc["outputs"][0]["data"]["concepts"][i]["value"];

//     const char* name = doc["outputs"][0]["data"]["concepts"][i]["name"];
//     const char* p = doc["outputs"][0]["data"]["concepts"][i]["value"];    
    
//     Serial.println("=====================");
//     Serial.print("Name:");
//     Serial.println(name[i]);
//     Serial.print("Prob:");
//     Serial.println(p);
//     Serial.println();
//   }
// }
