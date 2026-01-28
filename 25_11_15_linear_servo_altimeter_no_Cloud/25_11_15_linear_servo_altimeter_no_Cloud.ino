// 11/15/25 notes
//apparently this is the most recent non-Cloud Rat altimeter

//11/7/22 notes
//distance measuring was intermittent, now OK
//changed distance for message to <30cm



//tested 5/2/22 - works still
 
//notes 2/5/21 
//this program adds on to the existing barometric pressure based altimeter by
//adding an extra PWM output which will drive the servo city linear actuator
//the new output pulse width range will vary from   to    us which will correspond to full
//range of the movement = 500 ft
//the pulse width will not be allowed to exceed the above range to avoid running the
//movement to the stops

//digital pins not used: 8, 10, 13  (0, 1 are rx, tx)
//I'll use pin 10 for the linear slider "LIN"

//notes 2/7/21
//a few questions to answer first: "HEIGHT is defined, where is it used?
//it's a dummy value, and is replaced with a calculated value in the loop
//try int HEIGHT - works fine

//lcd displays pulse with of altitude, not zero at first
//fix this so it displays PW of zero feet first, then altitutde

//"pulseAlt is defined as 2443, but some places use 2450?

//"4.026 is used some places, 4.073 is used in others?"


//notes 2/11/21
//this version will have pin 10 as slider output "LIN"

//notes 2/20/21
//will add lcd displays to go with the linear slider
//start with text "Linear Slider"


//notes 2/21/21
//add calculations for slider position
//add limits to pulse width to slider -not done yet

//notes 2/22/21
//data on slider travel 850-2200us = 17.6",  800-2200us = 18.0", 800-2250us = 18.5"
//want slider travel = 18", set to 820-2220us - check = 18.4"
//try 820-2200us check = 18.0" good







//measues and displays temp, atmospheric pressure, altitude, distance from sensor
//10/8 version will be used to add a "zero" display each cycle
//10/3 version is to save previous version (which works) in case of disaster
//problem - off by up to 6 feet or so from digital readout
//if pulse width is too wide servo locks out & power must be recycled????

//program order
// 1  setup- display welcome messages
// 2  loop- read and display pressure, temp, altitude
// 3  loop- read and display range - if range <10cm message is displayed
// 4  loop- cauculate pulse width for servo from altitude and send out PWM signal


//9/14 version will add PWM drive to servo (to display altitude)
//servo drive for altimeter pointer taken from 19_09_16_StrandWSoundMoreEyes
//pulse width will scale PW = 165 + (K*Altitude)
//servo 0 deg to 180 deg = altitude from AltMin to AltMax in feet

//example from Circuit Digest 
//works!
//added if (distance <10) on board LED lights - works
//9/2 test to move trig and echo to digital pins
//9/6 added "Army Boots" and "Cowboy Boots"
//9/10 try adding BMP280 test from
//20_09_10_BMP280_test
//9/11 works - added pressure,altitude and temp
//9/13 add way to enter current barometric pressure -works!
//also add LCD display of temperature


//ENTER CURRENT BAROMETRIC PRESSURE HERE
#define BAR_PRES  30.41
//units are in-Hg was 30.15



#include <LiquidCrystal.h>
 


#define trigger 11
#define echo 12


#define ALT 9  //PWM for dial altimeter
#define LIN 10 //PWM for linear slider

//added 2/20/21
float LinMin = 820; //pulse width for zero feet on slider
float LinMax = 2200; //pulse witdth for 500 feet on slider



int pulseAlt = 2443; //calibrated pulse width for altimeter servo 10/3
int i = 0;  //added 9/14

//added from BMP280
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
Adafruit_BMP280 bmp; // I2C


//added 9/13 
//#define CUR_PRES 100597 //units are in bar


int HEIGHT;

//new2/21/21
float R;
int LinPW; 
 
LiquidCrystal lcd(2,3,4,5,6,7);
 
float time=0,distance=0;
 
void setup()
{
  
 pinMode(ALT,OUTPUT); //added 9/14 altimeter ouput pin
 pinMode(LIN,OUTPUT); //added 2/11/21 linear slider output
 pinMode(trigger,OUTPUT);
 pinMode(echo,INPUT);
 digitalWrite(ALT, LOW);  //added for troubleshooting 10/1
 digitalWrite(LIN, LOW); //added 2/11/21 - not sure this is needed

 lcd.begin(16,2);
 lcd.print(" Ultra sonic");
 lcd.setCursor(0,1);
 lcd.print("Distance Meter");
 delay(1000);
 lcd.clear();
 lcd.print(" Circuit Digest");
 delay(1000);
 lcd.clear();
 lcd.print("Your Mom Wears");
 lcd.setCursor(0,1); //new 9/5
 lcd.print("Army Boots");
 delay(1000);
//added 9/10
Serial.begin(9600);
Serial.println(F("BMP280 test"));
 lcd.clear();
 lcd.print("BMP280 Test");
 lcd.setCursor(0,1); //new 9/5
 lcd.print("Temp Altitude");
 delay(1000);

//add 9/19 to display reference pressure

 lcd.clear();
 lcd.print("Sea Level Press ");
 lcd.setCursor(0,1); 
 lcd.print(BAR_PRES);
 lcd.print(" in-HG");   //added 9/19
 delay(2000);





 

//adde9/11 - runs with this
 lcd.clear();
  if (!bmp.begin()) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
  }
//added 9/11
 bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

 pinMode(LED_BUILTIN, OUTPUT); //added 9/1/20

}



 
void loop()
{


//section 1 of loop - display temp, altitude, pressure
 //added 9/10
  Serial.print(F("Temperature = "));
  Serial.print(bmp.readTemperature());
  Serial.println(" *C");

  

lcd.clear(); //added 9/1
lcd.setCursor(0,0);
lcd.print("Temp= ");
lcd.print(bmp.readTemperature());
lcd.print(" C"); //added 9/12
delay(2000);

 Serial.print(F("Pressure = "));
 Serial.print(bmp.readPressure());
 Serial.println(" Pa");

lcd.clear(); //added 9/12   
lcd.setCursor(0,0);
lcd.print("Press= ");
lcd.print(bmp.readPressure());

delay(1000); //added 9/12

lcd.setCursor(0,1);
lcd.print(bmp.readPressure()*.0002953);
lcd.print(" in-Hg");

delay(2000);

 Serial.print(F("Approx altitude = "));
 Serial.print(bmp.readAltitude(BAR_PRES*33.86)); //added 9/13

 Serial.println(" m");
 Serial.println();
 delay(2000);
    
//added 9/11  works finally!
lcd.clear();
lcd.setCursor(0,0);
lcd.print("altitude =");

lcd.setCursor(0,1);
lcd.print(bmp.readAltitude(BAR_PRES*33.86)); //added 9/13
lcd.print("m"); //added 9/12
delay(2000);  //added 9/12

lcd.clear();
lcd.setCursor(0,0);
lcd.print("altitude =");

lcd.setCursor(0,1);
lcd.print(bmp.readAltitude(BAR_PRES*33.86)*3.2808);  //addded 9/13
lcd.print(" feet "); //added 9/12
delay(2000);  //added 9/12


 //second section - measure and display distance,if <10 cm display message
 lcd.clear();
 digitalWrite(trigger,LOW);
 delayMicroseconds(2);
 digitalWrite(trigger,HIGH);
 delayMicroseconds(10);
 digitalWrite(trigger,LOW);
 delayMicroseconds(2);
 time=pulseIn(echo,HIGH);
 distance=time*340/20000;
 
 lcd.clear();
 lcd.print("Distance:");
 lcd.print(distance);
 lcd.print("cm");

Serial.print(distance);
Serial.println("  cm");
Serial.println(); 

 lcd.setCursor(0,1);
 lcd.print("Distance:");
 lcd.print(distance/100);
 lcd.print("m");
 delay(2000);
 lcd.clear();  //added 9/14

 if (distance <30) {   //changed from 10 on 11/7/22
 digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level) added 9/1
 delay(1000);                       // wait for a second
 digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
 
 lcd.clear();
 lcd.print("Your Mom Wears");
 lcd.setCursor(0,1); //new 9/5
 lcd.print("Cowboy Boots");
 delay(2000);                       // wait for a second

 lcd.clear();   //added 9/19
 } 



HEIGHT =(bmp.readAltitude(BAR_PRES*33.86))*3.2808; //height in feet
//third section - generates a PWM signal to drive altimeter servo
//have LCD show pulse width

 lcd.clear();  
 lcd.print("Pulse Width");

 lcd.setCursor(0,1); //new 9/19
 lcd.print(pulseAlt); //revised 2/7/21
 lcd.print(" us");
 

//added 10/8 to take dial to zero feet before displaying altitude
for (i= 0; i < 150; i++) { 

digitalWrite(ALT, HIGH); //start servo pulse
delayMicroseconds(2450);  //use to calibrate 10/3 2443
digitalWrite(ALT, LOW);
delay(20);
}

delay(1500);

//added 2/7/21 so can display zero ft PW, along with PW for altitude
lcd.clear();  
lcd.print("Pulse Width");

lcd.setCursor(0,1); //new 9/19
lcd.print(pulseAlt-(HEIGHT*3.7)); //this is PW for altitude, not zero ft
lcd.print(" us");


//changed from 300 on 10/1
for (i= 0; i < 300; i++) { 

digitalWrite(ALT, HIGH); //start servo pulse

delayMicroseconds(pulseAlt-HEIGHT*4.026);  //calibrated 10/3
//delayMicroseconds(pulseAlt+(bmp.readAltitude(BAR_PRES*33.86)*3.2808));
digitalWrite(ALT, LOW);
delay(20);
}


Serial.println(pulseAlt-HEIGHT*4.073);
Serial.println();
Serial.println();
lcd.clear();  //added 9/19
delay(1000);


//new 2/20/21
lcd.clear();
lcd.print("Linear Slider");
lcd.setCursor(0,1); //new 9/5
lcd.print("Altitude Display");
delay(1000);

//now show minimum altitude
//pulse width
lcd.clear();
lcd.print("Zero Feet");
lcd.setCursor(0,1); //new 9/5
lcd.print(LinMin);
lcd.print("us");
delay(1000);


//new 2/11/21
//drive for linear slider
//test by generating 

for (i= 0; i < 400; i++) { 

digitalWrite(LIN, HIGH); //start servo pulse
delayMicroseconds(LinMin);  //use to calibrate for zero feet
digitalWrite(LIN, LOW);
delay(20);
}

delay(1500);

//now show maximum altitude
//pulse width
lcd.clear();
lcd.print("500 Feet");
lcd.setCursor(0,1); //new 9/5
lcd.print(LinMax);
lcd.print("us");
delay(1000);



for (i= 0; i < 400; i++) { 

digitalWrite(LIN, HIGH); //start servo pulse
delayMicroseconds(LinMax);  //use to calibrate for 500 feet
digitalWrite(LIN, LOW);
delay(20);
}

delay(1500);


//now show current height
lcd.clear();
lcd.print("Altitude");
lcd.setCursor(0,1); 
lcd.print(HEIGHT);
lcd.print("ft");
delay(1000);


//2/21/21 new - add calcualtions for "R"

R= (LinMax-LinMin)/500;

lcd.clear();
lcd.print("R");
lcd.setCursor(0,1); 
lcd.print(R);
delay(3000);


LinPW = HEIGHT*R + LinMin;

//display PW for slider
lcd.clear();
lcd.print("Slider PW");
lcd.setCursor(0,1); 
lcd.print(LinPW);
lcd.print("us");
delay(3000);

//slider position for altitude
for (i= 0; i < 400; i++) { 

digitalWrite(LIN, HIGH); //start servo pulse
delayMicroseconds(LinPW);  //use to calibrate for zero feet
digitalWrite(LIN, LOW);
delay(20);
}

delay(1500);





}
