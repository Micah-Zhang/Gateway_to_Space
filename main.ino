//NOTE: TO DOs FOR CODE
//Decide how many tests to take and for how long we should wait to collect air
//Its not good to constantly store measurements to the SD Card. This is why we used "LOG_INTERVAL" in the original code. Otherwise, the SD card will fill up too quickly. Instead, use millis to time the SD Card storage
//dataString = "";, which clears out the data string, needs to be correctly implemented. This makse sure we only print the current measurement rather than every measurement we've ever taken
//"Serial.println()" adds a new line between each data measurement. Its the equivalent of pressing ENTER in microsoft word. I do not believe that this has been implemented properly.
//Take more measurements by using a shorter test cycle

//Include Files for SD card
#include <SD.h>
#include <SPI.h>
#include <Servo.h>

// Initializes servo
Servo myservo;

//Variables used for SD Card
uint32_t timeStamp = 0;     // Time keeper. The time stamp used when recording data points
const int chipSelect = 8;  // This is set to 8 for the SparkFun uSDCard shield
char logFileName[16]; // Variable for file name
String dataString = ""; //Holds data from a read cycle (SD Card)
String sensorNames = "Time Stamp (ms), Pressure (psi), Fluorometer (V)";
const int LOG_INTERVAL = 500;
int datayes  = 0;
int fluoroyes = 0;

//Variables for Presssure Reading
float pressureReading;
float pressureVolt;
float psi;
char psiString[7];

//Variables for Fluorometer Reading and Testing
float fluoroVolt;
char fluoroVString[7];
int c; //Prevents us from opening cap twice
int p; //Prevents us from closing cap twice
int a; //Allows us to reset time after a fluorometer test has been taken

//Fluorometer Calibration Variables
float fc = 20;
int sencals = 100; //Sensor calibration speed
int servs = 100; //Servo speed

//Variables for Time Keeping
unsigned long ccinterval = 50000; // HOW LONG DO WE WANT TO COLLECT ALGAE?
unsigned long previousMillis;
unsigned long previousMillis1;
unsigned long previousMillis2;
unsigned long currentMillis;

//Other fluorometer constants
const int LIGHT_PIN = A0; // Pin connected to voltage divider output
const int LED_PIN = 13; // Use built-in LED as dark indicator

// Measure the voltage at 5V and the actual resistence of your
// 47k resistor, and enter them below:
const float VCC = 4.98; // Measured voltage of Arduino 5V line
const float R_DIV = 4660.0; // Measured resistance of 3.3k resistor

// Set this to the minimum resistance required to turn an LED on:
const float DARK_THRESHOLD = 10000.0;

//Variables for Pressure Average Linked List
int pressurenum = 10;
float targ = 10; //10
float avg;
int i;
float presA;
float sum;
typedef struct pressurevalues {
  float pressure; //For some reason I can't access this pressure value anywhere else in the code.
  struct pressurevalues *next;
} pressurevalues_t;

//Variables used for Cap Recalibration
int cloud1 = 7.81; //Pressure corresponding with the altitude of Altostratus Clouds
int cloud2 = 7.34; //Pressure corresponding with the altitude of Nimbostratus and Altocumulus Clouds
int cloud3 = 2.47; //Pressure corresponding with the altitude of Cirrostratus Clouds
int cloud4 = 2.14; //Pressure corresponding with the altitude of Cirrus, Cirrocumulus, and Cumulonimbus Clouds
int c1 = 0; //Prevents us from recalibrating cloud 1 twice
int c2 = 0; //Prevents us from recalibrating cloud 2 twice
int c3 = 0; //Prevents us from recalibrating cloud 3 twice
int c4 = 0; //Prevents us from recalibrating cloud 4 twice

//Variables Used for Cap Operation
unsigned long turnint = 20000;
float currentMax;
float currentMin;
float newReading;
int cc = 0;
int co = 0;
int ttake;

void setup() {
  Serial.begin(9600); // Sets frequency of communication to arudino
  pinMode(7, OUTPUT); // Configures Blue LED
  digitalWrite(7, HIGH);
  ttake = 0; // Cap is initially closed
  myservo.attach(9); //Servo must be attached to pin 9
  pinMode(10, OUTPUT); // Part of SD Card Initialization
  pinMode(chipSelect, OUTPUT); // Part of SD Card Initialization

  SDCardInit(); //Initializes the SD card. This function also assigns the header sensorNames to the file
}

void loop() {
  if (ttake == 0) {
    //Serial.println("entering pressure loop");
    pressurevalues_t * temp = malloc(sizeof(struct pressurevalues)); //Allocates memory to be manipulated by a pointer temp, which points to the struct "pressurevalues"
    pressurevalues_t * HEAD = temp; //Allocates memory to be manipulated by a pointer temp, which points to the struct "pressurevalues"
    pressureReading = analogRead(A3);
    pressureVolt = pressureReading * (5.0 / 1023); //This should use the same pressure reading that was assgined to the pointer "temp2"
    psi = (pressureVolt - 0.5) * (15.0 / 4.0);
    temp->pressure = psi; //Takes a pressure reading, assigns it to pressure, then assigns temp to point to this pressure reading
    temp->next  = NULL; //Assigns the value of next, which points to the struct pressurevalues, to be NULL and then assigns that to temp
    //HEAD = temp; //Defines HEAD as the value of temp
    previousMillis = millis();
    pressurevalues_t * temp2 = NULL;
    for (i = 0; i < pressurenum; i++) { //Enters a for loop that should create a struct with an amount of values determined by pressurenum
      temp2 = (pressurevalues_t *) malloc(sizeof(struct pressurevalues)); //Allocates memory to be manipulated by a pointer temp2, which points to the struct "pressurevalues"
      temp->next = temp2; //Sets next, which points to the struct "pressurevalues" to equal temp2, and then assigns temp to temp2
      pressureReading = analogRead(A3);
      pressureVolt = pressureReading * (5.0 / 1023); //This should use the same pressure reading that was assgined to the pointer "temp2"
      psi = (pressureVolt - 0.5) * (15.0 / 4.0);
      temp2->pressure = psi; //Takes a pressure reading and sets it equal to pressure, and then tells temp2 to point to the value of pressure
      //Serial.println(temp2->pressure, 2);
      //Serial.println("Getting Pressure");
      temp2->next = NULL; //Tells temp2 to point to next, which points to the struct "pressurevalues", which is set equal to NULL
      temp = temp2;//Refines temp to equal temp 2

      currentMillis = millis();
      if ((currentMillis - previousMillis) >= LOG_INTERVAL) {
        dataString = "";

        timeStamp = millis();
        dataString = String(timeStamp);
        Serial.print(timeStamp);

        pressureReading = analogRead(A3);
        pressureVolt = pressureReading * (5.0 / 1023); //This should use the same pressure reading that was assgined to the pointer "temp2"
        psi = (pressureVolt - 0.5) * (15.0 / 4.0);
        Serial.print(','); //Not sure what this does
        //Serial.print("\t PSI ");
        Serial.print(psi, 2);
        dtostrf(psi, 6, 2, psiString);              // Convert psi into a String
        dataString = dataString + "," + psiString;  // Add to the dataString

        fluoroVolt = 0;
        Serial.print(',');
        //Serial.print("\t V ");
        Serial.print(fluoroVolt, 2);
        dtostrf(fluoroVolt, 6, 2, fluoroVString);              // Convert voltage into a String
        dataString = dataString + "," + fluoroVString;  // Add to the dataString */

        Serial.println();
        writeDataToSD();

        previousMillis = millis();
      }
    }
    //free(temp2); //"Frees" or deletes temp2 so that no memory is wasted
    struct pressurevalues * TAIL = malloc(sizeof(struct pressurevalues)); //Allocates memory to be manipulated by a pointer TAIL, which points to the struct "pressurevalues"
    TAIL = temp; //Defines TAIL as temp
    free(temp); //Frees temp so that no memory is wasted
    // Now we have pressurenum amount of values with HEAD being first value and TAIL last
    avg = 0; //Initializes avg
    sum = 0; //Initializes sum
    pressurevalues_t * temp3 = HEAD; //Allocates memory to be manipulated by a pointer temp3, which points to the struct "pressurevalues"
    //presA is defined as temp3, which points to the floating point variable pressure, which is located inside the struct "pressurevalues"
    for (i = 0; i < pressurenum; i++) {//This for loop will find the sum of all the pressures
      presA = temp3->pressure; //DOESNT THIS JUST WRITE THE SAME PRESSURE VALUE TO PRESA AGAIN AND AGAIN? presA is defined as temp3, which points to the floating point variable pressure, which is located inside the struct "pressurevalues"
      sum += presA; //Adds presA to the current sum
      temp3 = temp3->next; //Shifts temp3 to the next value in the struct "pressurevalues"
      //Serial.println(sum);
    }
    avg = sum / pressurenum; //Finds the intial average
    //Serial.println(avg);
    previousMillis = millis();
    while (avg > targ) { //This while loop find the average, compare it with the target pressure. If more than target pressure, it will find a new average by shifting the values to the right. It less, then the loop will exit
      //Serial.println(avg);
      //Serial.println(targ);
      struct pressurevalues * temp = malloc(sizeof(struct pressurevalues)); //Allocates memory to be manipulated by a pointer temp, which points to the struct "pressurevalues"
      TAIL->next = temp;
      pressureReading = analogRead(A3);
      pressureVolt = pressureReading * (5.0 / 1023); //This should use the same pressure reading that was assgined to the pointer "temp2"
      psi = (pressureVolt - 0.5) * (15.0 / 4.0);
      temp->pressure = psi;
      temp->next = NULL;
      TAIL = temp;
      temp = HEAD;
      HEAD = HEAD->next;
      free(temp);
      sum = 0;
      pressurevalues_t * temp3 = HEAD;
      for (i = 0; i < pressurenum; i++) { //Create new struct with n = pressurenum values
        presA = temp3->pressure;
        sum += presA;
        temp3 = temp3->next;
      }
      avg = sum / pressurenum; //Calculate the new average

      currentMillis = millis();
      if ((currentMillis - previousMillis) >= LOG_INTERVAL) {
        dataString = "";

        timeStamp = millis();
        dataString = String(timeStamp);
        Serial.print(timeStamp);

        pressureReading = analogRead(A3);
        pressureVolt = pressureReading * (5.0 / 1023); //This should use the same pressure reading that was assgined to the pointer "temp2"
        psi = (pressureVolt - 0.5) * (15.0 / 4.0);
        Serial.print(','); //Not sure what this does
        //Serial.print("\t PSI ");
        Serial.print(psi, 2);
        dtostrf(psi, 6, 2, psiString);              // Convert psi into a String
        dataString = dataString + "," + psiString;  // Add to the dataString

        fluoroVolt = 0;
        Serial.print(',');
        //Serial.print("\t V ");
        Serial.print(fluoroVolt, 2);
        dtostrf(fluoroVolt, 6, 2, fluoroVString);              // Convert voltage into a String
        dataString = dataString + "," + fluoroVString;  // Add to the dataString */

        Serial.println();
        writeDataToSD();

        previousMillis = millis();
      }
    }//At this point, we've reached target pressure, it's time to start taking fluorometer measurements

    i = 0; //allows us to take 60 fluorometer readings
    c = 0; //prevents arduino from opening cap twice
    p = 0; //prevents arduino from closing cap twice
    a = 0; //allows us to reset the time

    //Initial calibration of cap and fluorometer
    previousMillis = millis();
    currentMillis = millis();
    currentMax = analogRead(LIGHT_PIN);
    myservo.write(sencals);
    while ((currentMillis - previousMillis) <= turnint) {
      newReading = analogRead(LIGHT_PIN);
      //Serial.println(newReading);
      if (newReading > currentMax) {
        currentMax = newReading;
      }
      currentMillis = millis();
    }
    myservo.write(94);
    Serial.println(currentMax);
    previousMillis = millis();
    currentMillis = millis();
    currentMin = analogRead(LIGHT_PIN); //Cap Closing
    myservo.write(sencals);
    while ((currentMillis - previousMillis) <= turnint) {
      newReading = analogRead(LIGHT_PIN);
      //Serial.println(newReading);
      if (newReading < currentMin) {
        currentMin = newReading;
      }
      currentMillis = millis();
    }
    myservo.write(94);
    Serial.println(currentMin);
    //End of intial calibration
    previousMillis = millis();
    previousMillis1 = millis();

    i = 0;

    while (i < 59) { //HOW MANY TESTS DO WE WANT TO TAKE?
     /* //Recalibrates the flurometer based off of max cloud altitude range
      pressureReading = analogRead(LIGHT_PIN);
      pressureVolt = pressureReading * (5.0 / 1023);
      psi = (pressureVolt - 0.5) * (15.0 / 4.0);
      if (psi < cloud1 && c1 == 0) { //Passed Altostratus Clouds
        previousMillis = millis();
        currentMillis = millis();
        currentMax = analogRead(LIGHT_PIN);
        myservo.write(sencals);
        while ((currentMillis - previousMillis) <= turnint) {
          newReading = analogRead(LIGHT_PIN);
          if (newReading > currentMax) {
            currentMax = newReading;
          }
          currentMillis = millis();
        }
        myservo.write(94);
        previousMillis = millis();
        currentMillis = millis();
        currentMin = analogRead(LIGHT_PIN);
        myservo.write(sencals);
        while ((currentMillis - previousMillis) <= turnint) {
          newReading = analogRead(LIGHT_PIN);
          if (newReading < currentMin) {
            currentMin = newReading;
          }
          currentMillis = millis();
        }
        myservo.write(94);
        c1 = 1;
        a = 0;
        c = 0;
        p = 0;
        previousMillis = millis(); //Resets time and capstate
      }

      if (psi < cloud2 && c2 == 0) { //Passed Nimbostratus and Altocumulus Clouds
        previousMillis = millis();
        currentMillis = millis();
        currentMax = analogRead(LIGHT_PIN);
        myservo.write(sencals);
        while ((currentMillis - previousMillis) <= turnint) {
          newReading = analogRead(LIGHT_PIN);
          if (newReading > currentMax) {
            currentMax = newReading;
          }
          currentMillis = millis();
        }
        myservo.write(94);
        previousMillis = millis();
        currentMillis = millis();
        currentMin = analogRead(LIGHT_PIN); //Cap Closing
        myservo.write(sencals);
        while ((currentMillis - previousMillis) <= turnint) {
          newReading = analogRead(LIGHT_PIN);
          if (newReading < currentMin) {
            currentMin = newReading;
          }
          currentMillis = millis();
        }
        myservo.write(94);
        c2 = 1;
        a = 0;
        c = 0;
        p = 0;
        previousMillis = millis();
      }

      if (psi < cloud3 && c3 == 0) { //Passed Cirrostratus Clouds
        previousMillis = millis();
        currentMillis = millis();
        currentMax = analogRead(LIGHT_PIN);
        myservo.write(sencals);
        while ((currentMillis - previousMillis) <= turnint) {
          newReading = analogRead(LIGHT_PIN);
          if (newReading > currentMax) {
            currentMax = newReading;
          }
          currentMillis = millis();
        }
        myservo.write(94);
        previousMillis = millis();
        currentMillis = millis();
        currentMin = analogRead(LIGHT_PIN); //Cap Closing
        myservo.write(sencals);
        while ((currentMillis - previousMillis) <= turnint) {
          newReading = analogRead(LIGHT_PIN);
          if (newReading < currentMin) {
            currentMin = newReading;
          }
          currentMillis = millis();
        }
        myservo.write(94);
        c3 = 1;
        a = 0;
        c = 0;
        p = 0;
        previousMillis = millis();
      }

      if (psi < cloud4 && c4 == 0) { //Passed Cirrus, Cirocumulus, and Cumulonimbus Clouds
        previousMillis = millis();
        currentMillis = millis();
        currentMax = analogRead(LIGHT_PIN);
        myservo.write(sencals);
        while ((currentMillis - previousMillis) <= turnint) {
          newReading = analogRead(LIGHT_PIN);
          if (newReading > currentMax) {
            currentMax = newReading;
          }
          currentMillis = millis();
        }
        myservo.write(94);
        previousMillis = millis();
        currentMillis = millis();
        currentMin = analogRead(LIGHT_PIN); //Cap Closing
        myservo.write(sencals);
        while ((currentMillis - previousMillis) <= turnint) {
          newReading = analogRead(LIGHT_PIN);
          if (newReading < currentMin) {
            currentMin = newReading;
          }
          currentMillis = millis();
        }
        myservo.write(94);
        c4 = 1;
        a = 0;
        c = 0;
        p = 0;
        previousMillis = millis();
      }
      // End of recalibration*/

      currentMillis = millis();

      if ((currentMillis - previousMillis1) >= LOG_INTERVAL) {
        dataString = "",

        timeStamp = millis(); //Log the time
        dataString = String(timeStamp);

        pressureReading = analogRead(A3); //Log the pressure
        pressureVolt = pressureReading * (5.0 / 1023);
        psi = (pressureVolt - 0.5) * (15.0 / 4.0);
        dtostrf(psi, 6, 2, psiString);              // Convert psi into a String
        dataString = dataString + "," + psiString;  // Add to the dataString
        previousMillis1 = millis();
      }

      if (a == 1) { //The resets the time after a fluorometer reading has been taken
        previousMillis = millis();
        a = 0; //Allows arduino to reset the time after the fluorometer has been measured
      }

      if (c == 0) { //If c == 0, then that means we haven't opened the cap before
        myservo.write(servs);
        Serial.println("Servo ON");
        while (co == 0) {
          fluoroVolt = analogRead(LIGHT_PIN);
          //Serial.println(fluoroVolt);
          if (fluoroVolt <  (currentMax + fc) && fluoroVolt > (currentMax - fc)) {
            myservo.write(94);
            Serial.println(fluoroVolt);
            Serial.println("Servo STOPPED");
            co = 1;
          }
        }
        Serial.println("Cap Open");
        c = 1; //Lets arduino know not to open it again
      }

      currentMillis = millis();
      if ((currentMillis - previousMillis) >= ccinterval) { //If p == 0, then that means that the cap has never been closed before
        myservo.write(servs);
        Serial.println("Servo ON");
        while (cc == 0) {
          fluoroVolt = analogRead(LIGHT_PIN);
          //Serial.println(fluoroVolt);
          if (fluoroVolt <  (currentMin + fc) && fluoroVolt > (currentMin - fc)) {
            myservo.write(94);
            Serial.println(fluoroVolt);
            Serial.println("Servo STOPPED");
            cc = 1;
          }
        }
        Serial.println("Cap Closed");
        currentMillis = millis();
        previousMillis1 = millis();
        //Serial.println(previousMillis1);
        while ((currentMillis - previousMillis1) <= LOG_INTERVAL) {
          currentMillis = millis();
        }
        fluoroVolt = analogRead(A4);
        //Serial.print(',');
        //Serial.print("\t V ");
        //Serial.println(fluoroVolt, 2);
        dtostrf(fluoroVolt, 6, 2, fluoroVString);        // Convert voltage into a String
        dataString = dataString + "," + fluoroVString;   // Add to the dataString
        //Serial.println();
        writeDataToSD();
        Serial.println(dataString);
        dataString = "";
        a = 1;
        c = 0; // Allows arduino to open cap again next cycle
        // p = 0; // Allows arduino to close cap again next cycle
        cc = 0;
        //cc = 2;
        co = 0;
        i++;
      }

      if (a == 0) {
        currentMillis = millis();
        previousMillis1 = millis();
        while ((currentMillis - previousMillis1) <= LOG_INTERVAL) {
          currentMillis = millis();
        }
        fluoroVolt = 0.0;
        dtostrf(fluoroVolt, 6, 2, fluoroVString);              // Convert voltage into a String
        dataString = dataString + "," + fluoroVString;  // Add to the dataString
        Serial.println(dataString);
        writeDataToSD();
        // Allows arduino to open cap again next cycle
        cc = 0;
        co = 0;
        //previousMillis1 = millis();
        dataString = "";
      }
    }
    ttake = 1; //Let's make sure Arduino doesn't retake those measurements again.
    //digitalWrite(8, LOW); //Turn off blue LED to save power???
    myservo.write(servs);
    while (cc == 0) {
      fluoroVolt = analogRead(LIGHT_PIN);
      if (fluoroVolt <  (currentMin + fc) && fluoroVolt > (currentMin - fc)) {
        myservo.write(94);
        cc = 1;
      }
    }
    Serial.println("Cap Closed");
  }

  previousMillis = millis();
  while (1) {
    currentMillis = millis();
    if ((currentMillis - previousMillis) >= LOG_INTERVAL) {
      dataString = "";

      timeStamp = millis();
      dataString = String(timeStamp);
      Serial.print(timeStamp);

      pressureReading = analogRead(A3);
      pressureVolt = pressureReading * (5.0 / 1023);
      psi = (pressureVolt - 0.5) * (15.0 / 4.0);
      Serial.print(','); //Not sure what this does
      //Serial.print("\t PSI ");
      Serial.print(psi, 2);
      dtostrf(psi, 6, 2, psiString);              // Convert psi into a String
      dataString = dataString + "," + psiString;  // Add to the dataString

      fluoroVolt = 0;
      Serial.print(',');
      //Serial.print("\t V ");
      Serial.print(fluoroVolt, 2);
      dtostrf(fluoroVolt, 6, 2, fluoroVString);              // Convert voltage into a String
      dataString = dataString + "," + fluoroVString;  // Add to the dataString

      Serial.println();
      writeDataToSD();

      previousMillis = millis();
    }
  }
}
