#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//const int stepsPerRevolution = 2048*2;
const int stepsPerRevolution = 2048;
//950 is the smallest microsecond delay between steps that would work reliably with the stepper motor.
int usStepperDelay = 1000;
int testSteps = 512;

#define STEPPER_IN1  2
#define STEPPER_IN2  4
#define STEPPER_IN3  3
#define STEPPER_IN4  5

#define LEFT_BUTTON 10
#define RIGHT_BUTTON 9

#define HALL_SENSOR 8

int nextStep = 0;
int position = 0;

bool hallSensorState;
bool newHallSensorState;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.display();
  display.dim(true);
  //display.setFont(ArialMT_Plain_10);
  
  pinMode(STEPPER_IN1, OUTPUT); 
  pinMode(STEPPER_IN2, OUTPUT); 
  pinMode(STEPPER_IN3, OUTPUT); 
  pinMode(STEPPER_IN4, OUTPUT); 

  pinMode(LEFT_BUTTON,INPUT); 
  pinMode(RIGHT_BUTTON,INPUT); 

  hallSensorState = digitalRead(HALL_SENSOR);

  displayPrint("Forwards",2,0);
  stepper(testSteps); 
  delay(2000);
  displayPrint("Backwards",2,0);
  stepper(-testSteps); 
  disengage();
}

void loop() {

  if(digitalRead(LEFT_BUTTON) == LOW){
    displayPrint("L pressed",2,0);
    stepper(testSteps);
    disengage();
  }
  if(digitalRead(RIGHT_BUTTON) == LOW){
    displayPrint("R Pressed",2,0);
    stepper(-testSteps);
    disengage();
  }
  
  newHallSensorState = digitalRead(HALL_SENSOR);
  if(newHallSensorState != hallSensorState){
    if(newHallSensorState == LOW){
      displayPrint("Mag sensed",2,0);
    } else {
      displayPrint("Mag gone",2,0);
    }
    hallSensorState = newHallSensorState;
  }
  
  
}

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

char msg[11];

void stepper(int steps){
  for (int x=0;x<abs(steps);x++){
    if(steps < 0 ) {
      step(false);
    } else {
      step(true);
    }
  }
  sprintf(msg,"Pos %d",position);
  displayPrint(msg,2,1);
}

void step(bool forward){
  if(forward) {
    nextStep++;
    position++;
  } else {
    nextStep--;
    position--;
  }
  if(nextStep>7){nextStep=0;}
  if(nextStep<0){nextStep=7; }

  delayMicroseconds(usStepperDelay);
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
}




char blankLine_size1[21]="                    ";
char blankLine_size2[11]="          ";

void displayPrint(char *message,int size, int line) {
  //Serial.print(message);
  int lineHeight = size * 8;

  char *blankline;
  if(size == 1){
    blankline = blankLine_size1;
  } else {
    blankline = blankLine_size2;
  }
  display.setTextSize(size);

  display.setTextColor(SSD1306_WHITE,0);        // Draw white text in opaque mode
  display.setCursor(0,line * lineHeight);             // Start at top-left corner
  display.print(blankline);
  display.setCursor(0,line * lineHeight);             // Start at top-left corner
  display.print(message);
  display.display();
}
