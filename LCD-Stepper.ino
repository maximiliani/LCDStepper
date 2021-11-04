#include <LiquidCrystal_I2C.h>
#include <AccelStepper.h>

#define reverseSwitch 2  // Push button for reverse, menu, start
#define driverPUL 7    // PUL- pin
#define driverDIR 6    // DIR- pin
#define spd A0     // Potentiometer
const unsigned int uSteps[] = {400, 800, 1000, 1600, 2000, 3200, 4000, 5000, 6400, 8000, 10000, 12800, 20000, 25600, 40000, 51200}; // available microsteps per rotation

#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args)  write(args);
#else
#define printByte(args)  print(args,BYTE);
#endif

AccelStepper stepper(1,7,6);      // Motor config
LiquidCrystal_I2C lcd(0x27,16,2); // Display config

// these values change during the runtime
int microsteps = 400; // value from uSteps-Array - configurable via Serial or Display
String rawString;
float speed; // speed - configurable via Serial or Display
unsigned int steps;
bool forw; // direction - configurable via Serial or Display

// only for display control
unsigned long displayMillis = 0;
unsigned long serialMillis = 0;
bool start = false;
bool setRPM = false;
bool setSTEPS = false;
int count = 0;
bool conf;

// This method acts as an interrupt handler for the button input
void revmotor (){ 
  if(start){ //change direction
    setRpm(0);
    start = false;
    getManualValues();
    }
  else {
    count ++;
    delay(500);
    }
}

// This method handles the display control.
void getManualValues(){
  if(count >= 5 && conf){
      actualizeDisplay();
      start = true;
      count = 0;
      speed = 0;
      steps = 0;
      forw = false;
      conf = false;
  }
  else if (count == 1){
    lcd.setCursor(0, 0);
    lcd.print("Enter Microsteps");
    lcd.setCursor(0, 1);
    steps = uSteps[map(analogRead(spd),0,1023,0,16)];
    lcd.print(steps);
    if(steps<1000) lcd.print("             ");
    if(steps<10000) lcd.print("            ");
  }
  else if (count == 2){
    microsteps = steps;
    lcd.setCursor(0, 0);
    lcd.print("Enter RPM:      ");
    lcd.setCursor(0, 1);
    speed = map(analogRead(spd),0,1023,0,175);
    lcd.print(speed);
    lcd.print("             ");
  }
  else if (count == 3){
    setRpm(speed);
    lcd.setCursor(0, 0);
    lcd.print("Toggle direction");
    lcd.setCursor(0, 1);
    if(analogRead(spd)> 10){
      lcd.print("Forward  ");
      lcd.printByte(126);
      lcd.print("     ");
      forw = true;
    }
    else{
      lcd.printByte(127);
      lcd.print(" Backward");
      lcd.print("     ");
      forw = false;
    }
  }
  else if (count == 4){
    if(!forw) stepper.setSpeed(-stepper.speed());
    lcd.setCursor(0, 0);
    lcd.print("START?         ");
    lcd.setCursor(0, 1);
    if(analogRead(spd)> 10){
      lcd.print("      YES       ");
      conf = true;
    }
    else{
      lcd.print("      NO        ");
      start = false;
      count = 0;
      conf = false;
    }
  }
  else count = 0;
}

// This method returns the calculated speed in RPM.
float rpm(){
  float sps = stepper.speed(); // get speed from stepper library
  float rps = sps/ microsteps;
  return rps*60;
}

// This method sets the speed.
void setRpm(float rpm){
  float rps = rpm/60; // calculation to rotations per second
  stepper.stop(),
  stepper.setSpeed(rps* microsteps); // calculation to steps per second
  stepper.runSpeed();
}

// This method prints the welcome screen on the display.
void lcdSetup(){
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("RPM-Display by");
  lcd.setCursor(0, 1);
  lcd.print("maximiliani");
  delay(3000);
  lcd.setCursor(0, 0);
  lcd.print("Controllable via");
  lcd.setCursor(0, 1);
  lcd.print("Serial console");
  delay(3000);
  lcd.setCursor(0, 0);
  lcd.print("Serial0/USB/UART");
  lcd.setCursor(0, 1);
  lcd.printByte(126);
  lcd.print(" 115200 Baud ");
  delay(3000);
}

// This method prints the actual speed, microsteps and the direction on the display.
// It is only called when new input is entered manually or via Serial console.
void actualizeDisplay(){
  lcd.setCursor(0, 0);
  String stringSPS = (String)speed;
  int empty = 16 - (stringSPS.length() +6);  
  lcd.print(rpm());
  lcd.print(" RPM ");

  if (stepper.speed() == 0) {
    lcd.printByte(6);
  }
  else if (stepper.speed() > 0){
    lcd.printByte(126);
  }
  else if (stepper.speed() < 0){
    lcd.printByte(127);
  } 
  for(int i = 0; i<empty; i++) lcd.print(" ");
 
  lcd.setCursor(0, 1);
  String stringSTEPS = (String)microsteps;
  empty = 16 - (stringSTEPS.length() +4);
  lcd.print(stringSTEPS),
  lcd.print(" Microsteps");
  for(int i = 0; i<empty; i++) lcd.print(" ");
}

// This method prints the welcome messages on the serial console.
void serialSetup(){ 
  Serial.begin(115200);
  Serial.flush();
  Serial.println("RPM-Display by maximiliani");
  Serial.println("..............................");
  Serial.println("This console enables the control of the steppermotor and the display.");
  Serial.println("This is a list of all possible commands (You have to make a new line after each command/value.):");
  Serial.println("- 'START' starts the Display and the motor. No value required.");
  Serial.println("- 'STOP' starts the Display and the motor. No value required.");
  Serial.println("- 'RPM' let you set the RPM in a new line. The value must be float or integer.");
  Serial.println("- 'STEPS' is necessary for an correct RPM-value. Please type in the next line the count of (micro)steps one Rotation takes.");
  Serial.println("- 'TOGGLEDIR' toggles the direction of rotation. No value required.");
  Serial.println("To get this information again, you'll have to restart the Arduino.");
  Serial.println();
}

// This method handles serial input.
void serialLoop(){ // handles serial commands
  if(Serial.available() > 0) {
    rawString = "";
    rawString = Serial.readStringUntil((char)'/');
    rawString.remove(rawString.length()-1);
    Serial.println("INPUT: " + rawString);

    if(setRPM){
      setRpm(rawString.toDouble());
      Serial.println("ok");
      actualizeDisplay();
      setRPM = false;
      }
    if(setSTEPS) {
      microsteps = rawString.toInt();
      if (microsteps < 0) microsteps = 200;
      Serial.println("ok");
      actualizeDisplay();
      setSTEPS = false;
    }
    else if(rawString == "RPM") setRPM = true;
    else if(rawString == "STEPS") setSTEPS = true;
    else if(rawString == "START") {
      start = true;
      Serial.println("ok");
      actualizeDisplay();
    }
    else if(rawString == "STOP") {
      setRpm(0);
      start = false;
      Serial.println("ok");
      actualizeDisplay();
    }
    else if(rawString == "TOGGLEDIR") {
      revmotor();
      Serial.println("ok");
      actualizeDisplay();
    }
  }
}

// This method prints the actual speed, microsteps and the direction on the serial console.
void actualizeSerial(){ // sends values over serial
  Serial.print(" SPS: " + (String)stepper.speed());
  Serial.print(" RPM: " + (String)rpm());
  Serial.print(" STEPS: " + (String)microsteps);
  if (stepper.speed() > 0) Serial.println(" DIRECTION: Forwards");
  else if (stepper.speed() < 0) Serial.println(" DIRECTION: Backwards");
  else Serial.println(" STOP");
}

// This method is called on startup and starts the setup methods.
void setup() {
  serialSetup();
  attachInterrupt(digitalPinToInterrupt(reverseSwitch), revmotor, FALLING);
  stepper.setMaxSpeed(1500);
  stepper.setAcceleration(20);
  lcdSetup();
}

// This method keeps the world running...
void loop() {
  if(start){
    stepper.runSpeed();

    if(millis() - serialMillis >= 1000){
    serialMillis = millis();
    actualizeSerial();
    }
  }
  else if (!start && millis()-displayMillis >=4000 && count == 0){
  // else{
    lcd.setCursor(0, 0);
    lcd.print("Please use the  ");
    lcd.setCursor(0, 1); 
    lcd.print("button or USB   ");
    delay(2000);
    lcd.setCursor(0, 0);
    lcd.print("when running to ");
    lcd.setCursor(0, 1);
    lcd.print("set the values. ");
    delay(2000);
  }
  if (!start && count > 0) {
    getManualValues();
    delay(300);
  }

  serialLoop();
}