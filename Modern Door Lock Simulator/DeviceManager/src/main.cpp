#include <Arduino.h>
#include <ESP32Servo.h>
#include <SPI.h>
#include <Wire.h>
#include <Keypad.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#define ROW_NUM     4 // four rows
#define COLUMN_NUM  4 // four columns
#define SERVO_PIN  22 
#define SUCCESS_LED_PIN 21
#define BUZZER_PIN    13  // ESP32 pin GIOP13, which connected to led

const char* lockKey = "D3938";

// initialization of the 4x4 keypad
byte pin_rows[ROW_NUM]      = {19, 18, 5, 17}; // GIOP19, GIOP18, GIOP5, GIOP17 connect to the row pins
byte pin_column[COLUMN_NUM] = {16, 4, 0, 2};   // GIOP16, GIOP4, GIOP0, GIOP2 connect to the column pins
char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

//initialize an instance of class Keypad
Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

//initialize an instance of class Servo
Servo servo;

bool isDoorLocked = true;

// Internet connection
const char* ssid = "BhishmaSHP";
const char* password = "c#vsjava";

// The endpoint is broker address
const char* awsEndpoint = "ahh6xadh7bhud-ats.iot.us-west-1.amazonaws.com";

const char* topic = "arn:aws:iot:us-west-1:452124643641:thing/Moisture_Sensor";
const char* subscribeTopic = "arn:aws:iot:us-west-1:452124643641:thing/Moisture_Sensor";

const char* rootCA = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n" \
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n" \
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n" \
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n" \
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n" \
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n" \
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n" \
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n" \
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n" \
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n" \
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n" \
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n" \
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n" \
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n" \
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n" \
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n" \
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n" \
"rqXRfboQnoZsG4q5WTP468SQvvG5\n" \
"-----END CERTIFICATE-----\n";

// xxxxxxxxxx-certificate.pem.crt
const char* certificate_pem_crt = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDWTCCAkGgAwIBAgIUWDWGnTRfjv+br4hQTg2ufM+W7c4wDQYJKoZIhvcNAQEL\n" \
"BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\n" \
"SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTIyMTEwMjA0Mzky\n" \
"MVoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0\n" \
"ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMDEA2JzzvZsh5MvYTqz\n" \
"VzIyaOsMTsO4pGZdJdvsRcICgtBZANfWHm5fsPDL0WpcvMnAEaVn2BNdZUc9zPWD\n" \
"kEF1gKucGaL1LeXCp6XnyOl8g9jy+Skux+q1ezEIgfwiadHF0doARHecL3oH5Cav\n" \
"2/Du+xllZEkspROgMawNLcAUVtFuqv5W/9yNeMuwuZNsPFQz6KtEHjJ7e2le66lB\n" \
"XWwUZOhTUlZ68Abfwaq8MDIhruyl0QNrnLUPDXHM2WaZ6saaZiMbLRssssxxnmvi\n" \
"THrs1w1Dpa7HTNWLTwpfX9Ewlx2RZLmiN9A5HCy1bKfev6G3F8hn68rykwIHJh4N\n" \
"Fm0CAwEAAaNgMF4wHwYDVR0jBBgwFoAUGgzpOnvW+sk5m+366FiI+iftNL8wHQYD\n" \
"VR0OBBYEFDDy9pH9RarJseJksoPvwOVbopwFMAwGA1UdEwEB/wQCMAAwDgYDVR0P\n" \
"AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQByVmKkMf3NHEosJC+essQGTb2I\n" \
"rq3BIFYZqI60hg3FZNH9DTmO5ylKnG9sJhFfUMHO75s0Qy78cIH0507jM+AtiozK\n" \
"62PDUudt+zNlTLo2D3wS/Kn7VqPyZ6bc0b0VXySNdTGOsz5hEJY6mY6vsVBsRfId\n" \
"UCM+dUKYpHFQyzMEgMWA6qG3ynk6OsAOukL9q2He4riPAu1CtPZdkBNMguR4Amcb\n" \
"lXvHAH+jk6q8chD6yFuPsXH3W1hQPkw1mWHBaKm23NTYtWOHrDNx7KWLNalyPq/M\n" \
"nIS3drIgrT4WDRSgyJKM8fqZG36xwpukO2VZOyVqV/K834/3weETv/WPR+mp\n" \
"-----END CERTIFICATE-----\n";

// xxxxxxxxxx-private.pem.key
const char* private_pem_key = \
"-----BEGIN RSA PRIVATE KEY-----\n" \
"MIIEogIBAAKCAQEAwMQDYnPO9myHky9hOrNXMjJo6wxOw7ikZl0l2+xFwgKC0FkA\n" \
"19Yebl+w8MvRaly8ycARpWfYE11lRz3M9YOQQXWAq5wZovUt5cKnpefI6XyD2PL5\n" \
"KS7H6rV7MQiB/CJp0cXR2gBEd5wvegfkJq/b8O77GWVkSSylE6AxrA0twBRW0W6q\n" \
"/lb/3I14y7C5k2w8VDPoq0QeMnt7aV7rqUFdbBRk6FNSVnrwBt/BqrwwMiGu7KXR\n" \
"A2uctQ8NcczZZpnqxppmIxstGyyyzHGea+JMeuzXDUOlrsdM1YtPCl9f0TCXHZFk\n" \
"uaI30DkcLLVsp96/obcXyGfryvKTAgcmHg0WbQIDAQABAoIBABeBDwSXKFolu5Y3\n" \
"xbw5l904BlD6rDPzNKCCpRYexha05axv3clzgzxkEJgb0goNCrvy3nPzqIYb8jhw\n" \
"kagyBYnlwO2r1/hlutX0GSsCix3lH9J5NeZOnPPCcyX0ap0KuSpp23+gXrjS43sW\n" \
"sTwCX2gOS7G6CjNd7TbqZoQbC0QLQGyiDzMRQCT2QIkbq2UV1D6lSBKumOpqapgU\n" \
"oj2LsBrk5TVvRZOAvUCTDiD03G++FZq8zIljjYRAhQkGbgVafRls3/1EKf2W0te+\n" \
"NGelXw5eL93JohfqCb+swau9mcaK1a3a2bPYmU1SeJPcUIK09jEKhnJfTAJsuamZ\n" \
"EWNjf6UCgYEA9B4pAgcRiV7IR3WboE5A7jDDFBXmvn/ksq8AIF5+CkDa5i9KRxes\n" \
"gt3uxVpTIuEUlYvqRjUql2mFuwy/AORlc1IElu5e47ZlzYEqkTbBQyPQrjYpatug\n" \
"VNYsNPW1OiwR0pO8L1vzD9T52D7NxpEPUKfDPaoBwcew2OODvqVBD4cCgYEAyiX6\n" \
"SNqNeRyHOBWWWRgF8M1gBT9TlWvwW0bntLNtUb2Y5SGmTvoMAezdKjB4g0k3ul1b\n" \
"Gt2g4CO25mJAhnikUmysUUSOb1UNng9N93W4i2IGyLNuKwN1VvYpNMhpAFGEw21V\n" \
"+bl9ZkMURrSxJMJIuab8/6sT2e55ToglDvlD32sCgYBwWo0/LKeqlOAS25Mz9bA2\n" \
"sGSokufvV4eXoamqPDNAXtcgyDUkgUXZuXFSjZ7j17oUlh/ODlw68csl6tjh/Tyg\n" \
"Feic2rr7jRBPxBomWZAbaZl7BNyNHOYvBZ+1BD1Tt8LAK1Saa3R0He6b01P8BvmI\n" \
"gp3+fIdwe4ruV9Bj50UJcwKBgEmZXC/sSrf+uAima3ASvA75Z21hqhfsDb3d6fyz\n" \
"myi4pBNAliGJDpgg2kFDFo2OOdkDpmj4qlsRBcWJnj6XbBSKdo68LDPCvst+fP+M\n" \
"/9zXmuq10XWds7ous8AA+6HfDDxu8W+TpmF2KMq5fQmKJxpY/yJC8l5VYhtCnGOA\n" \
"Q8LzAoGAWdUTKmJ1pvWOM14N8bGLl5nIARloGz4EBZs/XIh0yXJzWcXLvJgnkvp8\n" \
"GJ4PFOGNUd3fz7uXI6bNSaV3NfmxExNObvLPnUBJ1HQjXG/01P7OIzmONKMLDn4Y\n" \
"gXIw8/wAOP5koo45aE7cyWheB3LSQojfRpLnIs7Tb8/3HPysbRA=\n" \
"-----END RSA PRIVATE KEY-----\n";

WiFiClientSecure wiFiClient;
void msgReceived(char* topic, byte* payload, unsigned int len);
PubSubClient pubSubClient(awsEndpoint, 8883, msgReceived, wiFiClient); 
void pubSubCheckConnect();

void setup() {
  Serial.begin(115200);
  servo.attach(SERVO_PIN);
  servo.write(90);
  
  // pinMode(SUCCESS_LED_PIN, OUTPUT);

  Serial.print("Connecting to "); 
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  WiFi.waitForConnectResult();
  Serial.print(", WiFi connected, IP address: "); 
  Serial.println(WiFi.localIP());

  wiFiClient.setCACert(rootCA);
  wiFiClient.setCertificate(certificate_pem_crt);
  wiFiClient.setPrivateKey(private_pem_key);
}

void loop() {
  pubSubCheckConnect();

	bool onInput = true;
	char keyInput[30];
  int index = 0;

  while (onInput) {
    char key = keypad.getKey();
    if (key == '*' && !isDoorLocked) {
      isDoorLocked = true;
      onInput = false;
      for (int pos = 0; pos <= 90; pos++) {
        servo.write(pos);
        delay(30);
      }
                    
      tone(BUZZER_PIN, 2960, 300);
      delay(100);
      tone(BUZZER_PIN, 2960, 300);
      delay(100);
      tone(BUZZER_PIN, 2960, 300);

      Serial.println("\nDoor Locked--Success\n");
      String msg = String("Door Locked Success");
      boolean rc = pubSubClient.publish(topic, msg.c_str());
      Serial.print("Published, rc="); 
      Serial.print( (rc ? "OK: " : "FAILED: ") );

      delay(100);
      break;
    }

    if (isDoorLocked && key) {
			keyInput[index++] = key;
      if (key == '#') {
        keyInput[index - 1] = '\0';
        if (strcmp(lockKey, keyInput) == 0) {
          for (int pos = 90; pos >= 0; pos--) {
            servo.write(pos);
            delay(30);
          }

          isDoorLocked = false;
          Serial.println(strcat(keyInput, " is valid"));
          Serial.println("\nDoor Unlocked--Success\n");  
                    
          tone(BUZZER_PIN, 2960, 500);
          delay(100);
          tone(BUZZER_PIN, 2960, 500);

          noTone(BUZZER_PIN);
          // digitalWrite(SUCCESS_LED_PIN, LOW);

          String msg = String("Door Unlocked Success");
          boolean rc = pubSubClient.publish(topic, msg.c_str());
          Serial.print("Published, rc="); 
          Serial.print( (rc ? "OK: " : "FAILED: ") );
        } else {
          tone(BUZZER_PIN, 185, 800);
          delay(100);
          tone(BUZZER_PIN, 185, 800);
          delay(100);
          tone(BUZZER_PIN, 185, 800);

          noTone(BUZZER_PIN);
          Serial.println("\nDoor Unlocked--Failed\n");
          String msg = String("Door Unlocked Failed");
          boolean rc = pubSubClient.publish(topic, msg.c_str());
          Serial.print("Published, rc="); 
          Serial.print( (rc ? "OK: " : "FAILED: ") );
        }
        
        // Null terminate first element/Erase array content
        keyInput[0] = '\0';
        index = 0;
        onInput = false;
        
        delay(100);
      } else {
        Serial.print("Pressed: ");
        Serial.println(key);
      }
    }
  }
}

void msgReceived(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received on "); 
  Serial.print(topic); 
  Serial.print(": ");
  for (int i = 0; i < length; i++) {
    Serial.print((char) payload[i]);
  }
  Serial.println();
}

void pubSubCheckConnect() {
  if (!pubSubClient.connected()) {
    Serial.print("PubSubClient connecting to: "); 
    Serial.print(awsEndpoint);
    while (!pubSubClient.connected()) {
      Serial.print(".");
      pubSubClient.connect("Moisture_Sensor");
      delay(1000);
    }
    Serial.println(" connected");
    pubSubClient.subscribe(subscribeTopic);
  }
  pubSubClient.loop();
}
