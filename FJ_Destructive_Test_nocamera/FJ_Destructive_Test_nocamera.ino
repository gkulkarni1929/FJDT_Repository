#include <WiFi.h>
#include <genieArduinoDEV.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

// WiFi credentials
const char* ssid = "SCB-Data-Collection";
const char* password = "Annoy-Frequency5";

const char* serverUrl = "http://192.168.120.19:5000/endpoint";  // Flask server APP01

Genie genie;
Adafruit_INA219 ina219;      //I2C
Adafruit_BME680 bme(&Wire);  // I2C
HardwareSerial HMI(2);

#define RESETLINE 4
#define LED_NUM 16
#define USER_LED_NUM 2
#define STRINGS_NUM 18
#define WOOD_BUTTONS_NUM 6

// Constants for PSI calculation
#define psi_a 16.894
#define psi_b 216.65
#define psi_c -1140.6
#define psi_threshold 400

// Constants for Load calculation
#define load_slope 9.918
#define load_constant -812.879
#define width 1.5

//Passing criteria constants for joint strength
#define load_uts_constant 2.1
#define load_c1_min 1
#define load_c2 1
#define load_c3 1

//Passing criteria constants for 5th percentile joint strength
#define load_c1_5th 1.25
//Array to store C3 factors for different dimenison of sample
const float C3_factors[5] = { 1, 1, 1, 1.15, 1.19 };

float height = 0.0;
float current_mA = 0.0;
float psi_reading = 0.0;
float load_reading = 0.0;
int ft_value;
float min_ft = 0.0;          // called as minimum joint strength UTS in PLIB book
float fifth_perc_ft = 0.0;   // called as 5th percentile joint strength UTS in PLIB book
float min_uts = 0.0;         // called as minimum joint strength Tension Proof Load (lbf) in PLIB book
float fifth_perc_uts = 0.0;  // called as 5th percentile joint strength Tension Proof Load (lbf) in PLIB book

//Variables for sample test information
int counter2 = 0;
bool flag = false;
bool flag2 = false;
bool flag_reset_values = false;
bool flag_send_data = false;
bool flag_temperature = false;
bool flag_wifi_nc = false;
int state = 0;

int httpResponseCode;

//Structure to send sample test information
typedef struct sample_test_info {
  String first_name = "";
  String project_no = "";
  String panel_id = "";
  String shift_id = "";
  int date = 0;
  int Time = 0;
  String species = "";
  String grade = "";
  String dimension = "";
  int mc_right = 0;
  int mc_left = 0;
  int wood_failure_mode = 0;
  String test_result = "";
  float max_psi_reading = 0.0;
  float max_load_reading = 0.0;
  String adhv_appli = "";
  String squeeze_out = "";
  String adhv_batch = "";
  String fin_joint_app = "";
  String pos_align = "";
};

sample_test_info sample;

int currentStringIndex = 0;  // Current string index (0 to 9)

// Array to map string indices to Genie object indices
const int string_indices[19] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18 };

const int buttonIndices[6] = { 2, 3, 4, 5, 6, 7 };

//Array to map custom keyboard button text
const String keyboard_text_val[59] = {
  "df", "hf", "spf", "#2", "sel_str", "excessive", "adequate", "insufficient",
  "good", "fair", "poor", "a", "b", "c", "d", "e", "f",
  "g", "h", "i", "2x4", "2x6", "2x8", "2x10", "2x12",
  "day", "night", "back", "0", "1", "2", "3", "4", "5", "6", "7", "8",
  "9", "-", "+", ">", "<", "j", "k", "l", "m", "n", "o",
  "p", "q", "r", "s", "t", "u",
  "v", "w", "x", "y", "z"
};

//Array to map custom keyboard button decimal values
const int keyboard_dec_val[59] = {
  1, 2, 3, 4, 5, 9, 10, 11, 12, 13, 14, 65, 66, 67, 68, 69,
  70, 71, 72, 73, 21, 22, 23, 24, 25, 26, 27, 8, 48, 49, 50,
  51, 52, 53, 54, 55, 56, 57, 45, 43, 98, 99, 74, 75, 76, 77,
  78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90

};

unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;

const unsigned long interval1 = 10;
const unsigned long interval2 = 10000;

// Set your Static IP address
IPAddress local_IP(192, 168, 141, 212);
// Set your Gateway IP address
IPAddress gateway(192, 168, 141, 1);
// Subnet Mask
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);    //optional
IPAddress secondaryDNS(8, 8, 4, 4);  //optional

void setup() {

  Serial.begin(115200);

  ina219.begin();
  ina219.setCalibration_16V_400mA();
  // ina219.setCalibration_32V_1A();

  bme.begin(0X76);
  bme.setTemperatureOversampling(BME680_OS_8X);

  // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }

  // // Set device as a Wi-Fi Station
  // WiFi.mode(WIFI_STA);

  // initWiFi();

  // // delete old config
  // WiFi.disconnect(true);

  // delay(1000);

  WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.begin(ssid, password);

  HMI.begin(115200, SERIAL_8N1, 16, 17);  // Begin Serial1 at 9600 baud rate, RX on GPIO 16, TX on GPIO 17
  resetHMI();
  genie.Begin(HMI);
  genie.AttachEventHandler(myGenieEventHandler);
  genie.WriteContrast(15);
}

void loop() {

  unsigned long currentMillis = millis();  // Get the current time
  genie.DoEvents();

  if (flag_temperature) {
    bme.performReading();
    genie.WriteStr(string_indices[18], "  " + String((int)bme.temperature));
    genie.WriteObject(GENIE_OBJ_USER_LED, 5, 1);
    flag_temperature = false;
  }
  if (flag_wifi_nc) {
    genie.WriteStr(string_indices[18], " ");
    genie.WriteObject(GENIE_OBJ_USER_LED, 5, 0);
    flag_wifi_nc = false;
  }

  if (flag_send_data) {
    sendSampleTestInfo();
    flag_send_data = false;
  }

  if (flag_reset_values) {
    resetAllTestInfo();
    flag_reset_values = false;
  }

  if (flag == true) {

    current_mA = ina219.getCurrent_mA();
    Serial.print(current_mA);
    Serial.print("  ");

    if (current_mA < 4.7) {
      current_mA = 0.0;
      psi_reading = 0.0;
    }

    if (current_mA > 20.0) {
      current_mA = 20.0;
    }

    psi_reading = (psi_a * current_mA * current_mA) + (psi_b * current_mA) + psi_c;
    load_reading = psi_reading * load_slope + load_constant;

    if (psi_reading < 0 || load_reading < 0) psi_reading = 0.0, load_reading = 0.0;

    genie.WriteObject(GENIE_OBJ_COOL_GAUGE, 0, psi_reading / 100.0);
    genie.WriteObject(GENIE_OBJ_COOL_GAUGE, 1, load_reading / 100.0);
    Serial.print(psi_reading);
    Serial.print("  ");
    Serial.print(load_reading);
    Serial.print("  ");
    Serial.print(sample.max_psi_reading);
    Serial.print("  ");
    Serial.print(sample.wood_failure_mode);
    Serial.print("  ");
    Serial.println(sample.test_result);

    if (psi_reading > sample.max_psi_reading) {
      sample.max_psi_reading = psi_reading;
      sample.max_load_reading = load_reading;
    }

    if (sample.max_psi_reading > psi_threshold) {
      flag2 = true;
    }

    genie.WriteObject(GENIE_OBJ_LED_DIGITS, 4, (long)sample.max_psi_reading);
    genie.WriteObject(GENIE_OBJ_LED_DIGITS, 5, (long)sample.max_load_reading);

    getpassingcriteria();


    if (sample.max_load_reading > fifth_perc_uts) {
      genie.WriteObject(GENIE_OBJ_USER_LED, 0, 1);
      genie.WriteObject(GENIE_OBJ_USER_LED, 1, 0);
      sample.test_result = "pass";
    } else if (sample.max_load_reading < fifth_perc_uts && sample.wood_failure_mode == 6) {
      genie.WriteObject(GENIE_OBJ_USER_LED, 0, 0);
      genie.WriteObject(GENIE_OBJ_USER_LED, 1, 1);
      sample.test_result = "na";
    } else if (sample.max_load_reading < fifth_perc_uts) {
      genie.WriteObject(GENIE_OBJ_USER_LED, 0, 0);
      genie.WriteObject(GENIE_OBJ_USER_LED, 1, 1);
      sample.test_result = "fail";
    }
  }
}


void myGenieEventHandler(void) {
  genieFrame Event;
  genie.DequeueEvent(&Event);

  if (Event.reportObject.cmd == GENIE_REPORT_EVENT) {
    if (Event.reportObject.object == GENIE_OBJ_KEYBOARD) {
      if (Event.reportObject.index == 0) {

        int temp = genie.GetEventData(&Event);

        // Handle right arrow key (ASCII 98)
        if (temp == keyboard_dec_val[40]) {
          if (currentStringIndex == 9) {
            currentStringIndex = 12;  // Jump to index 12
          } else if (currentStringIndex < 17) {
            currentStringIndex += 1;  // Increment index
          } else {
            currentStringIndex = 0;  // Wrap around to 0
          }
        }

        // Handle left arrow key (ASCII 99)
        if (temp == keyboard_dec_val[41]) {
          if (currentStringIndex == 0) {
            currentStringIndex = 17;  // Wrap around to 17
          } else if (currentStringIndex == 12) {
            currentStringIndex = 9;  // Jump back to index 9
          } else {
            currentStringIndex -= 1;  // Decrement index
          }
        }

        // Update LEDs based on currentStringIndex
        for (int i = 0; i < LED_NUM; i++) {
          if (currentStringIndex >= 0 && currentStringIndex <= 9) {
            // For string indices 0 to 9, map to LEDs 0 to 9
            if (i == currentStringIndex) {
              genie.WriteObject(GENIE_OBJ_LED, i, 1);  // Turn on LED
            } else {
              genie.WriteObject(GENIE_OBJ_LED, i, 0);  // Turn off LED
            }
          } else if (currentStringIndex >= 12 && currentStringIndex <= 17) {
            // For string indices 12 to 16, map to LEDs 10 to 14
            if (i == (currentStringIndex - 12 + 10)) {
              genie.WriteObject(GENIE_OBJ_LED, i, 1);  // Turn on LED
            } else {
              genie.WriteObject(GENIE_OBJ_LED, i, 0);  // Turn off LED
            }
          } else {
            // If currentStringIndex is not in a valid range, turn off all LEDs
            genie.WriteObject(GENIE_OBJ_LED, i, 0);
          }
        }

        switch (currentStringIndex) {
          case 0:  // Logic for Strings0 - Input Project_no
            Serial.println("Logic for project_no");
            if (sample.project_no.length() < 5) {
              if ((temp >= keyboard_dec_val[28] && temp <= keyboard_dec_val[37]) || (temp >= keyboard_dec_val[11] && temp <= keyboard_dec_val[19]) || (temp >= keyboard_dec_val[42] && temp <= keyboard_dec_val[58])) {
                sample.project_no += char(temp);
                genie.WriteStr(string_indices[currentStringIndex], sample.project_no);
              }
            }
            if (temp == keyboard_dec_val[27]) {  // Handle back button press (ASCII 8)
              if (sample.project_no.length() > 0) {
                sample.project_no.remove(sample.project_no.length() - 1);
                genie.WriteStr(string_indices[currentStringIndex], sample.project_no);
              }
            }
            break;

          case 1:  // Logic for Strings1 - Input panel_id

            Serial.println("Logic for panel_id");
            if (sample.panel_id.length() < 11) {
              if ((temp >= keyboard_dec_val[28] && temp <= keyboard_dec_val[37]) || temp == keyboard_dec_val[38] || temp == keyboard_dec_val[39] || (temp >= keyboard_dec_val[11] && temp <= keyboard_dec_val[19]) || (temp >= keyboard_dec_val[42] && temp <= keyboard_dec_val[58])) {  // all numbers plus A or B or P or + or -
                sample.panel_id += char(temp);
                genie.WriteStr(string_indices[currentStringIndex], sample.panel_id);
              }
            }
            if (temp == keyboard_dec_val[27]) {  // Handle back button press (ASCII 8)
              if (sample.panel_id.length() > 0) {
                sample.panel_id.remove(sample.panel_id.length() - 1);
                genie.WriteStr(string_indices[currentStringIndex], sample.panel_id);
              }
            }
            break;

          case 2:  // Logic for Strings2 - Input shift_id

            Serial.println("Logic for shift_id");
            for (int i = 25; i <= 26; i++) {
              if (temp == keyboard_dec_val[i]) {
                sample.shift_id = keyboard_text_val[i];
                genie.WriteStr(string_indices[currentStringIndex], String(toUpperCase(sample.shift_id)));
              }
            }

            break;

          case 3:  // Logic for Strings3 - Input date_id

            Serial.println("Logic for date");

            if (temp >= keyboard_dec_val[28] && temp <= keyboard_dec_val[37]) {
              int digit = temp - keyboard_dec_val[28];

              // Determine the number of digits entered so far
              int length = String(sample.date).length();

              if (length < 8) {  // Maximum of 8 digits (YYYYMMDD)
                sample.date = sample.date * 10 + digit;
                genie.WriteStr(string_indices[currentStringIndex], String(sample.date));
              }
            } else if (temp == keyboard_dec_val[27]) {  // Handle back button press (ASCII 8)
              sample.date = sample.date / 10;
              genie.WriteStr(string_indices[currentStringIndex], String(sample.date));
            }

            break;

          case 4:  // Logic for Strings4 - Input Time

            Serial.println("Logic for time");

            if (temp >= keyboard_dec_val[28] && temp <= keyboard_dec_val[37]) {
              int digit = temp - keyboard_dec_val[28];

              // Check if adding another digit exceeds 2 digits
              if (sample.Time < 10) {
                sample.Time = sample.Time * 10 + digit;
                genie.WriteStr(string_indices[currentStringIndex], String(sample.Time));
              }
            } else if (temp == keyboard_dec_val[27]) {  // Handle back button press (ASCII 8)
              sample.Time = sample.Time / 10;
              genie.WriteStr(string_indices[currentStringIndex], String(sample.Time));
            }

            break;

          case 5:  // Logic for Strings5 - Input speciesio

            Serial.println("Logic for species");

            for (int i = 0; i <= 2; i++) {
              if (temp == keyboard_dec_val[i]) {
                sample.species = keyboard_text_val[i];
                genie.WriteStr(string_indices[currentStringIndex], String(toUpperCase(sample.species)));
              }
            }

            break;

          case 6:  // Logic for Strings6 - Input grade

            Serial.println("Logic for grade");

            for (int i = 3; i <= 4; i++) {
              if (temp == keyboard_dec_val[i]) {
                sample.grade = keyboard_text_val[i];
                genie.WriteStr(string_indices[currentStringIndex], String(toUpperCase(sample.grade)));
              }
            }

            break;

          case 7:  // Logic for Strings7 - Input dimension

            Serial.println("Logic for dimension");

            for (int i = 20; i <= 24; i++) {
              if (temp == keyboard_dec_val[i]) {
                sample.dimension = keyboard_text_val[i];
                genie.WriteStr(string_indices[currentStringIndex], String(toUpperCase(sample.dimension)));
              }
            }

            break;

          case 8:  // Logic for Strings8 - Input mc_right

            Serial.println("Logic for mc_right");

            if (temp >= keyboard_dec_val[28] && temp <= keyboard_dec_val[37]) {
              int digit = temp - keyboard_dec_val[28];

              // Check if adding another digit exceeds 2 digits
              if (sample.mc_right < 10) {
                sample.mc_right = sample.mc_right * 10 + digit;
                genie.WriteStr(string_indices[currentStringIndex], String(sample.mc_right));
              }
            } else if (temp == keyboard_dec_val[27]) {  // Handle back button press (ASCII 8)
              sample.mc_right = sample.mc_right / 10;
              genie.WriteStr(string_indices[currentStringIndex], String(sample.mc_right));
            }

            break;

          case 9:  // Logic for Strings9 - Input mc_left

            Serial.println("Logic for mc_left");

            if (temp >= keyboard_dec_val[28] && temp <= keyboard_dec_val[37]) {
              int digit = temp - keyboard_dec_val[28];

              // Check if adding another digit exceeds 2 digits
              if (sample.mc_left < 10) {
                sample.mc_left = sample.mc_left * 10 + digit;
                genie.WriteStr(string_indices[currentStringIndex], String(sample.mc_left));
              }
            } else if (temp == keyboard_dec_val[27]) {  // Handle back button press (ASCII 8)
              sample.mc_left = sample.mc_left / 10;
              genie.WriteStr(string_indices[currentStringIndex], String(sample.mc_left));
            }

            break;

          case 12:

            Serial.println("Logic for adhv_appli");

            for (int i = 5; i <= 7; i++) {
              if (temp == keyboard_dec_val[i]) {
                sample.adhv_appli = keyboard_text_val[i];
                genie.WriteStr(string_indices[currentStringIndex], String(toUpperCase(sample.adhv_appli)));
              }
            }

            break;

          case 13:

            Serial.println("Logic for squeeze_out");

            for (int i = 5; i <= 7; i++) {
              if (temp == keyboard_dec_val[i]) {
                sample.squeeze_out = keyboard_text_val[i];
                genie.WriteStr(string_indices[currentStringIndex], String(toUpperCase(sample.squeeze_out)));
              }
            }

            break;

          case 14:

            Serial.println("Logic for adhv_batch");

            for (int i = 8; i <= 10; i++) {
              if (temp == keyboard_dec_val[i]) {
                sample.adhv_batch = keyboard_text_val[i];
                genie.WriteStr(string_indices[currentStringIndex], String(toUpperCase(sample.adhv_batch)));
              }
            }

            break;

          case 15:

            Serial.println("Logic for fin_joint_app");

            for (int i = 8; i <= 10; i++) {
              if (temp == keyboard_dec_val[i]) {
                sample.fin_joint_app = keyboard_text_val[i];
                genie.WriteStr(string_indices[currentStringIndex], String(toUpperCase(sample.fin_joint_app)));
              }
            }

            break;

          case 16:

            Serial.println("Logic for pos_align");

            for (int i = 8; i <= 10; i++) {
              if (temp == keyboard_dec_val[i]) {
                sample.pos_align = keyboard_text_val[i];
                genie.WriteStr(string_indices[currentStringIndex], String(toUpperCase(sample.pos_align)));
              }
            }

            break;

          case 17:

            Serial.println("Logic for operator name");

            if (temp >= keyboard_dec_val[11] && temp <= keyboard_dec_val[19] || temp >= keyboard_dec_val[42] && temp <= keyboard_dec_val[58]) {
              sample.first_name += char(temp);
              // Convert first_name to lowercase
              sample.first_name = toLowerCase(sample.first_name);
              genie.WriteStr(string_indices[currentStringIndex], sample.first_name);
            }
            if (temp == keyboard_dec_val[27]) {  // Handle back button press (ASCII 8)
              if (sample.first_name.length() > 0) {
                sample.first_name.remove(sample.first_name.length() - 1);
                // Convert first_name to lowercase
                sample.first_name = toLowerCase(sample.first_name);
                genie.WriteStr(string_indices[currentStringIndex], sample.first_name);
              }
            }

            break;
        }
      }
    }
    //Submit test information
    if (Event.reportObject.object == GENIE_OBJ_4DBUTTON) {
      if (Event.reportObject.index == 1) {

        // Check if inputs are valid before allowing the button to proceed
        if (!isInputValid()) {  //make this ! to enable validation
          // Optionally, display a message to the user indicating that input is invalid
          genie.WriteStr(string_indices[10], "                    Please fill all fields correctly!");
          counter2 = 0;
          return;  // Exit the function if input is not valid
        }

        // Handle button press logic based on counter2 value
        if (counter2 == 0) {
          genie.WriteStr(string_indices[10], "                    Double check all information!!");
          counter2 = 1;
        } else if (counter2 == 1) {
          Serial.println("Test Submitted");
          genie.SetForm(2);
          flag = true;
          counter2 = 0;  // Reset counter2
        }
      }
    }
    //finish recording data
    if (Event.reportObject.object == GENIE_OBJ_4DBUTTON) {
      if (Event.reportObject.index == 9) {
        if (!flag2) {
          Serial.println("cannot press button");
        } else {
          genie.SetForm(3);
        }
      }
    }
    //Finish test button
    if (Event.reportObject.object == GENIE_OBJ_4DBUTTON) {
      if (Event.reportObject.index == 8) {
        if (state != 0) {
          if (counter2 == 0) {
            flag_send_data = true;
            Serial.println("data sent");
            genie.WriteStr(string_indices[11], " ");
            genie.WriteStr(string_indices[11], "                           Test data sent successfully");
            counter2 = 1;
          } else if (counter2 == 1) {
            // esp_restart();
            flag_reset_values = true;
            genie.SetForm(1);
            counter2 = 0;
            // resetAllTestInfo();
          }
        }
      }
    }
    //wood failure buttons
    if (Event.reportObject.object == GENIE_OBJ_4DBUTTON) {
      // Array of button indices and corresponding wood failure mode values
      const int buttonIndices[6] = { 2, 3, 4, 5, 6, 7 };
      const int woodFailureModes[6] = { 1, 2, 3, 4, 5, 6 };
      const int woodFailurePercentages[6] = { 0, 0, 75, 85, 90, 100 };  // Percentages for modes 3-6

      // Iterate through each button index
      for (int i = 0; i < 6; i++) {
        if (Event.reportObject.index == buttonIndices[i]) {
          state = genie.GetEventData(&Event);
          if (state == 1) {  // Button is ON
            sample.wood_failure_mode = woodFailureModes[i];
            Serial.println(sample.wood_failure_mode);

            // Prepare the display string for modes 3-6
            String displayText = "                         Wood failure mode : " + String(woodFailureModes[i]);
            if (woodFailureModes[i] >= 3 && woodFailureModes[i] <= 6) {
              displayText += ", " + String(woodFailurePercentages[i]) + "%";
            }

            // Write the display text to String 11
            genie.WriteStr(string_indices[11], displayText.c_str());

            // Turn off other buttons
            for (int j = 0; j < 6; j++) {
              if (j != i) {
                genie.WriteObject(GENIE_OBJ_4DBUTTON, buttonIndices[j], 0);
              }
            }
          } else if (state == 0) {  // Button is OFF
            sample.wood_failure_mode = 0;
            // Clear String 11 when the button is turned off
            genie.WriteStr(string_indices[11], "");
          }
          break;
        }
      }
    }
  }
}

bool isInputValid() {
  // Check if project_no is a valid length (between 3 and 5 characters)
  if (sample.project_no.length() < 3 || sample.project_no.length() > 5) return false;

  // Check if panel_id has been set and is not empty
  if (sample.panel_id.length() == 0) return false;

  // Check if shift_id has been set and is not empty
  if (sample.shift_id.length() == 0) return false;

  // Check if date is exactly eight digits long
  if (sample.date < 10000000 || sample.date > 99999999) return false;

  // Extract month, day, and year from date
  int dateInt = sample.date;
  int year = dateInt / 10000;         // First four digits
  int month = (dateInt / 100) % 100;  // Next two digits
  int day = dateInt % 100;            // Last two digits

  // Validate year (assuming year should be from 2000-2099)
  if (year < 2000 || year > 2099) return false;

  // Check if month is valid (01-12)
  if (month < 1 || month > 12) return false;

  // Check if day is valid (01-31)
  if (day < 1 || day > 31) return false;

  // Check if day is valid for the specific month
  if (month == 2) {              // February
    if (day > 29) return false;  // Max 29 days in February
    if (day == 29 && !((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
      return false;  // Not a leap year
    }
  } else if (month == 4 || month == 6 || month == 9 || month == 11) {  // April, June, September, November
    if (day > 30) return false;                                        // Max 30 days in these months
  }

  // Check if Time is valid (should be between 0 and 23, inclusive) 24 hour format
  // if (sample.Time < 1 || sample.Time > 24) return false;

  // Check if Time is valid based on shift_id
  if (sample.shift_id == "day") {
    // Valid times for the "day" shift
    if (sample.Time < 6 || sample.Time > 15) return false;
  } else if (sample.shift_id == "night") {
    // Valid times for the "night" shift
    if ((sample.Time < 1 || sample.Time > 3) && (sample.Time < 16 || sample.Time > 24)) return false;
  }

  // Check if species has been set and is not empty
  if (sample.species.length() == 0) return false;

  // Check if grade has been set and is not empty
  if (sample.grade.length() == 0) return false;

  // Check if dimension has been set and is not empty
  if (sample.dimension.length() == 0) return false;

  // Check if mc_right is valid (assumed to be a single-digit number)
  if (sample.mc_right < 1 || sample.mc_right > 99) return false;

  // Check if mc_left is valid (assumed to be a single-digit number)
  if (sample.mc_left < 1 || sample.mc_left > 99) return false;

  // Check if adhv_appli has been set and is not empty
  if (sample.adhv_appli.length() == 0) return false;

  // Check if squeeze_out has been set and is not empty
  if (sample.squeeze_out.length() == 0) return false;

  // Check if adhv_batch has been set and is not empty
  if (sample.adhv_batch.length() == 0) return false;

  // Check if fin_joint_app has been set and is not empty
  if (sample.fin_joint_app.length() == 0) return false;

  // Check if pos_align has been set and is not empty
  if (sample.pos_align.length() == 0) return false;

  // Check if first_name is valid (must be one of the specified names)
  if (sample.first_name != "nolan" && sample.first_name != "talon" && sample.first_name != "morgan" && sample.first_name != "dean" && sample.first_name != "wondwossen" && sample.first_name != "tahirih" && sample.first_name != "gourang") {
    return false;  // Invalid name
  }

  // All checks passed
  return true;
}

// Function to convert a String to uppercase
String toUpperCase(String str) {
  for (int i = 0; i < str.length(); i++) {
    str[i] = toupper(str[i]);
  }
  return str;
}

// Function to convert a String to uppercase
String toLowerCase(String str) {
  for (int i = 0; i < str.length(); i++) {
    str[i] = tolower(str[i]);
  }
  return str;
}


void printSampleTestInfo(const sample_test_info& sample) {
  Serial.printf("First Name: %s\n", sample.first_name.c_str());
  Serial.printf("Project Number: %s\n", sample.project_no.c_str());
  Serial.printf("Panel ID: %s\n", sample.panel_id.c_str());
  Serial.printf("Shift ID: %s\n", sample.shift_id.c_str());
  Serial.printf("Date: %d\n", sample.date);
  Serial.printf("Time: %d\n", sample.Time);
  Serial.printf("Species: %s\n", sample.species.c_str());
  Serial.printf("Grade: %s\n", sample.grade.c_str());
  Serial.printf("Dimension: %s\n", sample.dimension.c_str());
  Serial.printf("MC Right: %d\n", sample.mc_right);
  Serial.printf("MC Left: %d\n", sample.mc_left);
  Serial.printf("Wood Failure Mode: %d\n", sample.wood_failure_mode);
  Serial.printf("Test Result: %s\n", sample.test_result.c_str());
  Serial.printf("Max Psi Reading: %f\n", sample.max_psi_reading);
  Serial.printf("Max Load Reading: %f\n", sample.max_load_reading);
}

void resetAllTestInfo() {

  sample.project_no = "";
  sample.panel_id = "";
  sample.shift_id = "";
  sample.date = 0;
  sample.Time = 0;
  sample.species = "";
  sample.grade = "";
  sample.dimension = "";
  sample.mc_right = 0;
  sample.mc_left = 0;
  sample.wood_failure_mode = 0;
  sample.test_result = "";
  sample.max_psi_reading = 0.0;
  sample.max_load_reading = 0.0;
  sample.adhv_appli = "";
  sample.squeeze_out = "";
  sample.adhv_batch = "";
  sample.fin_joint_app = "";
  sample.pos_align = "";
  min_ft = 0.0;          // called as minimum joint strength UTS in PLIB book
  fifth_perc_ft = 0.0;   // called as 5th percentile joint strength UTS in PLIB book
  min_uts = 0.0;         // called as minimum joint strength Tension Proof Load (lbf) in PLIB book
  fifth_perc_uts = 0.0;  // called as 5th percentile joint strength Tension Proof Load (lbf) in PLIB book

  counter2 = 0;
  currentStringIndex = 0;
  flag = false;
  flag2 = false;

  for (int i = 0; i < STRINGS_NUM; i++) {
    genie.WriteStr(i, "");
  }
  for (int i = 0; i < LED_NUM; i++) {
    genie.WriteObject(GENIE_OBJ_LED, i, 0);  //turn all string editing indicator LEDs off
  }
  for (int i = 0; i < USER_LED_NUM; i++) {
    genie.WriteObject(GENIE_OBJ_USER_LED, i, 0);  //turn all string editing indicator LEDs off
  }
  for (int i = 0; i < WOOD_BUTTONS_NUM; i++) {
    genie.WriteObject(GENIE_OBJ_4DBUTTON, buttonIndices[i], 0);  //turn all string editing indicator LEDs off
  }

  genie.WriteObject(GENIE_OBJ_LED_DIGITS, 0, 0);
  genie.WriteObject(GENIE_OBJ_LED_DIGITS, 1, 0);
  genie.WriteObject(GENIE_OBJ_LED_DIGITS, 2, 0);
  genie.WriteObject(GENIE_OBJ_LED_DIGITS, 3, 0);
}
// Function to get `ft_value` based on the sample structure's species, grade, and dimension
float getFtValue(const sample_test_info& sample) {
  // Define `ft_value` lookup table based on species, grade, and dimension
  if (sample.species == "df" && sample.grade == "#2") {
    if (sample.dimension == "2x12") return 500;
    if (sample.dimension == "2x10") return 550;
    if (sample.dimension == "2x8") return 600;
    if (sample.dimension == "2x6") return 650;
    if (sample.dimension == "2x4") return 750;
  }
  if (sample.species == "df" && sample.grade == "sel_str") {
    if (sample.dimension == "2x12") return 830;
    if (sample.dimension == "2x10") return 910;
    if (sample.dimension == "2x6") return 1080;
    if (sample.dimension == "2x4") return 1240;
  }
  if (sample.species == "hf" && sample.grade == "#2") {
    if (sample.dimension == "2x12") return 580;
    if (sample.dimension == "2x10") return 640;
    if (sample.dimension == "2x8") return 690;
    if (sample.dimension == "2x6") return 750;
    if (sample.dimension == "2x4") return 870;
  }
  if (sample.species == "hf" && sample.grade == "sel_str") {
    if (sample.dimension == "2x12") return 780;
    if (sample.dimension == "2x10") return 860;
    if (sample.dimension == "2x8") return 930;
    if (sample.dimension == "2x6") return 1010;
    if (sample.dimension == "2x4") return 1170;
  }
  if (sample.species == "spf" && sample.grade == "#2") {
    if (sample.dimension == "2x12") return 450;
    if (sample.dimension == "2x10") return 500;
    if (sample.dimension == "2x8") return 540;
    if (sample.dimension == "2x6") return 590;
    if (sample.dimension == "2x4") return 680;
  }
  if (sample.species == "spf" && sample.grade == "sel_str") {
    if (sample.dimension == "2x12") return 700;
    if (sample.dimension == "2x10") return 770;
    if (sample.dimension == "2x8") return 840;
    if (sample.dimension == "2x6") return 910;
    if (sample.dimension == "2x4") return 1050;
  }

  return -1;  // Return an invalid value if no match is found
}

void getpassingcriteria() {

  // Determine `ft_value` based on species, grade, and dimension
  ft_value = getFtValue(sample);
  // Determine height based on dimension
  if (sample.dimension == "2x4") height = 3.5;
  else if (sample.dimension == "2x6") height = 5.5;
  else if (sample.dimension == "2x8") height = 7.5;
  else if (sample.dimension == "2x10") height = 4.6;
  else if (sample.dimension == "2x12") height = 5.6;

  min_ft = load_uts_constant * ft_value * load_c1_min * load_c2 * C3_factors[(sample.dimension == "2x4" ? 0 : (sample.dimension == "2x6" ? 1 : (sample.dimension == "2x8" ? 2 : (sample.dimension == "2x10" ? 3 : 4))))];
  fifth_perc_ft = load_uts_constant * ft_value * load_c1_5th * load_c2 * C3_factors[(sample.dimension == "2x4" ? 0 : (sample.dimension == "2x6" ? 1 : (sample.dimension == "2x8" ? 2 : (sample.dimension == "2x10" ? 3 : 4))))];
  min_uts = min_ft * width * height;
  fifth_perc_uts = fifth_perc_ft * width * height;

  genie.WriteObject(GENIE_OBJ_LED_DIGITS, 0, min_ft);
  genie.WriteObject(GENIE_OBJ_LED_DIGITS, 1, fifth_perc_ft);
  genie.WriteObject(GENIE_OBJ_LED_DIGITS, 2, min_uts);
  genie.WriteObject(GENIE_OBJ_LED_DIGITS, 3, fifth_perc_uts);
}

void resetHMI() {
  pinMode(RESETLINE, OUTPUT);
  digitalWrite(RESETLINE, 0);
  delay(100);
  digitalWrite(RESETLINE, 1);
  delay(5000);
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
  }
  Serial.println(WiFi.localIP());
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("Connected to AP successfully!");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  flag_temperature = true;
  Serial.println(WiFi.localIP());
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.wifi_sta_disconnected.reason);
  flag_wifi_nc = true;
  Serial.println("Trying to Reconnect");
  WiFi.begin(ssid, password);
}

// Function to format date as YYYY-MM-DD
String formatDate(int date) {
  int year = date / 10000;         // Extract year
  int month = (date / 100) % 100;  // Extract month
  int day = date % 100;            // Extract day

  // Create formatted date string
  char dateStr[11];
  snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d", year, month, day);

  return String(dateStr);
}

String padNumber(int num, int length) {
  String str = String(num);
  while (str.length() < length) {
    str = "0" + str;
  }
  return str;
}

// Function to send data to the server
void sendSampleTestInfo() {

  HTTPClient http;

  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");

  // Create JSON document and add data from the structure
  StaticJsonDocument<8192> jsonDoc;

  jsonDoc["operator_first_name"] = sample.first_name;

  // Ensure project_id starts with 'S'
  String projectId = sample.project_no;
  if (!projectId.startsWith("S")) {
    projectId = "S" + projectId;
  }
  jsonDoc["project_id"] = projectId;

  // Ensure panel_id starts with 'P'
  String panelId = sample.panel_id;
  if (!panelId.startsWith("P")) {
    panelId = "P" + panelId;
  }
  jsonDoc["panel_id"] = panelId;

  jsonDoc["shift_id"] = sample.shift_id;
  // Format sample_date
  String formattedDate = formatDate(sample.date);  // Assuming sample.date is in YYYYMMDD format
  jsonDoc["sample_date"] = formattedDate;

  // // Use sample.Time and shift_id for time formatting
  // int hour = sample.Time;  // 1-12 format from user input

  // // Convert 12-hour time to 24-hour time based on shift_id (AM/PM logic)
  // if (sample.shift_id == "pm" && hour != 12) {
  //   hour += 12;  // Convert PM to 24-hour time
  // } else if (sample.shift_id == "am" && hour == 12) {
  //   hour = 0;  // Midnight case
  // }
  // Format as HH:MM:SS (assuming minutes and seconds are 00 for simplicity)
  String formattedTime = padNumber(sample.Time, 2) + ":00:00";
  jsonDoc["sample_time"] = formattedTime;

  jsonDoc["specie"] = sample.species;
  jsonDoc["grade"] = sample.grade;
  jsonDoc["dimension"] = sample.dimension;
  jsonDoc["mc_right"] = sample.mc_right;
  jsonDoc["mc_left"] = sample.mc_left;
  jsonDoc["test_result"] = sample.test_result;
  jsonDoc["max_psi_reading"] = sample.max_psi_reading;
  jsonDoc["max_load_reading"] = sample.max_load_reading;
  jsonDoc["wood_failure_mode"] = sample.wood_failure_mode;
  jsonDoc["min_ft_psi"] = min_ft;
  jsonDoc["fifth_ft_psi"] = fifth_perc_ft;
  jsonDoc["min_uts_lbs"] = min_uts;
  jsonDoc["fifth_uts_lbs"] = fifth_perc_uts;
  jsonDoc["adhesive_application"] = sample.adhv_appli;
  jsonDoc["squeeze_out"] = sample.squeeze_out;
  jsonDoc["adhesive_batch_test_result"] = sample.adhv_batch;
  jsonDoc["finished_joint_appearance"] = sample.fin_joint_app;
  jsonDoc["positioning_alignment"] = sample.pos_align;

  String jsonData;
  serializeJson(jsonDoc, jsonData);

  Serial.println("JSON Data Length: " + String(jsonData.length()));

  // Set Content-Length header
  http.addHeader("Content-Length", String(jsonData.length()));

  // Send POST request
  int httpResponseCode = http.POST(jsonData);
  Serial.println(httpResponseCode);

  // Check response
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Response: " + response);
  } else {
    Serial.printf("Error on sending POST, HTTP Response Code: %d\n", httpResponseCode);
    String response = http.getString();
    Serial.println("Response: " + response);
  }

  http.end();
}