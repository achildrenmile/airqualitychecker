/*********
 * Michael Linder
 * 14.12.2020
 * 
 * Ideas and some code parts from
 * ** Rui Santos
 * ** Complete project details at https://RandomNerdTutorials.com/esp32-bme680-sensor-arduino/
 * **Permission is hereby granted, free of charge, to any person obtaining a copy
 * **of this software and associated documentation files.
 * **The above copyright notice and this permission notice shall be included in all
 * ** copies or substantial portions of the Software.
 * **Gas Scoring Model from https://github.com/G6EJD/BME680-Example/blob/master/ESP32_bme680_CC_demo_03.ino

*********/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include <WiFi.h>
#include "ESPAsyncWebServer.h"


const char* versionnumber='0.9.2'
//added versionnumber; added sealevel calculation at setup


#define PIXELPIN 16

#include "NeoPixelBus.h"
#include "NeoPixelAnimator.h"
#define PIXELCOUNT 7//Define the number of leds
const uint16_t PixelCount = PIXELCOUNT; // make sure to set this to the number of pixels in your strip
const uint8_t PixelPin = PIXELPIN;   // make sure to set this to the correct pin, ignored for Esp8266

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);
// For Esp8266, the Pin is omitted and it uses GPIO3 due to DMA hardware use.  
// There are other Esp8266 alternative methods that provide more pin options, but also have
// other side effects.
//NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount);
//
// NeoEsp8266Uart800KbpsMethod uses GPI02 instead

void SetPixelColorAndShow(RgbColor colortarget)
{
     for (uint16_t pixel = 0; pixel < PixelCount; pixel++) {
         strip.SetPixelColor(pixel, colortarget); 
     }
     strip.Show();
}

// Replace with your network credentials
const char* ssid = "<yourssid>";
const char* password = "<yourwifipassword>";

//Uncomment if using SPI
/*#define BME_SCK 18
#define BME_MISO 19
#define BME_MOSI 23
#define BME_CS 15*/

Adafruit_BME680 bme; // I2C
//Adafruit_BME680 bme(BME_CS); // hardware SPI
//Adafruit_BME680 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK);

float temperature;
float humidity;
float pressure;
float altitude;
float gasResistance;
String aiq;

AsyncWebServer server(80);
AsyncEventSource events("/events");

unsigned long lastTime = 0;  
unsigned long timerDelay = 30000;  // send readings timer

const float sealevel=1024; //TODO ON OTHER LOCATIONS: NHN nearest official station

float hum_weighting = 0.25; // so hum effect is 25% of the total air quality score
float gas_weighting = 0.75; // so gas effect is 75% of the total air quality score

int   humidity_score, gas_score;
float gas_reference = 2500;
float hum_reference = 40;
int   getgasreference_count = 0;
int   gas_lower_limit = 10000;  // Bad air quality limit
int   gas_upper_limit = 300000; // Good air quality limit

void GetGasReference() {
  // Now run the sensor for a burn-in period, then use combination of relative humidity and gas resistance to estimate indoor air quality as a percentage.
  //Serial.println("Getting a new gas reference value");
  int readings = 10;
  for (int i = 1; i <= readings; i++) { // read gas for 10 x 0.150mS = 1.5secs
    gas_reference += bme.readGas();
  }
  gas_reference = gas_reference / readings;
  //Serial.println("Gas Reference = "+String(gas_reference,3));
}

String CalculateIAQ(int score) {
  String IAQ_text = "";
  score = (100 - score) * 5;
  if      (score >= 301)                  IAQ_text += "Hazardous";
  else if (score >= 201 && score <= 300 ) IAQ_text += "Very Unhealthy";
  else if (score >= 176 && score <= 200 ) IAQ_text += "Unhealthy";
  else if (score >= 151 && score <= 175 ) IAQ_text += "Unhealthy for Sensitive Groups";
  else if (score >=  51 && score <= 150 ) IAQ_text += "Moderate";
  else if (score >=  00 && score <=  50 ) IAQ_text += "Good";
  Serial.print("IAQ Score = " + String(score) + ", ");
  return IAQ_text;
}

int GetHumidityScore(float current_humidity) {  //Calculate humidity contribution to IAQ index
  if (current_humidity >= 38 && current_humidity <= 42) // Humidity +/-5% around optimum
    humidity_score = 0.25 * 100;
  else
  { // Humidity is sub-optimal
    if (current_humidity < 38)
      humidity_score = 0.25 / hum_reference * current_humidity * 100;
    else
    {
      humidity_score = ((-0.25 / (100 - hum_reference) * current_humidity) + 0.416666) * 100;
    }
  }
  return humidity_score;
}

int GetGasScore() {
  //Calculate gas contribution to IAQ index
  gas_score = (0.75 / (gas_upper_limit - gas_lower_limit) * gas_reference - (gas_lower_limit * (0.75 / (gas_upper_limit - gas_lower_limit)))) * 100.00;
  if (gas_score > 75) gas_score = 75; // Sometimes gas readings can go outside of expected scale maximum
  if (gas_score <  0) gas_score = 0;  // Sometimes gas readings can go outside of expected scale minimum
  return gas_score;
}

int GetGasScore(float gasResistance) {
  //Calculate gas contribution to IAQ index
  gas_score = (0.75 / (gas_upper_limit - gas_lower_limit) * gasResistance - (gas_lower_limit * (0.75 / (gas_upper_limit - gas_lower_limit)))) * 100.00;
  if (gas_score > 75) gas_score = 75; // Sometimes gas readings can go outside of expected scale maximum
  if (gas_score <  0) gas_score = 0;  // Sometimes gas readings can go outside of expected scale minimum
  return gas_score;
}

void getBME680Readings(){
  // Tell BME680 to begin measurement.
  unsigned long endTime = bme.beginReading();
  if (endTime == 0) {
    Serial.println(F("Failed to begin reading :("));
    return;
  }
  if (!bme.endReading()) {
    Serial.println(F("Failed to complete reading :("));
    return;
  }
  temperature = bme.temperature;
  pressure = bme.pressure / 100.0;
  humidity = bme.humidity;
  gasResistance = bme.gas_resistance / 1000.0;

  Serial.println(bme.gas_resistance);

  humidity_score = GetHumidityScore(humidity);
  gas_score      = GetGasScore(gasResistance*1000.0);

  //Combine results for the final IAQ index value (0-100% where 100% is good quality air)
  float air_quality_score = humidity_score + gas_score;
  Serial.println(" comprised of " + String(humidity_score) + "% Humidity and " + String(gas_score) + "% Gas");
  //

  //if ((getgasreference_count++) % 5 == 0) GetGasReference();
  aiq=CalculateIAQ(air_quality_score);
  
  Serial.println(aiq);
  Serial.println("--------------------------------------------------------------");
}

String processor(const String& var){
  getBME680Readings();
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return String(temperature);
  }
  else if(var == "HUMIDITY"){
    return String(humidity);
  }
  else if(var == "PRESSURE"){
    return String(pressure);
  }
  else if(var == "ALTITUDE"){
    return String(altitude);
  }
 else if(var == "GAS"){
    return String(gasResistance);
  }
 else if(var == "AIQ"){
    return String(aiq);
 }
 else if(var == "AIQCLASS"){
    String aiqclass="unset";
    
    if(aiq=="Hazardous") {
      aiqclass="hazardous";
      SetPixelColorAndShow(RgbColor(255,0,0));
    }
    else if(aiq== "Very Unhealthy") {
      aiqclass="veryunhealthy";
      SetPixelColorAndShow(RgbColor(200,0,0));
    }
    else if(aiq=="Unhealthy") {
      aiqclass="unhealthy";
      SetPixelColorAndShow(RgbColor(255,99,71));
    }
    else if(aiq=="Unhealthy for Sensitive Groups") {
      aiqclass="unhealthyforsensitivegroups";
      SetPixelColorAndShow(RgbColor(255,69,0));
    }
    else if(aiq=="Moderate") {
      aiqclass="moderate";
      SetPixelColorAndShow(RgbColor(255,165,0));
    }
    else if(aiq=="good") {
      aiqclass="good";
      SetPixelColorAndShow(RgbColor(124,252,0));
    }

    Serial.println("requested");
    Serial.println(String(aiqclass));

    return aiqclass;
 }
}

void checkAIQandLight() 
{
   if(aiq=="Hazardous") {
      SetPixelColorAndShow(RgbColor(255,0,0));
    }
    else if(aiq== "Very Unhealthy") {
      SetPixelColorAndShow(RgbColor(200,0,0));
    }
    else if(aiq=="Unhealthy") {
      SetPixelColorAndShow(RgbColor(255,99,71));
    }
    else if(aiq=="Unhealthy for Sensitive Groups") {
      SetPixelColorAndShow(RgbColor(255,69,0));
    }
    else if(aiq=="Moderate") {
      SetPixelColorAndShow(RgbColor(255,165,0));
    }
    else if(aiq=="good") {
      SetPixelColorAndShow(RgbColor(124,252,0));
    }
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Environment Check</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: monospace; display: inline-block; text-align: center;}
    p {  font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #7d7d7d; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
    .reading { font-size: 2.8rem; }
    .card.temperature { color: #0e7c7b; }
    .card.humidity { color: #17bebb; }
    .card.pressure { color: #3fca6b; }
    .card.altitude { color: #edd93e; }
    .card.gas { color: #d62246; }
    .card.aiq { color: ##669999; }
    .hazardous { color: #cc0000 !important;}
    .veryunhealthy { color: #b30000 !important;}
    .unhealthy { color: #ff0000 !important; }
    .unhealthyforsensitivegroups { color: #ff3300 !important;} 
    .moderate { color: #ff9900 !important;}
    .good {color: #00ff00 !important;}
  </style>
</head>
<body>
  <div class="topnav">
    <h3>Environment Check</h3>
  </div>
  <div class="content">
    <div class="cards" style="margin-bottom: 30px;">
      <div class="card aiq">
        <h4><i class="fas fa-traffic-light"></i> Air Quality Score</h4><p><span class="reading"><span id="aiq" class="%AIQCLASS%">%AIQ%</span></span></p>
      </div>
    </div>
    <div class="cards">
      <div class="card temperature">
        <h4><i class="fas fa-thermometer-half"></i> TEMPERATURE</h4><p><span class="reading"><span id="temp">%TEMPERATURE%</span> &deg;C</span></p>
      </div>
      <div class="card humidity">
        <h4><i class="fas fa-tint"></i> HUMIDITY</h4><p><span class="reading"><span id="hum">%HUMIDITY%</span> &percnt;</span></p>
      </div>
      <div class="card pressure">
        <h4><i class="fas fa-angle-double-down"></i> PRESSURE</h4><p><span class="reading"><span id="pres">%PRESSURE%</span> hPa</span></p>
      </div>
       <div class="card altitude">
        <h4><i class="fas fa-angle-double-down"></i> SEA LEVEL</h4><p><span class="reading"><span id="pres">%ALTITUDE%</span> m</span></p>
      </div>
      <div class="card gas">
        <h4><i class="fas fa-wind"></i> GAS</h4><p><span class="reading"><span id="gas">%GAS%</span> K&ohm;</span></p>
      </div>
    </div>
  </div>
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);
 
 source.addEventListener('temperature', function(e) {
  console.log("temperature", e.data);
  document.getElementById("temp").innerHTML = e.data;
 }, false);
 
 source.addEventListener('humidity', function(e) {
  console.log("humidity", e.data);
  document.getElementById("hum").innerHTML = e.data;
 }, false);
 
 source.addEventListener('pressure', function(e) {
  console.log("pressure", e.data);
  document.getElementById("pres").innerHTML = e.data;
 }, false);

 source.addEventListener('altitude', function(e) {
  console.log("altitude", e.data);
  document.getElementById("altitude").innerHTML = e.data;
 }, false);
 
 source.addEventListener('gas', function(e) {
  console.log("gas", e.data);
  document.getElementById("gas").innerHTML = e.data;
 }, false);

  source.addEventListener('aiq', function(e) {
  console.log("aiq", e.data);
  var element=document.getElementById("aiq");
  element.className="";

  if(e.data=="Hazardous")
    element.className="hazardous";
  else if(e.data== "Very Unhealthy")
    element.className="veryunhealthy";
  else if(e.data== "Unhealthy")
    element.className="unhealthy";
  else if(e.data== "Unhealthy for Sensitive Groups")
    element.className="unhealthyforsensitivegroups";
  else if(e.data== "Moderate")
    element.className="moderate";
  else if(e.data== "Good")
    element.className="good";
  
  element.innerHTML = e.data;
 }, false);
}
</script>
</body>
</html>)rawliteral";

void setup() {
  Serial.begin(115200);

  // Set the device as a Station and Soft Access Point simultaneously
  WiFi.mode(WIFI_AP_STA);
  
  // Set device as a Wi-Fi Station
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
  }
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Init BME680 sensor
  if (!bme.begin()) {
    Serial.println(F("Could not find a valid BME680 sensor, check wiring!"));
    while (1);
  }
  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms

  altitude=bme.readAltitude(sealevel);
  
  GetGasReference();

  // Handle Web Server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Handle Web Server Events
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  server.begin();

  //------------------LED Shield Initialization------------------------------------------------------------------
  #ifdef PIXELPIN
  //------LED Initialization----
  Serial.println("[INFO: Starting LED");
  strip.Begin();
  #endif
  
  getBME680Readings();
  SetPixelColorAndShow(RgbColor(0,0,0));

}

void loop() {

  //SetPixelColorAndShow(RgbColor(250,0,0));
  if ((millis() - lastTime) > timerDelay) {
    GetGasReference();
    getBME680Readings();
    checkAIQandLight();
    Serial.printf("Temperature = %.2f ºC \n", temperature);
    Serial.printf("Humidity = %.2f % \n", humidity);
    Serial.printf("Pressure = %.2f hPa \n", pressure);
    Serial.printf("Sea level = %.2f m \n", altitude);
    Serial.printf("Gas Resistance = %.2f KOhm \n", gasResistance);
    //Serial.printf("AIQ = %s \n", aiq);
    Serial.println();

    // Send Events to the Web Server with the Sensor Readings
    events.send("ping",NULL,millis());
    events.send(String(temperature).c_str(),"temperature",millis());
    events.send(String(humidity).c_str(),"humidity",millis());
    events.send(String(pressure).c_str(),"pressure",millis());
    events.send(String(altitude).c_str(),"altitude",millis());
    events.send(String(gasResistance).c_str(),"gas",millis());
    events.send(String(aiq).c_str(),"aiq",millis());
    
    lastTime = millis();
  }
}
