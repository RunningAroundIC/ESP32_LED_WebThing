#define LARGE_JSON_BUFFERS 1

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Thing.h>
#include <WebThingAdapter.h>

// Wifi
const char *ssid = "*****";
const char *password = "******";
bool connected = false;

// Led information
const int ledPin = 13;
const int ledCount = 12;
const int brightness = 50;

// Declare NeoPixel strip object
Adafruit_NeoPixel strip(ledCount, ledPin, NEO_GRB + NEO_KHZ800);

// Declare webthing adaptor
WebThingAdapter *adapter;

// Declaring the webthing device, and its properties
const char *deviceTypes[] = {"Light", "OnOffSwitch", "ColorControl", nullptr};
// Device
ThingDevice device("StormTrooperLamp", "Controllable rgb lamp, there are opetions to dim and change color on this lamp.", deviceTypes);
// Property
ThingProperty deviceOn("on", "Whether the led is turned on or off", BOOLEAN, "OnOffProperty");
ThingProperty deviceLevel("level", "The brightness of the light from 0-100", NUMBER, "BrightnessProperty");
ThingProperty deviceColor("color", "The color of light in RGB", STRING, "ColorProperty");

// Setup
void setup() {
  Serial.begin(115200);
  // Turn off bluetooth
  Serial.println("Turning off bluetooth...");
  btStop();

  // Init LED and run startup sequence
  Serial.println("Activating LED...");
  strip.begin();
  strip.setBrightness(brightness);
  ledStartup(70);
  strip.show();

  // Connect to Wifi
  Serial.println("Connecting to Wifi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  const uint32_t ledBlinkColor = strip.Color(255, 0, 0);
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    ledBlink(70, 6, ledBlinkColor);
    delay(350);
  }
  Serial.println("");

  if(WiFi.status() == WL_CONNECTED){
    connected = true;
    const uint32_t ledFillColor = strip.Color(22, 181, 147);
    for(int i=0; i < 4; i++){
      strip.fill(ledFillColor);
      strip.show();
      delay(90);
      strip.clear();
      strip.show();
      delay(200);
    }

    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    adapter = new WebThingAdapter("rgbLamp", WiFi.localIP());

    // Setting values on the properties
    deviceLevel.minimum = 0;
    deviceLevel.maximum = 100;
    deviceLevel.unit = "percent";
    // Adding properties
    device.addProperty(&deviceOn);
    device.addProperty(&deviceLevel);
    device.addProperty(&deviceColor);
    // Adding device
    adapter->addDevice(&device);
    // Start the adaptor
    adapter->begin();
    Serial.println("HTTP server started");
    Serial.print("http://");
    Serial.print(WiFi.localIP());
    Serial.print("/things/");
    Serial.println(device.id);
  }else{
    Serial.println("Failed to connect to Wifi!");
  }

  delay(500);

  strip.show();
}

// A startup "effect" each led will glow green for N miliseconds, and then turn off.
void ledStartup(uint8_t wait){
  Serial.println("Running startup LED sequece...");
  for(int i=0; i < strip.numPixels(); i++){
    strip.setPixelColor(i, 0, 255, 0);
    strip.show();
    delay(wait);
    strip.setPixelColor(i, 0, 0, 0);
    strip.show();
  }
}

// Simple led blink function to be used in loop
void ledBlink(uint8_t wait, uint16_t ledNumber, uint32_t color){
  strip.setPixelColor(ledNumber, color);
  strip.show();
  delay(wait);
  strip.setPixelColor(ledNumber, 0, 0, 0);
  strip.show();
}

// Main code loop
void loop(){
  if(connected){
    rainbow(20);
  }  
}

// Test
void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
