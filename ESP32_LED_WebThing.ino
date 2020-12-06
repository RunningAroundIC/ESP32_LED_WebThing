#define LARGE_JSON_DOCUMENT_SIZE 8192
#define SMALL_JSON_DOCUMENT_SIZE 2048

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Thing.h>
#include <WebThingAdapter.h>

// Wifi
const char *ssid = "";
const char *password = "";
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
ThingDevice device("StormTrooperLamp", "Controllable RGB lamp.", deviceTypes);
// Property
ThingProperty deviceOn("On/Off", "Whether the led is turned on or off", BOOLEAN, "OnOffProperty");
ThingProperty deviceBrightness("Brightness", "The brightness of the light from 0-100", INTEGER, "BrightnessProperty");
ThingProperty deviceColor("Color", "The color of light in RGB", STRING, "ColorProperty");

// ### Functions to be used ###

// A startup "effect" each led will glow green for N miliseconds, and then turn off.
void ledStartup(uint8_t wait) {
  Serial.println("Running startup LED sequece...");
  for(int i=0; i < strip.numPixels(); i++){
    strip.setPixelColor(i, 0, 255, 0);
    strip.show();
    delay(wait);
    strip.setPixelColor(i, 0, 0, 0);
    strip.show();
  }
}

// Simple led blink function to be used in a loop
void ledBlink(uint8_t wait, uint16_t ledNumber, uint32_t color) {
  strip.setPixelColor(ledNumber, color);
  strip.show();
  delay(wait);
  strip.setPixelColor(ledNumber, 0, 0, 0);
  strip.show();
}

// Set/update brightness
void updateBrightness(int brightnessPercent){
  int brightness = map(brightnessPercent, 0, 100, 0, 255);
  if (brightness < 0 || brightness > 255)
  {
    Serial.println("Error, value out of range!");
    return;
  }
  if (strip.getBrightness() != brightness)
  {
    strip.setBrightness(brightness);
    strip.show();
  }
}

// Test Wheel and rainbow can be deleted

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
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start <= 60000){
    Serial.print(".");
    ledBlink(70, (strip.numPixels() / 2), ledBlinkColor);
    delay(330);
  }
  Serial.println("");

  if(WiFi.status() == WL_CONNECTED) {
    connected = true;
    const uint32_t ledFillColor = strip.Color(22, 181, 147);
    for(int i=0; i < 3; i++) {
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

    // init webthings adaptor
    adapter = new WebThingAdapter("rgbLamp", WiFi.localIP());

    // Setting values on the properties
    deviceBrightness.minimum = 0;
    deviceBrightness.maximum = 100;
    deviceBrightness.unit = "percent";

    // Adding properties
    device.addProperty(&deviceOn);
    device.addProperty(&deviceBrightness);
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
    Serial.println("Failed connecting to Wifi!");
    Serial.println("Please check your network, and retry.");
  }

  if (!connected)
  {
    const uint32_t ledErrorColor = strip.Color(255, 0, 0);
    while (true)
    {
      for(int i=0; i < strip.numPixels(); i++){
        ledBlink(140, i, ledErrorColor);
      }
      delay(500);
    }    
  }

  delay(500);

  
  
  strip.show();
}

bool lastOnOff = true;
int lastPrecent = 0;

// Main code loop
void loop(){
  adapter->update();
  
  // On/Off
  if (deviceOn.getValue().boolean != lastOnOff) {
    Serial.println("On/Off");
    const uint32_t testColor = strip.Color(0, 255, 0);
    const uint32_t testColor2 = strip.Color(0, 0, 0);
    deviceOn.getValue().boolean ? strip.fill(testColor) : strip.fill(testColor2);
    strip.show();
    lastOnOff = deviceOn.getValue().boolean;
  }
  if (deviceBrightness.getValue().integer != lastPrecent)
  {
    // Kan ikke bruge brightness til at dimme
    lastPrecent = deviceBrightness.getValue().integer;
    updateBrightness(deviceBrightness.getValue().integer);
  }
}
