#include "env_config.h"
#include "ESPAsyncWebServer.h"
#include "FastLED.h"
#include "WiFi.h"

// Init
CRGB leds[ENV_LEDS_NUMBER]; // Create LED array
char serprt_buffer[50];     // Create serial print buffer
AsyncWebServer server(80);         // Create AsyncWebServer on port 80
const char *COLOR_INPUT = "color"; // GET request param query keyword

// Function pointers
void (*enableColorEffect)(uint8_t, uint8_t, uint8_t);
void (*disableColorEffect)(uint8_t, uint8_t, uint8_t);

// Color effects
void colorEnableFromCenter(uint8_t red, uint8_t green, uint8_t blue)
{
  for (int i = ENV_LEDS_NUMBER / 2; i >= 0; i -= ENV_LED_GROUP_SIZE)
  {
    for (int j = i; j > i - ENV_LED_GROUP_SIZE && j >= 0; j--)
    {
      leds[j].setRGB(red, green, blue);
      if ((ENV_LEDS_NUMBER - j) < ENV_LEDS_NUMBER)
      {
        leds[ENV_LEDS_NUMBER - j].setRGB(red, green, blue);
      }
    }
    FastLED.show();
    FastLED.delay(ENV_DELAY_TIME);
  }
}

void colorDisableToCenter(uint8_t red, uint8_t green, uint8_t blue)
{
  for (int i = 0; i <= ENV_LEDS_NUMBER / 2; i += ENV_LED_GROUP_SIZE)
  {
    for (int j = i; j < i + ENV_LED_GROUP_SIZE && j <= ENV_LEDS_NUMBER / 2; j++)
    {
      leds[j].setRGB(red, green, blue);
      if ((ENV_LEDS_NUMBER - j) < ENV_LEDS_NUMBER)
      {
        leds[ENV_LEDS_NUMBER - j].setRGB(red, green, blue);
      }
    }
    FastLED.show();
    FastLED.delay(ENV_DELAY_TIME);
  }
}

void colorEnableFromEdge(uint8_t red, uint8_t green, uint8_t blue)
{
  for (int i = ENV_LEDS_NUMBER - 1; i >= 0; i -= ENV_LED_GROUP_SIZE)
  {
    for (int j = i; j > i - ENV_LED_GROUP_SIZE && j >= 0; j--)
    {
      leds[j].setRGB(red, green, blue);
    }
    FastLED.show();
    FastLED.delay(ENV_DELAY_TIME);
  }
}

void colorDisableToEdge(uint8_t red, uint8_t green, uint8_t blue)
{
  for (int i = 0; i < ENV_LEDS_NUMBER; i += ENV_LED_GROUP_SIZE)
  {
    for (int j = i; j < i + ENV_LED_GROUP_SIZE && j < ENV_LEDS_NUMBER; j++)
    {
      leds[j].setRGB(red, green, blue);
    }
    FastLED.show();
    FastLED.delay(ENV_DELAY_TIME);
  }
}

// Handle Loxone color update
void handleColorUpdate(int color)
{
  if (color == 0)
  {
    disableColorEffect(0, 0, 0);
    return; 
  }

  // Convert the input number to string
  char colorString[10];
  sprintf(colorString, "%09d", color);

  // Print the color string with leading zeros
  sprintf(serprt_buffer, "Loxone message (padded): %s", colorString);
  Serial.println(serprt_buffer);

  // Extract RED (0-100%)
  uint8_t red = atoi(&colorString[6]);

  // Extract GREEN (0-100%)
  colorString[6] = '\0';
  uint8_t green = atoi(&colorString[3]);

  // Extract BLUE (0-100%)
  colorString[3] = '\0';
  uint8_t blue = atoi(colorString);

  // Map percent values (0-100%) to RGB 8-bit values (0-255)
  red = map(red, 0, 100, 0, 255);
  green = map(green, 0, 100, 0, 255);
  blue = map(blue, 0, 100, 0, 255);

  sprintf(serprt_buffer, "Color values to LED STRIP - RED: %d, GREEN: %d, BLUE: %d", red, green, blue);
  Serial.println(serprt_buffer);

  // Call LED preset
  enableColorEffect(red, green, blue);
}

void setup()
{
  // Init LED strip and turn off all LEDs on boot
  FastLED.addLeds<WS2812B, ENV_SIGNAL_PIN, GRB>(leds, ENV_LEDS_NUMBER);
  FastLED.clear();
  FastLED.show();

  // Set function pointers based on ENV_EFFECT_SYMMETRICAL
  if (ENV_EFFECT_SYMMETRICAL == 1)
  {
    enableColorEffect = colorEnableFromCenter;
    disableColorEffect = colorDisableToCenter;
  }
  else
  {
    enableColorEffect = colorEnableFromEdge;
    disableColorEffect = colorDisableToEdge;
  }

  // Start serial monitor
  Serial.begin(115200);

  // Connect to Wi-Fi adn show local IP address
  const char *ssid = ENV_WIFI_SSID;
  const char *password = ENV_WIFI_PASS;
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi ...");
  }
  Serial.println(WiFi.localIP());

  // Parse Loxone color to RGB values and send to LED strip
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String inputMessage = request->getParam(COLOR_INPUT)->value();
    int color = inputMessage.toInt();
    sprintf(serprt_buffer, "Loxone message: %d", color);
    Serial.println(serprt_buffer);
    handleColorUpdate(color);
    request->send(200, "text/plain", "OK"); });

  // Start server
  server.begin();
}

void loop()
{
}
