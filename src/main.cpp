#include <Arduino.h>
#include <WEMOS_DHT12.h>
#include <ESP8266WiFi.h>
#include <settings.h>
#include <Wire.h>

class Heater {
public:
    Heater(DHT12& dht12);
    void heaterMode(int mode);
    float getTemp();
    float getHumi();
    void refresh();
    void setDesiredTemp(float temp);
    void heaterOn(bool on);
    bool heaterState();
    bool isInAutoMode();

private:
    DHT12& dht12;
    float _temp;
    float _humi;
    bool _isHeaterOn;
    bool _autoMode;
};

const float desiredTemp = 26;
const int relayPin = D1;
const int ledPin = BUILTIN_LED;

Heater::Heater(DHT12& dht) :dht12(dht) {
    pinMode(relayPin, OUTPUT);
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, HIGH);
}

float Heater::getTemp() {
    return _temp;
}

float Heater::getHumi() {
    return _humi;
}

void Heater::heaterMode(int mode) {
  if (mode == 1) {
    _autoMode = false;
    heaterOn(true);
    return;
  }
  else if (mode == 0) {
    _autoMode = false;
    heaterOn(false);
    return;
  } 
  else { 
    _autoMode = true;  
    if (dht12.cTemp < desiredTemp) {
        heaterOn(true);
        } 
    else if (dht12.cTemp >= desiredTemp) {
            heaterOn(false);
        }
    }
}

bool Heater::isInAutoMode() {
    return _autoMode;
}

void Heater::heaterOn(bool on) {
  if (on == true) {
    digitalWrite(ledPin, LOW);
    digitalWrite(relayPin, HIGH);
    _isHeaterOn = true;
  }
  else {
    digitalWrite(ledPin, HIGH);
    digitalWrite(relayPin, LOW);
    _isHeaterOn = false;
  }
}

bool Heater::heaterState() {
  return _isHeaterOn;
}

void Heater::refresh() {
  if (dht12.get() == 0) {
    _temp = dht12.cTemp;
    _humi = dht12.humidity;
  }  
}

//change wire pins because relay will be on D1
const int sclPin = D5;
const int sdaPin = D2;

const int pollInterval = 5000;

const char* ssid = _ssid;
const char* password = _password;

unsigned long counter = 1;
unsigned long prevMillis = 0;
unsigned long currentMillis = 0;

WiFiServer server(80);
DHT12 dht12;
Heater heater(dht12);

void setup() {

  Wire.begin(sdaPin, sclPin);
  Serial.begin(115200);
  delay(10);
 
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
 
  // Start the server
  server.begin();
  Serial.println("Server started");
 
  // Print the IP address
  Serial.print("Use this URL : ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
}
 
void loop() {
  //poll the sensor every 5000 seconds
  currentMillis = millis();
  if (currentMillis - prevMillis > pollInterval) {
    prevMillis = currentMillis;
    heater.refresh();    
    if (heater.isInAutoMode() == true) {
      heater.heaterMode(2);
    }
    counter++;
  }  
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
 
  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    Serial.println("client wait");
    delay(1);
  }
 
  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();
 
  // Match the request
  if (request.indexOf("/Heater=ON") != -1) {
    heater.heaterMode(1);
  } 
  else if (request.indexOf("/Heater=AUTO") != -1) {
    heater.heaterMode(2);
  }
  else if (request.indexOf("/Heater=OFF") != -1) {
    heater.heaterMode(0);
  }
 
  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
 
  client.print("Heater is now ");
  client.print(heater.heaterState());
  if(heater.heaterState() == false) {
    client.print(" Off<br>");
  } else {
    client.print(" On<br>");
  }
    
  client.println("<br><br>");
  client.println("Click <a href=\"/Heater=ON\">here to turn heater ON</a><br>");
  client.println("Click <a href=\"/Heater=OFF\">here to turn heater OFF</a><br>");
  client.println("Click <a href=\"/Heater=AUTO\">here to set to AUTO mode</a><br>");
  
  client.println("============<br> Temperature Control <br>============");
  client.println("<br><br>");

  client.println("Temperature in Celsius : ");
    client.println(heater.getTemp());
    client.println("<br>");
    client.print("Relative Humidity : ");
    client.println(heater.getHumi());
    client.println("<br>END");
    client.println(counter);

  
  client.println("</html>");
 
  delay(1);
  Serial.println("Client disconnected");
  Serial.println("");
}