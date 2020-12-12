#define LARGE_JSON_BUFFERS 1

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
const int brightness = 51;
String defaultColor = "#00ff00";

// Declare NeoPixel strip object
Adafruit_NeoPixel strip(ledCount, ledPin, NEO_GRB + NEO_KHZ800);

// Declare webthing adaptor
WebThingAdapter *adapter;

// Declaring the webthing device, and its properties
const char *deviceTypes[] = {"Light", "OnOffSwitch", "ColorControl", nullptr};
// Device
ThingDevice device("StormTrooperLamp", "Stormtrooper lamp", deviceTypes);
// Property
ThingProperty deviceOn("On/Off", "Whether the led is turned on or off", BOOLEAN, "OnOffProperty");
ThingProperty deviceBrightness("Brightness", "The brightness of the light from 0-100", INTEGER, "BrightnessProperty");
ThingProperty deviceColor("Color", "The color of light", STRING, "ColorProperty");

// ### Functions to be used ###

struct RGB{
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct RGB
// Convert HEX to RGB
colorConverter(String color){
  int hexValue = (int) strtol( &color[1], NULL, 16);
  struct RGB rgbColor;
  rgbColor.r = hexValue >> 16;
  rgbColor.g = hexValue >> 8 & 0xFF;
  rgbColor.b = hexValue & 0xFF;
  return (rgbColor);
}

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

// Get latest set color as RGB
const uint32_t getColor(String color){
  if (color.isEmpty())
  {
    Serial.println("Error, getting color failed, no color given!");
    const uint32_t errorColor = strip.gamma32(strip.Color(255, 0, 0));
    return errorColor;
  }
  RGB rgb = colorConverter(color);

  const uint32_t convertedColor = strip.gamma32(strip.Color(rgb.r, rgb.g, rgb.b));

  return convertedColor;
}

// Update color
void updateColor(String color){
  if (color.isEmpty())
  {
    Serial.println("Error, no color given!");
    return;
  }
  RGB rgb = colorConverter(color);

  const uint32_t ledColor = strip.gamma32(strip.Color(rgb.r, rgb.g, rgb.b));
  strip.fill(ledColor);
  strip.show();
}

// Update brightness
void updateBrightness(int brightnessPercent, String color){
  int brightness = map(brightnessPercent, 0, 100, 0, 255);
  
  if ((brightness < 0 || brightness > 255) && color.isEmpty())
  {
    Serial.println("Error, value out of range, or no color given!");
    return;
  }  

  if (strip.getBrightness() != brightness)
  {
    RGB rgb = colorConverter(color);
    const uint32_t ledcolor = strip.gamma32(strip.Color(rgb.r, rgb.g, rgb.b));

    strip.clear();
    strip.show();
    if(!strip.canShow()){
      delayMicroseconds(300);
    }
    strip.setBrightness(brightness);
    strip.show();
    if(!strip.canShow()){
      delayMicroseconds(300);
    }
    strip.fill(ledcolor);
    strip.show();
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
    adapter = new WebThingAdapter("stormTrooperLamp", WiFi.localIP());

    // Setting values on the properties
    ThingPropertyValue on;
    on.boolean = true;
    deviceOn.setValue(on);

    int startPercent = ((float)brightness/255)*100;
    deviceBrightness.minimum = 0;
    deviceBrightness.maximum = 100;
    deviceBrightness.unit = "percent";
    ThingPropertyValue brightnessValue;
    brightnessValue.integer = startPercent;
    deviceBrightness.setValue(brightnessValue);

    ThingPropertyValue colorValue;
    colorValue.string = &defaultColor;
    deviceColor.setValue(colorValue);

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

bool lastOnOff = false;
String lastColor = "#00ff00";
int lastPrecent = ((float)brightness/255)*100;

// Main code loop
void loop(){
  adapter->update();
  
  if (deviceOn.getValue().boolean != lastOnOff) {
    const uint32_t off = strip.Color(0, 0, 0);
    deviceOn.getValue().boolean ? strip.fill(getColor(lastColor)) : strip.fill(off);
    strip.show();
    lastOnOff = deviceOn.getValue().boolean;
  }
  if (!deviceColor.getValue().string->equals(lastColor)){
    updateColor(deviceColor.getValue().string->c_str());
    lastColor = deviceColor.getValue().string->c_str();
  }
  if (deviceBrightness.getValue().integer != lastPrecent)
  {
    lastPrecent = deviceBrightness.getValue().integer;
    updateBrightness(deviceBrightness.getValue().integer, deviceColor.getValue().string->c_str());
  }
}
