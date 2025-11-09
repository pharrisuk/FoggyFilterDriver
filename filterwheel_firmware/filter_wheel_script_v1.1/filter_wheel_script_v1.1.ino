#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#include <Adafruit_GFX.h>
#include <Fonts/FreeMono9pt7b.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//To output debug messages to Serial port
//this needs to be set to false for the ascom
//link to work
bool debug = false;

//const int stepsPerRevolution = 2048*2;
//const int stepsPerRevolution = 2048;
//950 is the smallest microsecond delay between steps that would work reliably with the stepper motor.
int usStepperDelay = 1100;

#define STEPPER_IN1  2
#define STEPPER_IN2  4
#define STEPPER_IN3  3 
#define STEPPER_IN4  5

#define LEFT_BUTTON 10
#define RIGHT_BUTTON 9

#define HALL_SENSOR 8

#define CLOCKWISE true
#define ANTI_CLOCKWISE false

int nextStep = 0;
int position = 0;

/*Set high to max at start, will be re-set by calibration*/
int stepsPerRevolution = 32767;

/*
* FILTER1_POS is the number of steps (or more accurately, position increments) 
* clockwise to get to first filter from the magnet (home position) and is needed as a starting
* point for calibration. The other filter positions will be calculated.
*/
#define FILTER1_POS 1100
int filter_pos[5];
int currentFilter = -1;
char filterNames[5][8] = {"Filter0", "Filter1", "Filter2", "Filter3", "Filter4"};

bool hallSensorState;
bool newHallSensorState;
bool calibrated = false;

char msg[121];
char smsg[40];

String cmd;
//bool calibrated= false;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    if(debug)Serial.println(F("Debug: SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.display();
  display.dim(true);
  display.setRotation(2);
  display.setFont(&FreeMono9pt7b);
  //display.setFont(ArialMT_Plain_10);
  
  pinMode(STEPPER_IN1, OUTPUT); 
  pinMode(STEPPER_IN2, OUTPUT); 
  pinMode(STEPPER_IN3, OUTPUT); 
  pinMode(STEPPER_IN4, OUTPUT); 

  pinMode(LEFT_BUTTON,INPUT); 
  pinMode(RIGHT_BUTTON,INPUT); 

  hallSensorState = digitalRead(HALL_SENSOR);

  displayPrint("Calibrate",1);
  displayPrint("or Connect",2); 
  
  /*
  delay(2000);
  gotoFilter(2);
  delay(2000);
  gotoFilter(1);
  delay(2000);
  gotoFilter(5);
  delay(2000);
  gotoFilter(3);
  delay(2000);
  gotoFilter(5);
  */

}

void loop() {
  if(Serial.available() >0 ){
    cmd = Serial.readStringUntil('#');
    if(cmd=="GETFILTER"){
        sprintf(msg,"FILTER:%i#",currentFilter);
        Serial.print(msg);
    } 
    else if(cmd =="FILTER0") gotoFilter(0);
    else if(cmd =="FILTER1") gotoFilter(1);
    else if(cmd =="FILTER2") gotoFilter(2);
    else if(cmd =="FILTER3") gotoFilter(3);
    else if(cmd =="FILTER4") gotoFilter(4);
    else if(cmd.startsWith("CONNECT")){
      ascomConnect(cmd);
    }
    else if(cmd =="CALIBRATE") {
      if(!calibrated) calibrate();
    }
    Serial.print("OK#");
  }

  if(digitalRead(LEFT_BUTTON) == LOW){
    if(!calibrated)calibrate();
    else nudgeFilter(ANTI_CLOCKWISE);
  }

  if(digitalRead(RIGHT_BUTTON) == LOW){
    if(!calibrated)calibrate();
    else nudgeFilter(CLOCKWISE);
  }

  //sprintf(msg,"Pos %d",position);
  //displayPrint(msg,1);
  disengage();
}

void ascomConnect(String connectString){
  connectString.remove(0,8); //Remove "CONNECT,"
  char temp[121];
  if(debug){
    connectString.toCharArray(temp, 120);
    sprintf(msg,"connectString: %s",temp);
    Serial.println(msg);
  }
  int indexOfComma = 0;
  for(int i=0;i<5;i++){
    indexOfComma = connectString.indexOf(",");
    if(indexOfComma > 0){
      //filterNames[i] = connectString.substring(0,indexOfComma);
      connectString.substring(0,indexOfComma).toCharArray(filterNames[i], 9);
      connectString.remove(0,indexOfComma+1);
    } else {
      connectString.toCharArray(filterNames[i], 9);
      //filterNames[i] = connectString;
    }
      sprintf(smsg,"FName %i %s",i,filterNames[i]);
      if(debug)Serial.println(smsg);
  }
  gotoFilter(currentFilter);
}


void stepper(int steps, bool clockwise){
  for (int x=0;x<steps;x++){
    if(clockwise) {
      step(CLOCKWISE);
    } else {
      step(ANTI_CLOCKWISE);
    }
  }
}

int nudgeFilter(bool clockwise){
  int targetFilter;
  if(clockwise){
    targetFilter = currentFilter+1;
    if(targetFilter == 5){targetFilter=0;}
  } else {
    targetFilter = currentFilter-1;
    if(targetFilter == -1){targetFilter=4;}
  }
  gotoFilter(targetFilter);
  return targetFilter;
}


/* Debug version of gotoFilter - doesn't actually move between filters
void gotoFilter2(int filter){
  sprintf(smsg,"F%i --> F%i",currentFilter,filter);
  if(debug)Serial.println(smsg);
  sprintf(msg,"%s ->",filterNames[currentFilter]);
  displayPrint(msg,1);
  sprintf(msg,"%s",filterNames[filter]);
  displayPrint(msg,2);

  delay(4000);
  currentFilter = filter;

  displayPrint("Parked:",1);
  sprintf(msg,"%s(%i)",filterNames[currentFilter],currentFilter);
  displayPrint(msg,2);
  sprintf(smsg,"FILTER:%i#",currentFilter);
  Serial.print(smsg);
}
*/

void gotoFilter(int filter){
  int clockwiseDistance =0;
  int anticlockwiseDistance = 0;

  sprintf(smsg,"F%i --> F%i",currentFilter,filter);
  if(debug)Serial.println(smsg);
  if(currentFilter == -1) sprintf(msg,"->");
  else sprintf(msg,"%s ->",filterNames[currentFilter]);
  displayPrint(msg,1);
  sprintf(msg,"%s",filterNames[filter]);
  displayPrint(msg,2);

  if(currentFilter != filter){
    clockwiseDistance = distanceCorrector(filter_pos[filter] - position);
    anticlockwiseDistance = distanceCorrector(stepsPerRevolution - (filter_pos[filter] - position));

    if(clockwiseDistance < anticlockwiseDistance){
      if (filter_pos[filter] > position) {
        stepper(filter_pos[filter] - position,CLOCKWISE);
      } else {
        /*As we're going past home, take the opportunity to re-calibrate*/
        findHome(CLOCKWISE);
        stepper(filter_pos[filter],CLOCKWISE);
      }
    } else {
      if (filter_pos[filter] < position) {
        stepper(position - filter_pos[filter],ANTI_CLOCKWISE);
      } else {
        /*As we're going past home, take the opportunity to re-calibrate*/
        findHome(ANTI_CLOCKWISE);
        stepper(stepsPerRevolution - filter_pos[filter],ANTI_CLOCKWISE);
      }
    }
    currentFilter = filter;
  }
  displayPrint("Parked:",1);
  sprintf(msg,"%s(%i)",filterNames[currentFilter],currentFilter);
  displayPrint(msg,2);
  sprintf(smsg,"FILTER:%i#",currentFilter);
  Serial.print(smsg);
}

int distanceCorrector(int distance){
  if(distance >= stepsPerRevolution){
    return (distance - stepsPerRevolution);
  } else {
    if(distance < 0) {
       return (distance + stepsPerRevolution);
    } else {
       return distance;
    }
  }
}

void calibrate(){
  displayPrint("Finding",1);  
  displayPrint("Magnet",2);
  findHome(CLOCKWISE);
  //displayPrint("Pos 0",1);
  displayPrint("Calculating",1);
  displayPrint("Steps/rev.",2);
  stepsPerRevolution = findHome(CLOCKWISE);
  
  //Comment out the call above and use the line below instead for
  //quicker testing (doesn't rotate the wheel to count the steps)
  //stepsPerRevolution=19996;
  /*now insert the filter positions into the filter_pos array*/
  int steps_between_filters = int(stepsPerRevolution / 5);
  for(int i=0;i<5;i++){
    filter_pos[i] = FILTER1_POS + (steps_between_filters * i);
  }
  //Goto filter 4 as this is the best place to
  //re-calibrate from should we be conneced to from ascom which will
  //cause a reset on opening the com port
  gotoFilter(4);

  //sprintf(msg,"Poss %d",position);
  //displayPrint(msg,1,1);
  disengage();
  calibrated = true;
}

//Find the right side of the magnet
int findHome(bool clockwise){
  int stepCount=0;

  /*if we're not already at the magnet, find it  */
  while(hallSensorState == HIGH){
    if(clockwise) {
      step(CLOCKWISE);
    } else {
      step(ANTI_CLOCKWISE);
    }
    stepCount++;
    hallSensorState = digitalRead(HALL_SENSOR);
  }
 
  /*traverse the magnet*/
  while (hallSensorState == LOW) {
    step(CLOCKWISE);
    stepCount++;
    hallSensorState = digitalRead(HALL_SENSOR);
  }
  stepCount = position;
  
  position = 0;
  return stepCount;
}

//Between using the stepper motor we call this
//to stop any current flowing through the coils
//This stops the stepper motor getting warm
void disengage(){
  digitalWrite(STEPPER_IN1, LOW); 
  digitalWrite(STEPPER_IN2, LOW);
  digitalWrite(STEPPER_IN3, LOW);
  digitalWrite(STEPPER_IN4, LOW);
}

void stepper_init(){
  digitalWrite(STEPPER_IN1, LOW); 
  digitalWrite(STEPPER_IN2, LOW);
  digitalWrite(STEPPER_IN3, LOW);
  digitalWrite(STEPPER_IN4, HIGH);
}

/*
* One step clockwise or anticlockwise where the direction referes to the filter wheel 
* (so the steppr motor will travelling in teh opposite direction)
*/
void step(bool clockwise){
  if(clockwise) {
    nextStep--;
    position++;
    if(position > stepsPerRevolution){position=0;}
  } else {
    nextStep++;
    position--;
    if(position < 0){position=stepsPerRevolution;}
  }
  if(nextStep>7){nextStep=0;}
  if(nextStep<0){nextStep=7; }

  switch(nextStep){
    case 0:
      digitalWrite(STEPPER_IN1, LOW); 
      digitalWrite(STEPPER_IN2, LOW);
      digitalWrite(STEPPER_IN3, LOW);
      digitalWrite(STEPPER_IN4, HIGH);
      break; 
    case 1:
      digitalWrite(STEPPER_IN1, LOW); 
      digitalWrite(STEPPER_IN2, LOW);
      digitalWrite(STEPPER_IN3, HIGH);
      digitalWrite(STEPPER_IN4, HIGH);
      break; 
    case 2:
      digitalWrite(STEPPER_IN1, LOW); 
      digitalWrite(STEPPER_IN2, LOW);
      digitalWrite(STEPPER_IN3, HIGH);
      digitalWrite(STEPPER_IN4, LOW);
      break; 
    case 3:
      digitalWrite(STEPPER_IN1, LOW); 
      digitalWrite(STEPPER_IN2, HIGH);
      digitalWrite(STEPPER_IN3, HIGH);
      digitalWrite(STEPPER_IN4, LOW);
      break; 
    case 4:
      digitalWrite(STEPPER_IN1, LOW); 
      digitalWrite(STEPPER_IN2, HIGH);
      digitalWrite(STEPPER_IN3, LOW);
      digitalWrite(STEPPER_IN4, LOW);
      break; 
    case 5:
      digitalWrite(STEPPER_IN1, HIGH); 
      digitalWrite(STEPPER_IN2, HIGH);
      digitalWrite(STEPPER_IN3, LOW);
      digitalWrite(STEPPER_IN4, LOW);
      break; 
    case 6:
      digitalWrite(STEPPER_IN1, HIGH); 
      digitalWrite(STEPPER_IN2, LOW);
      digitalWrite(STEPPER_IN3, LOW);
      digitalWrite(STEPPER_IN4, LOW);
      break; 
    case 7:
      digitalWrite(STEPPER_IN1, HIGH); 
      digitalWrite(STEPPER_IN2, LOW);
      digitalWrite(STEPPER_IN3, LOW);
      digitalWrite(STEPPER_IN4, HIGH);
      break; 
    default:
      digitalWrite(STEPPER_IN1, LOW); 
      digitalWrite(STEPPER_IN2, LOW);
      digitalWrite(STEPPER_IN3, LOW);
      digitalWrite(STEPPER_IN4, LOW);
      break; 
  }
  delayMicroseconds(usStepperDelay);
}

void displayPrint(char *message, int line) {
  //Serial.println(message);
  int lineHeight =10;
  int lineGap = 8;

  display.setTextColor(SSD1306_WHITE,0);        // Draw white text in opaque mode
  display.setCursor(0,(line * lineHeight) + (line-1) * lineGap);             // Start at top-left corner
  display.fillRect(0,(16*(line-1)),156,16,0);  //blank out line

  display.print(message);
  display.display();
}