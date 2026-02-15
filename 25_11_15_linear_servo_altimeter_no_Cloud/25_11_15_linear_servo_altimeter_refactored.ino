/*
  Linear Servo Altimeter - REFACTORED VERSION
  
  Original: 25_11_15_linear_servo_altimeter_no_Cloud.ino
  Improvements:
  - Extracted magic numbers to named constants
  - Created helper functions for PWM signal generation
  - Consolidated altitude calculations
  - Added error handling and validation
  - Improved code organization and readability
  - Added detailed comments
  
  Hardware:
  - BMP280 pressure/altitude sensor (I2C)
  - HC-SR04 ultrasonic distance sensor (pins 11, 12)
  - Servo City linear actuator on pin 10 (LIN)
  - Dial servo on pin 9 (ALT)
  - 16x2 LCD display
*/

#include <LiquidCrystal.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>

// ============================================================================
// CONFIGURATION & CONSTANTS
// ============================================================================

// Barometric pressure reference (in-Hg) - UPDATE THIS FOR YOUR LOCATION
#define BAR_PRES 30.41

// Unit conversion factors
#define METERS_TO_FEET 3.2808
#define PA_TO_INHG 0.0002953
#define PASCALS_PER_INHG 33.86

// Sensor pins
#define TRIGGER_PIN 11
#define ECHO_PIN 12
#define ALT_PIN 9    // PWM for dial altimeter
#define LIN_PIN 10   // PWM for linear slider

// Altimeter servo calibration
#define ALT_PWM_BASE 2443        // Pulse width at 0 feet
#define ALT_PWM_ZERO_CYCLES 150  // Number of pulses to send at startup
#define ALT_PWM_CYCLES 300       // Number of pulses per measurement
#define ALT_PWM_SCALE 4.026      // Microseconds per foot (inverse: negative because higher alt = lower PW)

// Linear slider calibration
#define LIN_MIN_PW 820           // Pulse width for 0 feet
#define LIN_MAX_PW 2200          // Pulse width for 500 feet
#define LIN_RANGE_FEET 500       // Full range in feet
#define LIN_PWM_CYCLES 400       // Number of pulses per command

// Distance sensor thresholds
#define DISTANCE_WARNING_CM 30   // Trigger warning if closer than this
#define DISTANCE_TIMEOUT_US 30000  // Timeout for ultrasonic measurement

// Display timing
#define DISPLAY_DELAY_SHORT 1000   // 1 second
#define DISPLAY_DELAY_MEDIUM 2000  // 2 seconds
#define DISPLAY_DELAY_LONG 3000    // 3 seconds

// Sensor validation ranges
#define MIN_SAFE_ALTITUDE_FEET -500
#define MAX_SAFE_ALTITUDE_FEET 5000
#define MIN_SAFE_DISTANCE_CM 2
#define MAX_SAFE_DISTANCE_CM 400

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

Adafruit_BMP280 bmp;
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

float altitudeFeet = 0;
float altitudeMeters = 0;
float distanceCM = 0;
float temperatureC = 0;
float pressurePa = 0;

// ============================================================================
// INITIALIZATION
// ============================================================================

void setup() {
  // Initialize pins
  pinMode(ALT_PIN, OUTPUT);
  pinMode(LIN_PIN, OUTPUT);
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Ensure outputs start LOW
  digitalWrite(ALT_PIN, LOW);
  digitalWrite(LIN_PIN, LOW);
  digitalWrite(LED_BUILTIN, LOW);
  
  // Initialize serial for debugging
  Serial.begin(9600);
  delay(500);
  
  // Initialize LCD
  lcd.begin(16, 2);
  displayWelcomeScreen();
  
  // Initialize BMP280 sensor
  if (!initializeBMP280()) {
    displayError("BMP280 FAILED");
    while (1); // Halt - sensor required
  }
  
  displayBarometricPressure();
  
  Serial.println(F("System initialized successfully"));
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  // Read all sensors
  readAllSensors();
  
  // Display sensor readings
  displayTemperature();
  displayPressure();
  displayAltitude();
  
  // Measure and display distance
  measureDistance();
  
  // Update altimeter and slider displays
  updateAltimeterDisplay();
  updateSliderDisplay();
  
  // Small delay before next cycle
  delay(500);
}

// ============================================================================
// SENSOR READING FUNCTIONS
// ============================================================================

void readAllSensors() {
  // Read temperature
  temperatureC = bmp.readTemperature();
  
  // Read pressure
  pressurePa = bmp.readPressure();
  
  // Read altitude
  float altitudeMetersRaw = bmp.readAltitude(BAR_PRES * PASCALS_PER_INHG);
  altitudeMeters = altitudeMetersRaw;
  altitudeFeet = altitudeMeters * METERS_TO_FEET;
  
  // Validate altitude
  if (!isValidAltitude(altitudeFeet)) {
    Serial.println(F("WARNING: Invalid altitude reading"));
    altitudeFeet = 0; // Default to 0 on error
  }
}

boolean initializeBMP280() {
  if (!bmp.begin()) {
    Serial.println(F("BMP280 initialization failed"));
    return false;
  }
  
  // Configure sensor for high accuracy
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_X16,
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_500);
  
  Serial.println(F("BMP280 initialized successfully"));
  return true;
}

void measureDistance() {
  // Trigger ultrasonic sensor
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  
  // Measure pulse duration with timeout
  unsigned long pulseDuration = pulseIn(ECHO_PIN, HIGH, DISTANCE_TIMEOUT_US);
  
  // Convert to distance in centimeters
  // Speed of sound = 340 m/s, distance = speed * time / 2
  distanceCM = (pulseDuration * 340.0) / 20000.0;
  
  // Validate distance
  if (!isValidDistance(distanceCM)) {
    Serial.println(F("WARNING: Invalid distance reading"));
    displayDistance();
    return;
  }
  
  displayDistance();
  
  // Check for proximity warning
  if (distanceCM < DISTANCE_WARNING_CM) {
    triggerProximityWarning();
  }
}

// ============================================================================
// VALIDATION FUNCTIONS
// ============================================================================

boolean isValidAltitude(float alt) {
  return (alt >= MIN_SAFE_ALTITUDE_FEET && alt <= MAX_SAFE_ALTITUDE_FEET);
}

boolean isValidDistance(float dist) {
  return (dist >= MIN_SAFE_DISTANCE_CM && dist <= MAX_SAFE_DISTANCE_CM);
}

boolean isValidPWM(int pulseWidth, int minPW, int maxPW) {
  return (pulseWidth >= minPW && pulseWidth <= maxPW);
}

// ============================================================================
// PWM SIGNAL GENERATION
// ============================================================================

void sendPWMSignal(int pin, int pulseWidth, int cycles) {
  for (int i = 0; i < cycles; i++) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(pulseWidth);
    digitalWrite(pin, LOW);
    delay(20); // 20ms between cycles for servo settling
  }
}

void sendPWMSignalWithValidation(int pin, int pulseWidth, int minPW, int maxPW, int cycles) {
  // Constrain pulse width to safe range
  pulseWidth = constrain(pulseWidth, minPW, maxPW);
  
  sendPWMSignal(pin, pulseWidth, cycles);
}

// ============================================================================
// ALTIMETER DISPLAY FUNCTIONS
// ============================================================================

void updateAltimeterDisplay() {
  // Display "Pulse Width" header
  lcd.clear();
  lcd.print("Pulse Width");
  lcd.setCursor(0, 1);
  
  // Calculate pulse width for current altitude
  int altPW = ALT_PWM_BASE - (int)(altitudeFeet * ALT_PWM_SCALE);
  
  // Display the pulse width
  lcd.print(altPW);
  lcd.print(" us");
  delay(DISPLAY_DELAY_MEDIUM);
  
  // First, send zero-altitude pulse to initialize servo
  sendPWMSignal(ALT_PIN, ALT_PWM_BASE, ALT_PWM_ZERO_CYCLES);
  delay(1500);
  
  // Then send altitude-based pulse
  sendPWMSignalWithValidation(ALT_PIN, altPW, 1700, 2650, ALT_PWM_CYCLES);
  
  // Debug output
  Serial.print(F("Altimeter PW: "));
  Serial.print(altPW);
  Serial.println(F(" us"));
}

// ============================================================================
// SLIDER DISPLAY FUNCTIONS
// ============================================================================

void updateSliderDisplay() {
  displaySliderHeader();
  displaySliderZeroPosition();
  displaySliderMaxPosition();
  displaySliderCurrentPosition();
}

void displaySliderHeader() {
  lcd.clear();
  lcd.print("Linear Slider");
  lcd.setCursor(0, 1);
  lcd.print("Altitude Display");
  delay(DISPLAY_DELAY_SHORT);
}

void displaySliderZeroPosition() {
  lcd.clear();
  lcd.print("Zero Feet");
  lcd.setCursor(0, 1);
  lcd.print(LIN_MIN_PW);
  lcd.print("us");
  delay(DISPLAY_DELAY_SHORT);
  
  sendPWMSignal(LIN_PIN, LIN_MIN_PW, LIN_PWM_CYCLES);
  delay(1500);
}

void displaySliderMaxPosition() {
  lcd.clear();
  lcd.print("500 Feet");
  lcd.setCursor(0, 1);
  lcd.print(LIN_MAX_PW);
  lcd.print("us");
  delay(DISPLAY_DELAY_SHORT);
  
  sendPWMSignal(LIN_PIN, LIN_MAX_PW, LIN_PWM_CYCLES);
  delay(1500);
}

void displaySliderCurrentPosition() {
  // Display current altitude
  lcd.clear();
  lcd.print("Altitude");
  lcd.setCursor(0, 1);
  lcd.print((int)altitudeFeet);
  lcd.print("ft");
  delay(DISPLAY_DELAY_SHORT);
  
  // Calculate slider scaling factor
  float sliderScale = (LIN_MAX_PW - LIN_MIN_PW) / (float)LIN_RANGE_FEET;
  
  // Calculate pulse width for current altitude
  int sliderPW = (int)(altitudeFeet * sliderScale) + LIN_MIN_PW;
  
  // Display calculated pulse width
  lcd.clear();
  lcd.print("Slider PW");
  lcd.setCursor(0, 1);
  lcd.print(sliderPW);
  lcd.print("us");
  delay(DISPLAY_DELAY_LONG);
  
  // Send constrained pulse to slider
  sendPWMSignalWithValidation(LIN_PIN, sliderPW, LIN_MIN_PW, LIN_MAX_PW, LIN_PWM_CYCLES);
  
  // Debug output
  Serial.print(F("Slider PW: "));
  Serial.print(sliderPW);
  Serial.println(F(" us"));
}

// ============================================================================
// SENSOR DISPLAY FUNCTIONS
// ============================================================================

void displayTemperature() {
  lcd.clear();
  lcd.print("Temp= ");
  lcd.print(temperatureC);
  lcd.print(" C");
  delay(DISPLAY_DELAY_MEDIUM);
  
  Serial.print(F("Temperature: "));
  Serial.print(temperatureC);
  Serial.println(F(" *C"));
}

void displayPressure() {
  // Display pressure in Pascals
  lcd.clear();
  lcd.print("Press= ");
  lcd.print(pressurePa);
  delay(DISPLAY_DELAY_SHORT);
  
  // Display pressure in in-Hg
  lcd.setCursor(0, 1);
  float pressureInHg = pressurePa * PA_TO_INHG;
  lcd.print(pressureInHg);
  lcd.print(" in-Hg");
  delay(DISPLAY_DELAY_MEDIUM);
  
  Serial.print(F("Pressure: "));
  Serial.print(pressurePa);
  Serial.print(F(" Pa = "));
  Serial.print(pressureInHg);
  Serial.println(F(" in-Hg"));
}

void displayAltitude() {
  // Display in meters
  lcd.clear();
  lcd.print("Altitude=");
  lcd.setCursor(0, 1);
  lcd.print(altitudeMeters);
  lcd.print("m");
  delay(DISPLAY_DELAY_MEDIUM);
  
  // Display in feet
  lcd.clear();
  lcd.print("Altitude=");
  lcd.setCursor(0, 1);
  lcd.print((int)altitudeFeet);
  lcd.print(" feet");
  delay(DISPLAY_DELAY_MEDIUM);
  
  Serial.print(F("Altitude: "));
  Serial.print(altitudeMeters);
  Serial.print(F("m = "));
  Serial.print(altitudeFeet);
  Serial.println(F(" feet"));
}

void displayDistance() {
  // Display in centimeters
  lcd.clear();
  lcd.print("Distance:");
  lcd.print((int)distanceCM);
  lcd.print("cm");
  
  Serial.print(F("Distance: "));
  Serial.print(distanceCM);
  Serial.println(F(" cm"));
  
  delay(DISPLAY_DELAY_SHORT);
  
  // Display in meters
  lcd.setCursor(0, 1);
  lcd.print("Distance:");
  lcd.print(distanceCM / 100.0);
  lcd.print("m");
  delay(DISPLAY_DELAY_MEDIUM);
}

// ============================================================================
// WELCOME & ERROR DISPLAYS
// ============================================================================

void displayWelcomeScreen() {
  lcd.print(" Ultra sonic");
  lcd.setCursor(0, 1);
  lcd.print("Distance Meter");
  delay(DISPLAY_DELAY_SHORT);
  
  lcd.clear();
  lcd.print(" Circuit Digest");
  delay(DISPLAY_DELAY_SHORT);
  
  lcd.clear();
  lcd.print("Your Mom Wears");
  lcd.setCursor(0, 1);
  lcd.print("Army Boots");
  delay(DISPLAY_DELAY_SHORT);
  
  lcd.clear();
  lcd.print("BMP280 Test");
  lcd.setCursor(0, 1);
  lcd.print("Temp Altitude");
  delay(DISPLAY_DELAY_SHORT);
}

void displayBarometricPressure() {
  lcd.clear();
  lcd.print("Sea Level Press");
  lcd.setCursor(0, 1);
  lcd.print(BAR_PRES);
  lcd.print(" in-HG");
  delay(2000);
}

void displayError(const char* errorMsg) {
  lcd.clear();
  lcd.print("ERROR:");
  lcd.setCursor(0, 1);
  lcd.print(errorMsg);
  
  Serial.print(F("ERROR: "));
  Serial.println(errorMsg);
}

// ============================================================================
// WARNING/ALERT FUNCTIONS
// ============================================================================

void triggerProximityWarning() {
  // Blink LED
  digitalWrite(LED_BUILTIN, HIGH);
  delay(DISPLAY_DELAY_SHORT);
  digitalWrite(LED_BUILTIN, LOW);
  
  // Display warning message
  lcd.clear();
  lcd.print("Your Mom Wears");
  lcd.setCursor(0, 1);
  lcd.print("Cowboy Boots");
  delay(DISPLAY_DELAY_MEDIUM);
  
  Serial.println(F("PROXIMITY WARNING - Object detected within 30cm"));
}