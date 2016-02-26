/*
  Makeathon code
  
  The circuit:
 * LCD RS pin to digital pin 12 /
 * LCD Enable pin to digital pin 11 /
 * LCD D4 pin to digital pin 5 /
 * LCD D5 pin to digital pin 4 /
 * LCD D6 pin to digital pin 3 /
 * LCD D7 pin to digital pin 2 /
 * LCD R/W pin to ground /
 * LCD VSS pin to ground /
 * LCD VCC pin to 5V / 
 * 10K pot:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 * Neopixel data to digital pin 6
 */

#include <LiquidCrystal.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

#define LED_PIN   (6)
#define NUM_LEDS  (105)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, 
                                              NEO_GRB + NEO_KHZ800);

#define POT_PIN 0
#define BTN_PIN 10

#define WATER_BUDGET  (13)
#define MS_IN_SEC     (1000)
#define SEC_IN_MIN    (60)

int state = 0;
int val = 0;
uint16_t gpm = 1;
uint32_t ledColor = strip.Color(255, 0, 0);
float gallonsUsed = 0.0;
unsigned long showerStart = 0;
unsigned long lastTime = 0;
unsigned int budget = WATER_BUDGET;

void setup() {
  // LCD
  lcd.begin(16, 2);
  printState();
  printUsage();

  // Push Button
  pinMode(BTN_PIN, INPUT_PULLUP);

  // LED strip
  strip.begin();           
  //strip.setBrightness(100);
  fillStrip(ledColor, NUM_LEDS);

  // Serial (bluetooth)
  Serial1.begin(9600);
}

void loop() {  
  // Check for turn on/off
  if (digitalRead(BTN_PIN) == LOW) {
    state = !state;
    printState();
    printUsage();
    // If turning on, reset budget and track start
    while (digitalRead(BTN_PIN) == LOW) {
      delay(10);
    }
    if (state == 1) {
      budget = WATER_BUDGET;
      gallonsUsed = 0;
      lastTime = millis();
      showerStart = millis();
    } else {
      Serial1.print(millis() - showerStart);
      Serial1.print("/");
      Serial1.println(gallonsUsed);
    }
  }

  // Print time
  printTime();

  // Read pot
  val = analogRead(POT_PIN);
  if (gpm != map(val, 0, 1000, 1, 10)) {
    gpm = map(val, 0, 1000, 1, 10);
    printUsage();
  }

  // Adjust water budget
  if (millis() - lastTime > 900 && state == 1) {
    float secPassed = (millis() - lastTime)/1000.0;
    float gpsec = gpm / 60.0;
    gallonsUsed += secPassed*gpsec;
    if (budget > 0)
      budget = WATER_BUDGET - (int)gallonsUsed;
    else if (budget > 14)
      budget = 0;
    lastTime = millis();
    printUsage();
  }

  // Print water budget and adjust LEDs
  setColor();
  int numOn = numLEDsOn();
  fillStrip(ledColor, numOn);

  // Delay
  delay(50);
}

void printState() {
  // Shower on/off
  lcd.setCursor(0,0);
  lcd.print("Shower ");
  if (state == 0) {
    lcd.print("OFF      ");
  }
  else {
    lcd.print("ON ");
  }
}

void printTime() {
  lcd.setCursor(10,0);
  if (state == 1) {
    unsigned long seconds = (millis() - showerStart)/1000;
    int minutes = seconds / 60;
    seconds %= 60;
    lcd.print(" ");
    if (minutes < 10) {
      lcd.print(" ");
    }
    lcd.print(minutes);
    lcd.print(":");
    if (seconds < 10)
      lcd.print("0");
    lcd.print(seconds);
  } else {
    lcd.print("     ");
  }
}

void printUsage() {
  // Water usage
  gpm = map(val, 0, 1023, 1, 10);
  if (state == 0) {
    lcd.setCursor(0,1);
    lcd.print("               ");
  } else {
    lcd.setCursor(0,1);
    if (gpm < 10) {
      lcd.print(" ");
      lcd.print(gpm);
    } else {
      lcd.print(gpm);
    }
    lcd.setCursor(3,1);
    lcd.print("gpm / ");
    if (budget < 10) {
      lcd.print(" ");
      lcd.print(budget);
    } else {
      lcd.print(budget);
    }
    lcd.setCursor(12,1);
    lcd.print("gal");
  }
}

void fillStrip(uint32_t c, uint32_t on) {
  uint32_t black = strip.Color(0, 0, 0);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    if (i >= NUM_LEDS - on) {
      strip.setPixelColor(i, c);
    } else {
      strip.setPixelColor(i, black);
    }
  }
  //strip.setBrightness(10);
  strip.show();
}

void setColor() {
  if (state == 0) {
    ledColor = strip.Color(0, 0, 0);
  } else {
    int mapped = val;
    if (mapped < 1023/2) {
      mapped = map(mapped, 0, 1023/2, 0, 255);
      ledColor = strip.Color(mapped, 255, 0);
    } else {
      mapped = map(mapped, 1023/2, 1023, 0, 255);
      ledColor = strip.Color(255, 255-mapped, 0);
    }
  }
}

int numLEDsOn() {
  // Water budget in gallons scaled up by 2 orders of magnitude
  int waterBudgetScaled = WATER_BUDGET*1000;

  // Gallons used scaled
  int gallonsUsedScaled = (int)(gallonsUsed*1000.0);
  
  // Map this
  return NUM_LEDS - map(gallonsUsedScaled, 0, waterBudgetScaled, 0, NUM_LEDS);
}

