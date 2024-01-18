//Syringe Project!

// include the library code:
#include <LiquidCrystal.h>
#include <math.h>
#include <AccelStepper.h>
#include <avr/wdt.h>

//setting up the LCD
#define RS_PIN 3
#define EN_PIN 13
#define D4_PIN 4
#define D5_PIN 5
#define D6_PIN 6
#define D7_PIN 7
//init. the lcd
LiquidCrystal lcd(RS_PIN, EN_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN);

//Rotary encoder pins
#define CLICK_PIN 10
#define DT_PIN 11
#define CLK_PIN 12

//button pins
#define BUTTON_CW_PIN A0
#define BUTTON_CCW_PIN A1
#define LIMIT_PIN 2  //external interrupt limit switch

//stepper pins
#define DIRECTION_PIN 8
#define STEP_PIN 9

//motor and syringe values
#define SPR 1600 //steps per revolution its at eighth step rn
#define PITCH 0.8 //cm per revolution of lead screw
#define SYRINGE_SIZE 100 //the size of the syringe, 100ml or 200ml
#define ML_100_RADIUS 0.730 //the radius of the 100 ml syringe in cm .705
#define ML_200_RADIUS 0.950 //the radius of the 200 ml syringe in cm

//The toggle switch, just another way to do a constant extrude because stupid criteria I have to hit
#define toggleSwitch A2


//the RGB led pins, they use the analog pins
//LED that is green when running, yellow when paused, and red when out of liquid
#define redLED A3
#define greenLED A4
#define blueLED A5
#define green 1
#define yellow 2
#define red 3
#define off 4


//setting up the accelstepper functions
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIRECTION_PIN);
//setting up timing for jog function
unsigned long previousApproveMicros = 0;
unsigned long previousCommitMicros = 0;

//rotary encoder
int counter = 0; //the number for the encoder, one click CW +1, CCW -1
int currentStateCLK;
int lastStateCLK;
bool currentDir; //direction encoder is moving 1 is CW, 2 is CCW
unsigned long lastButtonPress = 0; //timer for detecting encoder press

//The display stage of the screen, screen 1,2,3,4 etc it helps keep track
int displayScreen = 1;
//If the display has the current information on it, to prevent constaint refreshs
//1 means the shit is on the screen 0 means that it can be refreshed
bool displayUpdated = 0;
//The amount of steps the motor has done since turning on, positive or negative
long stepperCount = 0;
//The amount of steps the motor has done from the orgin, positive or negative
long stepperCountOrgin = 0;
//used to temp store the value of the knob here for selection screens
//its used to compare the last value to the current knob.count()
//if knob.count() is different, you know that the knob was turned
int knobCounter = 0;

//How many ml's to extrude.  used in screens 3 an 4 I think lol
float mlToExtrude = 0;



//function to easily change color of the LED
//green
//yellow
//red
//off
void statusLED(int color){
  switch (color){
    //green
    case 1:
      digitalWrite(redLED, LOW);
      digitalWrite(greenLED, HIGH);
      digitalWrite(blueLED, LOW);
      break;
    case 2:
      digitalWrite(redLED, HIGH);
      digitalWrite(greenLED, HIGH);
      digitalWrite(blueLED, LOW);
      break;
    case 3:
      digitalWrite(redLED, HIGH);
      digitalWrite(greenLED, LOW);
      digitalWrite(blueLED, LOW);
      break;
    default:
      digitalWrite(redLED, LOW);
      digitalWrite(greenLED, LOW);
      digitalWrite(blueLED, LOW);
      break;
  }
}


//moves the motor a specific number of steps at a specific speed
//similar to jog
//USE: input motor direction true = clockwise, false = counter
//steps, just the number of steps needed
//speed in cm/s
void stepMotor(bool motorDirection, int steps, float speed, bool limitp){
  digitalWrite(DIRECTION_PIN, motorDirection); //immedietly sets the direction of the motor
  bool stepperStep; //should the stepper move?
  int stpCm = static_cast<int>(SPR/PITCH); //calculates step per cm 
  int uspStp = static_cast<int>(1000000 / (stpCm * speed)); //calculates microseconds per step

  //repeats until all the steps have been made
  while(steps > 0){

    //turns the green LED on for MOVING or or the limit switch, bro idk
    if (limitp == 1){
      statusLED(red);
    }
    else{statusLED(green);}

    //turns stepperStep true after every uspStp interval
    if ((micros() - previousApproveMicros) >= uspStp){
      stepperStep = true;
      steps --;
      previousApproveMicros = micros();
    }

    //turns the step pin low once 100 us has passes
    if (micros() - previousCommitMicros >= 50){
      if (stepperStep == false && digitalRead(STEP_PIN) == HIGH){
        digitalWrite(STEP_PIN,LOW);
      }
      else if (stepperStep == true){ //drives the step pin high and signal that the step has been made

        digitalWrite(STEP_PIN, HIGH);
        stepperStep = false;

        //increment the stepper counter
        if (motorDirection == true) {
          stepperCount ++;
        }
        else{
          stepperCount --;
        }
      }
      previousCommitMicros = micros();
      
    }
  }
  //the motor is no longer moving
  statusLED(yellow);
}

//jogs the motor to move things quickly forward or backward
//uses microsecond precision timing
//USE: input (the direction of the motor true = clockwise , false = counterclockwise) , (speed in cm/s)
bool stepperStep; //should the stepper move?
void jog(bool motorDirection, float speed){
  digitalWrite(DIRECTION_PIN, motorDirection); //immedietly sets the direction of the motor
  int stpCm = static_cast<int>(SPR/PITCH); //calculates step per cm 
  int uspStp = static_cast<int>(1000000 / (stpCm * speed)); //calculates microseconds per step
  //turns stepperStep true after every uspStp interval
  Serial.println(uspStp);
  uspStp = 1;
  //sets stepperstep true only if the timing conditions are correct and if the step pin is already low
  if (((micros() - previousApproveMicros) >= uspStp) && digitalRead(STEP_PIN) == LOW ){
    stepperStep = true;
    previousApproveMicros = micros();
  }
  //turns the step pin low once 50 us has passes
  if (micros() - previousCommitMicros >= 50){
    if (stepperStep == false && digitalRead(STEP_PIN) == HIGH){
      digitalWrite(STEP_PIN,LOW);
      //Serial.print("low");
    }
    else if (stepperStep == true){ //drives the step pin high and signal that the step has been made
      digitalWrite(STEP_PIN, HIGH);
      //Serial.print("high");
      stepperStep = false;
      //increment the stepper counter
      if (motorDirection == true) {
        stepperCount ++;
      }
      else{
        stepperCount --;
      }
    }
    previousCommitMicros = micros();
  }
}



//converts the ml requested to the amount of steps for the motor
//size is for size of syringe. 100 or 200
//USE: input (amount of ml you want) , (radius of syringe in cm)
//RETURNS: the amount of steps needed from the motor
int mlToSteps(float ml, float radius){

  float area = 1.0 /(M_PI * pow(radius, 2)); //calculates the area
  int stpCm = static_cast<int>(SPR/PITCH); //calculates step per cm
  int steps = static_cast<int>(area * stpCm * ml); //calculates the steps needed 
  return(steps);
}


//starts a class for the rotaryEncoder, dont know how it fucking works, thank chatGPT 
//USE: returns tons of info about the encoder fuck im tired
//knob.count(); returns a count of how much the knob has been turned
//knob.currentDir(); returns high if CW, low if CCW
//knob.buttonStatus(); returns high if the button is pressed
class RotaryEncoder {
private:
  int counter;
  bool currentDir;
  int lastStateCLK;
  unsigned long lastButtonPress;
    bool buttonStatus;

public:
  // Constructor
  RotaryEncoder() : counter(0), currentDir(false), lastStateCLK(0), lastButtonPress(0), buttonStatus(false) {}

  // Method to read the rotary encoder
  void read() {
    // Read the current state of CLK
    int currentStateCLK = digitalRead(CLK_PIN);

    // If last and current state of CLK are different, then pulse occurred
    // React to only 1 state change to avoid double count
    if (currentStateCLK != lastStateCLK && currentStateCLK == 1) {
      // If the DT state is different than the CLK state then
      if (digitalRead(DT_PIN) != currentStateCLK) {
        // the encoder is rotating CCW so decrement
        counter--;
        currentDir = false;
      } else {
        // Encoder is rotating CW so increment
        counter++;
        currentDir = true;
      }
    }

    // Remember the last CLK state
    lastStateCLK = currentStateCLK;

    // Read the button state
    int clickState = digitalRead(CLICK_PIN);

    //set button status to false as we dont know the state yet
    buttonStatus = false;
    // If we detect LOW signal, button is pressed
    if (clickState == LOW) {
      // If 50ms have passed since last LOW pulse, it means that the
      // button has been pressed, released and pressed again
      if (millis() - lastButtonPress > 50) {
        buttonStatus = true;
      }

      // Remember last button press event
      lastButtonPress = millis();
    }
    // Put in a slight delay to help debounce the reading
    //not clean but it works so f u
    delay(1);
  }

  // Getter methods
  int count() const {
    return counter;
  }

  bool encoderDir() const {
    return currentDir;
  }

  bool button() const {
    return buttonStatus;
  }
};
//setting up the class rotaryEncoder to knob
RotaryEncoder knob;

//for debugging
//this prints to the lcd
void lcdTest(){
  lcd.print("This is a test");
  lcd.setCursor(0, 1);
  lcd.print("Hi!");
  delay(2000);
  lcd.clear();
}



//for debugging
//this prints to the lcd and the terminal 
//knob direction 1 = clockwise  0 = counterclockwise
//terminal says if its been clicked
void knobTest(){
  //read the current state of CLK
  currentStateCLK = digitalRead(CLK_PIN);

	// If last and current state of CLK are different, then pulse occurred
	// React to only 1 state change to avoid double count
  if (currentStateCLK != lastStateCLK  && currentStateCLK == 1){

    // If the DT state is different than the CLK state then
		// the encoder is rotating CCW so decrement
		if (digitalRead(DT_PIN) != currentStateCLK) {
			counter --;
			currentDir = false;
		} else {
			// Encoder is rotating CW so increment
			counter ++;
			currentDir = true;
		}
    Serial.print("direction: ");
    Serial.print(currentDir);
    Serial.print(" | Counter: ");
		Serial.println(counter);

    lcd.setCursor(0,0);
    lcd.print("direction: ");
    lcd.print(currentDir);
    lcd.setCursor(0,1); 
    lcd.print(" | Counter: ");
    lcd.print(counter);
  }
  //remember the last CLK state
  lastStateCLK = currentStateCLK;

  //read the button state
  int clickState = digitalRead(CLICK_PIN);

  //If we detect LOW signal, button is pressed
	if (clickState == LOW) {
		//if 50ms have passed since last LOW pulse, it means that the
		//button has been pressed, released and pressed again
		if (millis() - lastButtonPress > 50) {
			Serial.println("Button pressed!");
		}

		// Remember last button press event
		lastButtonPress = millis();
	}

	// Put in a slight delay to help debounce the reading
	delay(1);
}



//displays whenever you fuckup and hit an edgecase (hopefully)
//will prevent code from running ( goes into a infinite while loop)
//USE:  input the error you want to display, like the funtion name 
//i.e. error("mlToSteps");
void error(String code){
  lcd.clear();
  lcd.home();
  lcd.print("ERROR! Code:");
  lcd.setCursor(0, 1);
  lcd.print(code);
  lcd.setCursor(0, 3);
  lcd.print("im just chilling rn");
  while(true){}
}



//forces a restart
//used in the external interruptn and the endStop
//im becoming more and more lazy
void softwareReset() {
  wdt_enable(WDTO_15MS);
  while (1) {}  // This loop will force the reset
}


//For the interruptHandler to call
//moves the motor some and displays a screen
//waits until user aknowlages that they fucked up
void endStop(){
  statusLED(red);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("      Warning:      ");
  lcd.setCursor(0, 1);
  lcd.print("   End of Syringe   ");


  //void stepMotor(bool motorDirection, int steps, float speed)
  //move the motor back 3cm at .5cm/s
  stepMotor(1,(SPR/PITCH),1, 1);


  lcd.setCursor(0, 2);
  lcd.print(" >Click to Confirm< ");

  knob.read();
  while(knob.button() == false){
    knob.read();
  }
  //when the knob is pressed restart
  softwareReset();
  /*
  displayScreen = 1;
  displayUpdated == false;
  //return;
  //Serial.print("test");
  */

} 



//runs only if limit switch is pressed
//prevents machine from overshooting
void interruptHandler() {
  //TODO
  //Serial.print("fu");
  //sometimes when its turned on the interrupt hits, so this just adds a small delay
  if (millis() > 1000){
    endStop();
  }
  
  error("endstop func fail");
  /*
  //Beyond the edge, whispers weave, Here be dragons, secrets they leave.
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("   Beyond the edge  ");
  lcd.setCursor(0, 1);
  lcd.print("   whispers weave   ");
  lcd.setCursor(0, 2);
  lcd.print("  here be dragons   ");
  lcd.setCursor(0, 3);
  lcd.print(" secrets they leave ");
  */
  //while(true){}
  
}



//This is the first screen that will display on the screen
//the user will set the zero position and then confirm it
void screen1(){

  //spits out screen
  if(displayUpdated == false){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("  Zero Out Syringe");
    lcd.setCursor(0,2);
    lcd.print(">Click to Complete<");
    //shows that the display is up to date
    displayUpdated = true;
  }
  //moves the motor depending on what jog button has been pressed
  if (digitalRead(BUTTON_CW_PIN) == LOW && digitalRead(BUTTON_CCW_PIN) == LOW){
    stepMotor(0,(SPR/PITCH)/20,.2, 0);
    //jog(1,.5);
  }
  else if (digitalRead(BUTTON_CW_PIN) == LOW){
    stepMotor(1,(SPR/PITCH)/20,1, 0);
    //jog(1,5);
  }
  else if (digitalRead(BUTTON_CCW_PIN) == LOW){
    stepMotor(0,(SPR/PITCH)/20,1, 0);
    //jog(0,5);
  }

  knob.read();
  if (knob.button() == true){
    displayScreen = 2;
    stepperCountOrgin = stepperCount;
    displayUpdated = false;

  }
}

//used to select what syringe size the user wants
//0 is 100ml, 1 is 200ml
bool syringeSize = 0;
//this is the second screen that will appear, it will ask for the user to select 
//100ml or 200ml syringe
//turn knob to pick and click to confirm
void screen2(){
  if(displayUpdated == false){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Select syringe size ");

    //checks what the value of syringeSize is and displays accordingly
    if(syringeSize == 0){
      lcd.setCursor(0,1);
      lcd.print("       >10ml<       ");
      lcd.setCursor(0, 2);
      lcd.print("        20ml        ");
      }
      else{
      lcd.setCursor(0,1);
      lcd.print("        10ml        ");
      lcd.setCursor(0, 2);
      lcd.print("       >20ml<       ");
      }

    //shows that the display is up to date
    displayUpdated = true;
  }
  knob.read();
  //checks if the knob has been turned 
  //if so it changes the value of syringeSize
  if (knob.count() != knobCounter){
    syringeSize = !syringeSize;
    knobCounter = knob.count();
    displayUpdated = false;
  }

  //checks if the knob has been pressed
  //if so it finishes and moves to screen 3
  if (knob.button() == true){
    displayScreen = 3;
    displayUpdated = false;

  }
}


//selects what mode of operation the user wants
//0 is ml/min
//1 is ml
bool operationMode = 0;
//third screen, asks the user if they want a constant ml/min flow, or a set ml
//very similar to screen2 function wise
void screen3(){
  if(displayUpdated == false){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("    Select Mode");
    lcd.setCursor(0,1);
    lcd.print("    of Operation");

    //checks what the value of syringeSize is and displays accordingly
    if(operationMode == 0){
      lcd.setCursor(0,2);
      lcd.print("      >ml/min<");
      lcd.setCursor(0, 3);
      lcd.print("         ml");
      }
      else{
      lcd.setCursor(0,2);
      lcd.print("       ml/min");
      lcd.setCursor(0, 3);
      lcd.print("        >ml<");
      }

    //shows that the display is up to date
    displayUpdated = true;
  }
  knob.read();
  //checks if the knob has been turned 
  //if so it changes the value of operationMode
  if (knob.count() != knobCounter){
    operationMode = !operationMode;
    knobCounter = knob.count();
    displayUpdated = false;
  }

  //checks if the knob has been pressed
  //if so it finishes and moves to screen 3
  if (knob.button() == true){
    if (operationMode == 0){
      //if the operation mode is set to ml/min, then go to screen -4
      displayScreen = -4;
    }
    else{
      //if the operation mode is set to ml/min, then go to screen 4
      displayScreen = 4;
    }
    displayUpdated = false;
  }
}

//screen for the constaint ml/min flow
//this uses the accelstepper library because I forgot we had to use it...
//I also forgot that the project was to make a constaint flow syringe pump...
//yeahhh
void screen4a(){
  if(displayUpdated == false){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(" Constant Flow Rate");
    lcd.setCursor(5,1);
    lcd.print(mlToExtrude,1);
    lcd.print("ml/min");
    //shows that the display is up to date
    displayUpdated = true;
  }
  knob.read();
  
  //checks if the knob has been turned 
  //if so it changes the value of mlToExtrude, either adding .1 or subtracting .1
  //This is the same variable as the screen 4 ml option  but these wont conflict
  if (knob.count() > knobCounter){
    mlToExtrude += .1;
    knobCounter = knob.count();
    displayUpdated = false;
  }
  else if (knob.count() < knobCounter){
    mlToExtrude -= .1;
    knobCounter = knob.count();
    displayUpdated = false;
  }

  //checks if the knob has been pressed
  //if so it finishes and moves to screen 3
  if (knob.button() == true || digitalRead(toggleSwitch) == 0){
    displayScreen = -5;
    displayUpdated = false;

  }
}



//the number of steps to do in a second
float stepsPerMlPerSec = 0;
//The last screen for part a, it just shows extruding while a constant flow is being outputted. click to stop with either the toggle or the knob.
void screen5a(){
  if(displayUpdated == false){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("    Extruding...");

    //shows that the display is up to date
    displayUpdated = true;
  }
  //gets the proper radius of the syringe for calculations and temp stores it in i
  float i;
  if (syringeSize == 0){i = ML_100_RADIUS;}
  else if (syringeSize == 1){i = ML_200_RADIUS;}
  //finds how many steps are needed

  //sets up the direction the motor will spin depending on if we are extruding or sucking
  bool motorDirection;
  if (mlToExtrude <= 0){motorDirection = 1;}
  else if (mlToExtrude > 0){motorDirection = 0;}
  //send the command to start moving at x ml a minute
  //im lazy so the (abs(mlToExtrude) / mlToExtrude) just makes the rest of everything a positive or negative
  stepsPerMlPerSec = (abs(mlToExtrude) / mlToExtrude) * (mlToSteps(abs(mlToExtrude), i) / -60.0);
  stepper.setSpeed(stepsPerMlPerSec);

  //continually extrudes until the knob is pressed or the toggle switched is pressed
  knob.read();
  while(knob.button() == false && digitalRead(toggleSwitch) == 0){/////////////////////////////////////////////////////////////////////////////////////////////////////////////
    statusLED(green);
    stepper.runSpeed();
    knob.read();
  }
  statusLED(yellow);
  //once you hit the knob, move back to the operation mode screen.
  displayScreen = 3;
  displayUpdated = false;
}

//big fucking meatballs
//this screen asks how many ml you want extruded, turn the knob to change the ml
//click the knob to confirm
void screen4(){
  if(displayUpdated == false){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("   Extrude amount");
    lcd.setCursor(7,1);
    lcd.print(mlToExtrude,1);
    lcd.print("ml");
    //shows that the display is up to date
    displayUpdated = true;
  }
  knob.read();

  //checks if the knob has been turned 
  //if so it changes the value of mlToExtrude, either adding .1 or subtracting .1
  if (knob.count() > knobCounter){
    mlToExtrude += .1;
    knobCounter = knob.count();
    displayUpdated = false;
  }
  else if (knob.count() < knobCounter){
    mlToExtrude -= .1;
    knobCounter = knob.count();
    displayUpdated = false;
  }

  //checks if the knob has been pressed
  //if so it finishes and moves to screen 3
  if (knob.button() == true){
    displayScreen = 5;
    displayUpdated = false;

  }

}


//this actually extrudes the material
//when this is completed, loop back to screen3
void screen5(){
  if(displayUpdated == false){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("    Extruding...");

    //shows that the display is up to date
    displayUpdated = true;
  }
  //gets the proper radius of the syringe for calculations and temp stores it in i
  float i;
  if (syringeSize == 0){i = ML_100_RADIUS;}
  else if (syringeSize == 1){i = ML_200_RADIUS;}
  //finds how many steps are needed
  //stepMotor(bool motorDirection, int steps, float speed)

  //sets up the direction the motor will spin depending on if we are extruding or sucking
  bool motorDirection;
  if (mlToExtrude <= 0){motorDirection = 1;}
  else if (mlToExtrude > 0){motorDirection = 0;}
  //send the command to begin extrusion at .3 cm/s
  stepMotor(motorDirection, mlToSteps(abs(mlToExtrude), i), .3, 0);

  //finish and return to screen 3
  displayUpdated = false;
  displayScreen = 3;
}

//SETUP!!
void setup() {
  Serial.begin(9600);
  lcd.begin(20, 4); //start up the lcd

  //setting up the input pins 
  pinMode(CLICK_PIN, INPUT_PULLUP);
  pinMode(CLK_PIN, INPUT_PULLUP);
  pinMode(DT_PIN, INPUT_PULLUP);

  pinMode(BUTTON_CW_PIN, INPUT_PULLUP);
  pinMode(BUTTON_CCW_PIN, INPUT_PULLUP);
  pinMode(LIMIT_PIN, INPUT_PULLUP); //the external interupt pin !IMPORTAINT!

  pinMode(DIRECTION_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);

  pinMode(toggleSwitch,INPUT_PULLUP);

  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(blueLED, OUTPUT);

  //sets the max speed limit of the stepper motor for accelstepper.
  stepper.setMaxSpeed(1000);

  //starts an external interrupt
  //if the limit switch is touched, it immedietly runs
  attachInterrupt(digitalPinToInterrupt(LIMIT_PIN), interruptHandler, FALLING);

  //remebers the initial position of the rotary encoder
  lastStateCLK = digitalRead(CLK_PIN);
}


//LOOP!!
void loop() {
  //lcdTest();
  //knobTest();
  // while(1){
  //  jog(1, 2);
  // }

  //start of with a yellow LED
  statusLED(yellow);
  switch (displayScreen){
    case 1:
      screen1();
      break;
    case 2:
      screen2();
      break;
    case 3:
      screen3();
      break;
    case 4:
      screen4();
      break;
    case 5:
      screen5();
      break;
    case -4:
      screen4a();
      break;
    case -5:
      screen5a();
      break;
    default:
      error("screen switchcase");
      break;
  }
}