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
ThingProperty deviceLevel("level", "The brightness of the light from 0-255", NUMBER, "BrightnessProperty");
ThingProperty deviceColor("color", "The color of light in RGB", STRING, "ColorProperty");
ThingProperty remberColor("no", "Whether to rember the given color on next startup", BOOLEAN, "BooleanProperty");

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

void update(String *color) {
  int red, green, blue;
  if (color && (color->length() == 7) && color->charAt(0) == '#') {
    const char *hex = 1 + (color->c_str()); // skip leading '#'
    sscanf(0 + hex, "%2x", &red);
    sscanf(2 + hex, "%2x", &green);
    sscanf(4 + hex, "%2x", &blue);
  }

  if (red && green && blue) {
    uint32_t rgbcolor = strip.gamma32(strip.Color(red, green, blue));
    strip.fill(rgbcolor);
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
    for(int i=0; i < 4; i++) {
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
    deviceLevel.minimum = 0;
    deviceLevel.maximum = 255;
    deviceLevel.unit = "brightness";

    // Adding properties
    device.addProperty(&deviceOn);
    device.addProperty(&deviceLevel);
    device.addProperty(&deviceColor);
    device.addProperty(&remberColor);

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

// Main code loop
void loop(){
  adapter->update();

  // On/Off
  if (deviceOn.getValue().boolean) {
    strip.setBrightness(deviceLevel.getValue().integer);
    strip.show();
  }else {
    strip.setBrightness(0);
    strip.show();
  }
  
  // Set color
  update(deviceColor.getValue().string);

}
