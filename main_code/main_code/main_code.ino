#include "RTClib.h"
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <Servo.h>

#define ONE_WIRE_BUS 2

SoftwareSerial bluetooth(8, 9);

OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

#define SensorPin 0  // the pH meter Analog output is connected with the Arduino’s Analog

Servo myservo;
Servo myservo2;

const int pump1RelayPin = 3;
const int pump2RelayPin = 4;

// Juan A. Villalpando
// http://kio4.com/appinventor/9A0_Resumen_Bluetooth.htm

RTC_DS3231 rtc;


String palabra = "";
volatile int length = 0;
char incomingData[255];

//current information [food,acid,base]
volatile int inventory_status[3] = { 0, 0, 0 };

//fish in tank [activity,species, number, weigth ]
int fish_status[10][4] = {
  { 0, 0, 0, 0 },
  { 0, 0, 0, 0 },
  { 0, 0, 0, 0 },
  { 0, 0, 0, 0 },
  { 0, 0, 0, 0 },
  { 0, 0, 0, 0 },
  { 0, 0, 0, 0 },
  { 0, 0, 0, 0 },
  { 0, 0, 0, 0 },
  { 0, 0, 0, 0 },
};


//  row = species(tilapia,panguius,rohu,mrigal,catla,def_1,def_2), column = wiegth cateory
int fish_dataset[7][5] = {
  { 6, 4, 3, 2, 2 },
  { 6, 5, 4, 3, 3 },
  { 6, 5, 4, 3, 3 },
  { 6, 5, 4, 3, 3 },
  { 6, 5, 4, 3, 3 },
  { 6, 5, 4, 3, 3 },
  { 9, 7, 5, 3, 2 },
};


int recorded_data[5][3]{
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
};

//variable receiving
String op;
volatile int food;
volatile int acid;
volatile int base;
volatile int species;
volatile int number;
volatile int weight;
volatile int feed1;
volatile int feed2;
volatile int feed3;
volatile int feed4;
volatile int feed5;
volatile int time_now;
volatile float pH;
volatile float temp;
volatile float food_amount;
volatile float food_amount_once = 2;    /////not done
volatile float total_food_amount = 30;  ////not done
volatile int water_change_time = 2;
const unsigned long feedingDuration = 5000;  // 20 seconds in milliseconds
int Low_food = 0;
int error = 0;
int no_fish = 0;
int low_chemical = 0;
int temp_out_of_control = 0;
int pH_out_of_control = 0;


volatile int ind1;
volatile int ind2;
volatile int ind3;
volatile int ind4;
volatile int ind5;
volatile int ind6;
volatile int ind7;

unsigned long int avgValue;  // Store the average value of the sensor feedback
float voltage, pHValue;


void setup() {
  // Reading Rate
  sensors.begin();
  Serial.begin(9600);
  bluetooth.begin(9600);
  // DS3231
  Wire.begin();
  rtc.begin();
  // Set the initial date and time
  rtc.adjust(DateTime(2023, 8, 30, 17, 29, 30));


  pinMode(pump1RelayPin, OUTPUT);
  pinMode(pump2RelayPin, OUTPUT);
  digitalWrite(pump1RelayPin, HIGH);
  digitalWrite(pump2RelayPin, HIGH);
  // Turn off both pumps initially
  myservo.attach(12);
  myservo.write(40);
}


void update_fish_dataset() {
  //species ,feed1,feed2,feed3,feed4,feed5
  int feed[] = { feed1, feed2, feed3, feed4, feed5 };
  Serial.println("here2");
  int col;
  for (col = 0; col < 5; col++) {
    Serial.println(feed[col]);
  }
  for (col = 0; col < 5; col++) {
    fish_dataset[species - 1][col] = feed[col];
  }
}

void update_fish_status() {
  //species,weight,number
  int num = 0;
  int old_fish = 0;
  Serial.println("here2");
  Serial.println(number);

  for (int row = 0; row < 10; row++) {
    if ((fish_status[row][0] == 1) & (fish_status[row][1] == species) & (fish_status[row][3] == weight)) {
      old_fish = 1;
      num = fish_status[row][2] + number;
      Serial.println("here5");
      if (num < 0) {
        no_fish = 1;  ///////////no fish
        Serial.println("no fish func");

        error = 1;
        break;
      } else if (num > 0) {
        fish_status[row][2] = num;
        break;
      } else {
        fish_status[row][0] = 0;
      }
    }
  }
  if (old_fish == 0) {
    Serial.println("here3");
    for (int row = 0; row < 10; row++) {
      Serial.println("here4");
      if (fish_status[row][0] == 0) {
        Serial.println("here4");
        if (number <= 0) {
          //send error code
          break;
        } else {
          Serial.println("here5");
          fish_status[row][0] = 1;
          fish_status[row][1] = species;
          fish_status[row][2] = number;
          fish_status[row][3] = weight;
          break;
        }
      }
    }
  }
}

void weight_level(int w) {
  //weight value
  if (w <= 10) {
    weight = 1;
  } else if (w <= 50) {
    weight = 2;
  } else if (w <= 100) {
    weight = 3;
  } else if (w <= 250) {
    weight = 4;
  } else {
    weight = 5;
  }
}

void update_inventory() {
  if (food != 0) {
    inventory_status[0] = food;
  }
  if (acid != 0) {
    inventory_status[1] = acid;
  }
  if (base != 0) {
    inventory_status[2] = base;
  }
}

void record_pH() {

  for (int i = 0; i < 10; i++)  // Get 10 sample value from the sensor for smoothing the value
  {
    avgValue += analogRead(SensorPin);
    delay(10);
  }
  avgValue /= 10;  // Calculate the average value

  // Convert the average analog reading to voltage
  voltage = avgValue * 5.0 / 1024;

  // Calculate pH value using logarithmic formula
  // Adjust the constants based on your sensor specifications
  pHValue = -5.70 * voltage + 30.74;
  pH = pHValue;
  Serial.println("pH recorded");
  Serial.println(pH, 2);

  delay(900);
}


void record_temperature() {
  float Celsius = 0;
  float Fahrenheit = 0;

  sensors.requestTemperatures();

  Celsius = sensors.getTempCByIndex(0);
  Fahrenheit = sensors.toFahrenheit(Celsius);
  temp = Celsius;
  Serial.println(temp);
  Serial.print(Celsius);
  Serial.print(" C  ");
  Serial.print(Fahrenheit);
  Serial.println(" F");

  delay(100);
}
void change_water2();

int control_temperature() {
  // water_change_time = water_change_time / 2;
  int i = 0;

  while (((temp < 24) || (temp > 33)) && (i < 10)) {
    change_water2();
    delay(10000);
    record_temperature();
    i++;
  }
  water_change_time = water_change_time * 2;
  return i;
}

void add_acid();
void add_base();

int control_pH() {
  int i = 0;
  Serial.println("Controlling pH");
  while (((pH < 6) || (pH > 9)) && (i < 10)) {
    if (pH < 6) {
      add_base();
      inventory_status[2] = inventory_status[2] - 5;
    }
    if (pH > 9) {
      add_acid();
      inventory_status[1] = inventory_status[1] - 5;
    }
    Serial.print("Attempt no:");
    Serial.println(i);
    //delay(60*1000);
    record_pH();
    i++;
  }
  return i;
}

void add_acid() {

  // Rotate the servo 360 degrees
  myservo.write(60);
  delay(1000);
  myservo.write(90);
  delay(5000);
  myservo.write(40);
  delay(60000);
  Serial.println("acid added");
}

void add_base() {
  myservo.write(20);
  delay(1000);
  myservo.write(0);
  delay(5000);
  myservo.write(40);
  Serial.println("base added");
  delay(60000);
}

void save_recorded_data() {
  for (int i = 1; i < 5; i++) {
    for (int j = 0; j < 3; j++) {
      recorded_data[i - 1][j] = recorded_data[i][j];
    }
  }

  recorded_data[4][0] = time_now;
  recorded_data[4][1] = pH;
  recorded_data[4][2] = temp;

  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 3; j++) {
      Serial.print(recorded_data[i][j]);
      Serial.print("   ");
    }
    Serial.println("");
  }
}

int fish_count() {
  int fish_number = 0;
  for (int row = 0; row < 10; row++) {
    if (fish_status[row][0] == 1) {

      fish_number = fish_status[row][2] + fish_number;
    }
  }
  return fish_number;
}



void calculate_food() {
  float food_amount_local = 0;
  float w;
  int w1;
  int species;
  float num;

  for (int row = 0; row < 10; row++) {
    if (fish_status[row][0] == 1) {
      species = fish_status[row][1];
      num = fish_status[row][2];

      if (fish_status[row][3] == 1) {
        w = 5;
        w1 = 1;
      } else if (fish_status[row][3] == 2) {
        w = 30;
        w1 = 2;
      } else if (fish_status[row][3] == 3) {
        w = 75;
        w1 = 3;
      } else if (fish_status[row][3] == 4) {
        w = 180;
        w1 = 4;
      } else if (fish_status[row][3] == 5) {
        w = 300;
        w1 = 5;
      }
      // Serial.println(num);
      // Serial.println(w );
      // Serial.println(species );
      // Serial.println(w1 );
      // Serial.println(( (fish_dataset[species-1][w1-1])*0.01));
      // Serial.println((num * w * (fish_dataset[species-1][w1-1])*0.01));
      food_amount_local = food_amount_local + (num * w * (fish_dataset[species - 1][w1 - 1]) * 0.01);
    }
  }
  food_amount = food_amount_local;
}


void update_food_inventory();

void feed() {
  // myservo2.attach(13);
  // myservo2.write(0);
  int n = ceil(food_amount / food_amount_once);  ////not done
  Serial.println(n);
  unsigned long startTime = millis();
  ;                                      // Variable to store the start time
  unsigned long currentTime = millis();  // Get the current time
  //myservo2.write(0);

  // Run the loop for 20 seconds
  for (int i = 0; i < n; i++) {
    // for (int pos = 0; pos <= 360; pos += 1) {
    //   myservo2.write(pos);
    //   delay(50);
    // }
    myservo2.attach(13);
    //myservo2.write(0);
    //delay(1500);
    //myservo2.detach();
    // delay(1000);
    // myservo2.attach(13);
    myservo2.write(360);
    delay(2000);
    //myservo2.detach();
    //delay(1000);
    // myservo2.attach(13);
    // myservo2.write(180);
    //delay(1500);
    //currentTime = millis(); // Update the current time
    //myservo2.write(0);
    myservo2.detach();
    delay(1000);
  }
  int percentage = ceil(((n * food_amount_once / total_food_amount)) * 100);
  Serial.println(percentage);
  inventory_status[0] = inventory_status[0] - percentage;
  // if (inventory_status[0] <= 20) {
  //   Low_food = 1;
  //   error = 1;
  //   //////////////////////////////////////////////////////////////////////////////////////send error
  // }
  //update_food_inventory();
  // When 20 seconds have elapsed, exit the loop
  //myservo2.write(0); // Set the servo back to its initial position
  // myservo2.detach();
  // delay(2000);
}


// void update_food_inventory() {
//   int percentage = ceil((food_amount_once / total_food_amount) * 100);
//   Serial.println(percentage);
//   inventory_status[0] = inventory_status[0] - percentage;
//   if (inventory_status[0] <= 20) {
//     Low_food = 1;
//     error = 1;
//     //////////////////////////////////////////////////////////////////////////////////////send error
//   }
// }

void send_data() {
  String Time = String(recorded_data[0][0]) + "," + String(recorded_data[1][0]) + "," + String(recorded_data[2][0]) + "," + String(recorded_data[3][0]) + "," + String(recorded_data[4][0]);
  String PH = String(recorded_data[0][1]) + "," + String(recorded_data[1][1]) + "," + String(recorded_data[2][1]) + "," + String(recorded_data[3][1]) + "," + String(recorded_data[4][1]);
  String Temp = String(recorded_data[0][2]) + "," + String(recorded_data[1][2]) + "," + String(recorded_data[2][2]) + "," + String(recorded_data[3][2]) + "," + String(recorded_data[4][2]);

  String Store = String(inventory_status[0]) + "," + String(inventory_status[1]) + "," + String(inventory_status[2]) + ",";

  int fish = fish_count();

  String Send = ";" + Time + ";" + PH + ";" + Temp + ";" + Store + ";" + String(fish) + ";";
  if (error == 0) {
    Serial.println(Send);
    bluetooth.println(Send);
    delay(1000);
  } else {
    if (Low_food == 1) {
      bluetooth.println("Low food;");
      Serial.println("Low food;");
      Low_food = 0;
      error = 0;
      delay(5000);
    }
    if (no_fish == 1) {
      bluetooth.println("Fish remove failed;");
      Serial.println("Fish remove failed;");
      no_fish = 0;
      error = 0;
      delay(5000);
    }
    if (low_chemical == 1) {
      bluetooth.println("Low chemical;");

      low_chemical = 0;
      error = 0;
      delay(5000);
    }
    if (temp_out_of_control == 1) {
      bluetooth.println("Can't control temp;");
      temp_out_of_control = 0;
      error = 0;
      delay(5000);
    }

    if (pH_out_of_control == 1) {
      bluetooth.println("Can't control pH;");
      pH_out_of_control = 0;
      error = 0;
      delay(5000);
    }
  }
}



void change_water() {
  Serial.println("Changing water");
  for (int cycle = 0; cycle < 3; cycle++) {
    // Turn on pump 1

    digitalWrite(pump1RelayPin, LOW);
    digitalWrite(pump2RelayPin, HIGH);
    delay(60000);  // Delay for 2 minutes

    // Turn off pump 1
    digitalWrite(pump1RelayPin, HIGH);

    // Turn on pump 2
    digitalWrite(pump2RelayPin, LOW);
    delay(60000);  // Delay for 2 minutes



    // Turn off pump 2

    //digitalWrite(pump2RelayPin, HIGH);

    //delay(5 * 60 * 1000); // Delay for 5 minutes before the next cycle
  }

  digitalWrite(pump1RelayPin, HIGH);

  digitalWrite(pump2RelayPin, HIGH);
}

void change_water2() {
  for (int cycle = 0; cycle < 1; cycle++) {
    // Turn on pump 1
    digitalWrite(pump1RelayPin, LOW);
    digitalWrite(pump2RelayPin, HIGH);
    delay(30000);  // Delay for 2 minutes

    // Turn off pump 1
    digitalWrite(pump1RelayPin, HIGH);

    // Turn on pump 2
    digitalWrite(pump2RelayPin, LOW);
    delay(30000);  // Delay for 2 minutes



    // Turn off pump 2

    //digitalWrite(pump2RelayPin, HIGH);

    //delay(5 * 60 * 1000); // Delay for 5 minutes before the next cycle
  }

  digitalWrite(pump1RelayPin, HIGH);

  digitalWrite(pump2RelayPin, HIGH);
}

void loop() {


  // reading string
  //  Serial.println("dwdad");
  if (bluetooth.available() > 0) {

    char caracter = bluetooth.read();
    //   palabra = palabra + caracter;
    // Serial.println(caracter);
    if (caracter == '*') {
      Serial.println("here");
      Serial.println("received data" + (String(incomingData)));
      palabra = "";
      palabra = (String(incomingData));
      for (int i = 0; i < 255; i++) {
        incomingData[i] = NULL;
      }

      Serial.println(palabra);
      length = 0;
      ind1 = palabra.indexOf(',');
      op = palabra.substring(0, ind1);
      Serial.println(op);

      if (op == "1") {
        ind2 = palabra.indexOf(',', ind1 + 1);
        food = (palabra.substring(ind1 + 1, ind2)).toInt();
        ind3 = palabra.indexOf(',', ind2 + 1);
        acid = (palabra.substring(ind2 + 1, ind3)).toInt();
        ind4 = palabra.indexOf(',', ind3 + 1);
        base = (palabra.substring(ind3 + 1)).toInt();
        update_inventory();  ///not done
        for (int c = 0; c < 3; c++) {
          Serial.print(inventory_status[c]);
          Serial.print("    ");
        }
      }

      if (op == "21") {
        ind2 = palabra.indexOf(',', ind1 + 1);
        species = (palabra.substring(ind1 + 1, ind2)).toInt();
        ind3 = palabra.indexOf(',', ind2 + 1);
        Serial.println("here");
        number = (palabra.substring(ind2 + 1, ind3)).toInt();
        ind4 = palabra.indexOf(',', ind3 + 1);
        weight_level((palabra.substring(ind3 + 1)).toInt());
        update_fish_status();
        for (int r = 0; r < 10; r++) {
          for (int c = 0; c < 4; c++) {
            Serial.print(fish_status[r][c]);
            Serial.print("\t");
          }
          Serial.println("");
        }
        /// done
      }

      if (op == "22") {
        ind2 = palabra.indexOf(',', ind1 + 1);
        species = (palabra.substring(ind1 + 1, ind2)).toInt();
        ind3 = palabra.indexOf(',', ind2 + 1);
        number = ((palabra.substring(ind2 + 1, ind3)).toInt()) * (-1);
        ind4 = palabra.indexOf(',', ind3 + 1);
        weight_level((palabra.substring(ind3 + 1)).toInt());
        update_fish_status();

        for (int r = 0; r < 10; r++) {
          for (int c = 0; c < 4; c++) {
            Serial.print(fish_status[r][c]);
            Serial.print("\t");
          }
          Serial.println("");  /// done
        }
      }

      if (op == "3") {
        ind2 = palabra.indexOf(',', ind1 + 1);
        species = (palabra.substring(ind1 + 1, ind2).toInt());
        Serial.println(species);
        Serial.println("here");
        ind3 = palabra.indexOf(',', ind2 + 1);
        feed1 = (palabra.substring(ind2 + 1, ind3).toInt());
        Serial.println(feed1);
        ind4 = palabra.indexOf(',', ind3 + 1);
        feed2 = (palabra.substring(ind3 + 1, ind4)).toInt();
        ind5 = palabra.indexOf(',', ind4 + 1);
        feed3 = (palabra.substring(ind4 + 1, ind5)).toInt();
        ind6 = palabra.indexOf(',', ind5 + 1);
        feed4 = (palabra.substring(ind5 + 1, ind6)).toInt();
        ind7 = palabra.indexOf(',', ind6 + 1);
        feed5 = (palabra.substring(ind6 + 1)).toInt();
        update_fish_dataset();  /// done

        for (int i = 0; i < 7; i++) {
          for (int j = 0; j < 5; j++) {
            Serial.print(fish_dataset[i][j]);
            Serial.print("\t");  // Add a tab between matrix elements
          }
          Serial.println();  // Move to the next row
        }
      }

    } else if (caracter != NULL) {
      Serial.println("in");
      Serial.println(caracter);
      incomingData[length] = caracter;
      length++;
      Serial.println(String(incomingData));
      Serial.println(length);
      Serial.println("out");
    }
  }



  DateTime now = rtc.now();
  calculate_food();
  if ((food_amount >= (total_food_amount * (inventory_status[0])))) {
    // Serial.println("Low food");
    //bluetooth.println("Low food;");
    Low_food = 1;
    error = 1;
    //////////////////////////////////////////                                      send error
  }

  if ((inventory_status[1] < 15) || ((inventory_status[2] < 15))) {
    // Serial.println("Low chemical");
    low_chemical = 1;
    error = 1;
  }
  if (error && (now.second() == 30 || now.minute() == 0)) {
    if (Low_food) {
      Serial.println("Low food");
      send_data();
    }

    if (no_fish) {
      Serial.println("No Fish");
      send_data();
    }
    if (low_chemical) {
      Serial.println("Low chemical");
      send_data();
    }
  }

  // if ((now.hour() == 9) || (now.hour() == 14) || (now.hour() == 20))) {
  if (((now.minute() == 31) || (now.minute() == 33)) && (now.second() == 0)) {
    calculate_food();
    Serial.print("Food amount: ");
    Serial.println(food_amount);
    if ((food_amount <= (total_food_amount * (inventory_status[0])))) {
      Serial.println("feeding");
      feed();

    } else {
      Serial.println("Low food");
      //bluetooth.println("Low food;");
      Low_food = 1;
      error = 1;
      //////////////////////////////////////////                                      send error
    }
    Serial.print("Inventory: ");
    for (int c = 0; c < 3; c++) {
      Serial.print(inventory_status[c]);
      Serial.print("    ");
    }
    Serial.println();
  }
  if ((now.hour() == 18 && now.minute() == 29 && now.second() == 45)) {
    change_water();  //not done
  }

  if (now.minute() == 0 || now.second() == 0) {
    // Serial.println("now time");
    time_now = now.minute();
    Serial.println(time_now);
    record_pH();
    if ((pH < 6) || (pH > 9)) {
      if ((inventory_status[1] > 15) && ((inventory_status[2] > 15))) {
        int pH_control_attempts = control_pH();

        if (pH_control_attempts >= 10) {
          Serial.println("Out of control");
          pH_out_of_control = 1;
          error = 1;  /////////////////////////////////////////////////////////// send error
        }
      }
    }
    record_temperature();

    if ((temp < 24) || (temp > 34)) {
      int temp_control_attempts = control_temperature();

      if (temp_control_attempts >= 10) {
        temp_out_of_control = 1;
        error = 1;
        /////////////////////////////////////////////////////////// send error
      }
    }

    save_recorded_data();
    send_data();
  }
}
