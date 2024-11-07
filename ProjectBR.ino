#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

// Pins:
const int HX711_dout = 4; // mcu > HX711 dout pin
const int HX711_sck = 5; // mcu > HX711 sck pin

// HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAdress = 0;
unsigned long t = 0;
bool menuDisplayed = false; // Flag to check if menu has been displayed

void setup() {
  Serial.begin(57600); delay(10);
  Serial.println("Starting...");

  LoadCell.begin();
  unsigned long stabilizingtime = 2000; 
  boolean _tare = true; 
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  } else {
    LoadCell.setCalFactor(1.0); 
    Serial.println("Startup is complete");
  }
  while (!LoadCell.update());
}

void loop() {
  // Display menu once
  if (!menuDisplayed) {
    displayMenu();
    menuDisplayed = true; // Set flag to indicate menu has been displayed
  }

  // Check for user input and handle menu selection
  if (Serial.available() > 0) {
    char option = Serial.read();
    switch (option) {
      case '1':
        Serial.println("Calibration mode selected.");
        calibrate();
        menuDisplayed = false; // Reset flag to show menu after calibration
        clearSerialBuffer();   // Clear any leftover input
        break;
      case '2':
        Serial.println("Measurement mode selected.");
        measureMode();
        menuDisplayed = false; // Reset flag to show menu after measurement mode
        clearSerialBuffer();   // Clear any leftover input
        break;
      case '3':
        Serial.println("Exiting program.");
        while (1); // Stop the program
        break;
      default:
        Serial.println("Invalid option. Please select 1, 2, or 3.");
        clearSerialBuffer();   // Clear invalid input
        break;
    }
  }
}

// Function to display the main menu
void displayMenu() {
  Serial.println("\n--- Main Menu ---");
  Serial.println("1: Calibrate the scale");
  Serial.println("2: Measure mode");
  Serial.println("3: Quit program");
  Serial.println("Enter option:");
}

// Function to clear the serial buffer
void clearSerialBuffer() {
  while (Serial.available() > 0) {
    Serial.read(); // Clear any remaining characters
  }
}

// Calibration function
void calibrate() {
  Serial.println("*** Start calibration ***");
  Serial.println("Remove any load from the load cell.");
  Serial.println("Press 't' to tare.");

  boolean _resume = false;
  while (!_resume) {
    LoadCell.update();
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 't') {
        LoadCell.tareNoDelay();
        while (!LoadCell.getTareStatus()) LoadCell.update(); 
        Serial.println("Tare complete. Place known weight and enter its mass:");
        _resume = true;
      } else if (inByte == 'm') { // Return to menu
        Serial.println("Returning to main menu...");
        return;
      }
    }
  }

  _resume = false;
  float known_mass = 0;
  while (!_resume) {
    LoadCell.update();
    if (Serial.available() > 0) {
      known_mass = Serial.parseFloat();
      if (known_mass != 0) {
        Serial.print("Known mass is: ");
        Serial.println(known_mass);
        _resume = true;
      } else if (Serial.read() == 'm') {
        Serial.println("Returning to main menu...");
        return;
      }
    }
  }

  LoadCell.refreshDataSet();
  float newCalibrationValue = LoadCell.getNewCalibration(known_mass);

  Serial.print("New calibration value: ");
  Serial.println(newCalibrationValue);
  Serial.println("Save to EEPROM? (y/n)");

  _resume = false;
  while (!_resume) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.begin(512);
#endif
        EEPROM.put(calVal_eepromAdress, newCalibrationValue);
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.commit();
#endif
        Serial.println("Calibration value saved.");
        _resume = true;
      } else if (inByte == 'n') {
        Serial.println("Calibration value not saved.");
        _resume = true;
      } else if (inByte == 'm') {
        Serial.println("Returning to main menu...");
        return;
      }
    }
  }

  Serial.println("*** End calibration ***");
}

// Measurement mode function
void measureMode() {
  Serial.println("Entering measurement mode...");
  Serial.println("Press 'm' to return to the main menu.");

  while (true) {
    bool dataUpdated = LoadCell.update(); // Check if new data is available

    if (dataUpdated) {
      float weight = LoadCell.getData();
      Serial.print("Weight: ");
      Serial.println(weight);
    } else {
      Serial.println("Waiting for data...");
    }

    // Check if the user wants to return to the main menu
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'm') {
        Serial.println("Returning to main menu...");
        break;
      }
    }

    delay(200); // Short delay for serial stability
  }
}
