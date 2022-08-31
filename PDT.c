/*
 * Authors:
 *          Federal University of Paraná
 *          Federal Institute of Paraná
 *          Federal Technological University of Paraná
 * Date   : June 6, 2022
 *
 * PINING OF PHOTODYNAMIC THERAPY
 * 
 * Display LCD I2C: SDA GPIO21
 *                  SCL GPIO22
 * 
 * Potentiometers: pRed  GPIO15
 *                 pGreen  GPIO2
 *                 pBlue GPIO4
 * 
 * pinTouchStart 13    //Button of Click
 * pinTouchLess 12    //-
 * pinTouchMore 14    //+
 *  
 * Tape of Led WS1228b: leds GPIO5
 * 
 * Lux sensor BH1750FVI:  SDA GPIO21
 *                        SCL GPIO22
 *                           
 * Buzzer: GPIO23   
*/

//LIBRARIES/
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>

#include <Adafruit_NeoPixel.h>
#ifdef _AVR_
#include <avr/power.h>     // Required for 16 MHz Adafruit Trinket
#endif

LiquidCrystal_I2C lcd(0x27, 16, 2);

void displaySensorDetails();
void configureSensor();

Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

//PIN DEFINITION/

//Pins Adjustments Intensity of each color of LED
#define pRed 15   //Red Potentiometer
#define pGreen 2  //Green Potentiometer
#define pBlue 4   //Blue Potentiometer

#define pLeds 5      //Pins of Leds
#define NUMPIXELS 30 //Number of leds
Adafruit_NeoPixel pixels(NUMPIXELS, pLeds, NEO_GRB + NEO_KHZ800);

//Menu Adjustment Pins
#define pinoTouchStart 13  //Button of Click
#define pinoTouchLess 14   //Decrement -
#define pinoTouchMore 12   //Increment + 
#define buzzer 23          //Buzzer

//CREATION AND INITIALIZATION OF VARIABLES/
byte setRed = 0;    //Red color intensity adjustment
byte setGreen = 0;  //Green color intensity adjustment
byte setBlue = 0;   //Blue color intensity adjustment
byte startRed = 0;     //Red color operating intensity
byte startGreen = 0;   //Green color operating intensity
byte startBlue = 0;    //Blue color operating intensity


//rotary encoder variables
int contPosicao = 0;     //account positions
int posicaoAnt;          //last position record
int verificaGiro;        //stores reading from rotating CLK pin
boolean verificaSentido; //checks the encoder clockwise/counterclockwise rotation direction

//configuration and operation variables
unsigned int count_time = 61;            //initial value of the time that appears on the display
unsigned int total_count_time = 60;    //process start time
byte menu = 0;                      //0-menu configuration | 1-display operation | 2-Finished process
int measured_lux = 0;                //sensor reading in lux
double irradiance = 0;           //sensor reading in Joule/cm^2
double irradiance_final = 0;     //final reading in Joule/cm^2
unsigned int time_left = 0;   //time in minutes
bool flagI2C = false;               //I2C shared between lux sensor and LCD
bool flagcount_time = LOW;               //1 running, 0 off

double irradiated_area = 660; //660 cm² of area irradiated on botom of box
double LEDpower = 9; //Power of LED (0W to 9W)

//Start the task queue
QueueHandle_t fila;
int tamanhodafila = 50;

//SETTINGS/
void setup() 
{ 
  //starts Serial Communication
  Serial.begin(115200); 

  //inicia Display LCD
  lcd.begin ();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Starting...");
  lcd.setCursor(0,1);

  //Configure Buzzer Pin
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, HIGH);
  delay(1000);
  digitalWrite(buzzer, LOW);
    
  //Configure LED Adjustment Pins
  pinMode(pRed, INPUT);
  pinMode(pGreen, INPUT);
  pinMode(pBlue, INPUT);

  //Configure and initialize LEDs
  #if defined(_AVR_ATtiny85_) && (F_CPU == 16000000)
    clock_prescale_set(clock_div_1);
  #endif
  pixels.begin();
  pixels.clear(); // turn off the leds
    
  //Configure TSL2561 Light Sensor
  //use tsl.begin() to default to Wire, 
  //tsl.begin(&Wire2) directs api to use Wire2, etc.
  if(!tsl.begin())
  {
    Serial.print("TSL2561 lux sensor detection error, check addressing");
         for(int i=0;i<6;i++)
        {
          digitalWrite(buzzer,HIGH);
          delay(300);
          digitalWrite(buzzer,LOW);
          delay(300);
        }  
  }  
  displaySensorDetails();
  configureSensor();

   xTaskCreatePinnedToCore
  (
    TaskReadLux,      /* Task or Role  */
    "TaskReadLux",    /* Task or Role Name  */
    10000,            /* Stack Size  */
    NULL,             /* Input Parameter  */
    2,                /* Task Priority  */
    NULL,             /* Task Identifier  */
    0                 /* Task Processor - 0 or 1  */
  ); 

   xTaskCreatePinnedToCore
  (
    TaskCountTime,  /* Task or Role  */
    "TaskCountTime",/* Task or Role Name  */
    10000,             /* Stack Size  */
    NULL,              /* Input Parameter  */
    2,                 /* Task Priority  */
    NULL,              /* Task Identifier  */
    1                  /* Task Processor - 0 or 1  */
  ); 
  
  xTaskCreatePinnedToCore
  (
    TaskReadSettings,  /* Task or Role  */
    "TaskReadSettings",/* Task or Role Name  */
    10000,            /* Stack Size  */
    NULL,             /* Input Parameter  */
    2,                /* Task Priority  */
    NULL,             /* Task Identifier  */
    0                 /* Task Processor - 0 or 1 */
  ); 

  xTaskCreatePinnedToCore
  (
    TaskDisplayLCD,  /* Task or Role  */
    "TaskDisplayLCD",/* Task or Role Name  */
    10000,             /* Stack Size  */
    NULL,              /* Input Parameter  */
    2,                 /* Task Priority  */
    NULL,              /* Task Identifier  */
    1                  /* Task Processor - 0 or 1  */
  ); 

  xTaskCreatePinnedToCore
  (
    TaskLedsRefresh,  /* Task or Role  */
    "TaskLedsRefresh",/* Task or Role Name  */
    10000,               /* Stack Size  */
    NULL,                /* Input Parameter  */
    2,                   /* Task Priority  */
    NULL,                /* Task Identifier  */
    1                    /* Task Processor - 0 or 1  */
  ); 

  Serial.println("Setup concluded.");
}

void loop() 
{
  delay(5000);
    Serial.println("Running Loop.");
}
 
void TaskReadSettings( void * parameter)
{
  while(menu == 0)
  { 
    delay(300);
    Serial.println("Task 1 - Read settings.");
    int aux_setRed = analogRead(pRed);       //Red LED adjustment
    int aux_setGreen = analogRead(pGreen);   //Green LED adjustment
    int aux_setBlue = analogRead(pBlue);     //Blue LED adjustment

    if((aux_setRed<=0)||(aux_setRed>10000))aux_setRed=0;
    else if(aux_setRed>=4095)aux_setRed=4095;
    if((aux_setGreen<=0)||(aux_setGreen>10000))aux_setGreen=0;
    else if(aux_setGreen>=4095)aux_setGreen=4095;
    if((aux_setBlue<=0)||(aux_setBlue>10000))aux_setBlue=0;
    else if(aux_setBlue>=4095)aux_setBlue=4095;
        
    setRed = map(aux_setRed, 4095, 0, 0, 255);       //set the range to 0-255
    if((setRed<=0)||(setRed>10000))setRed=0;
    else if(setRed>=255)setRed=255;
    
    setGreen = map(aux_setGreen, 4095, 0, 0, 255);   //set the range to 0-255
    if((setGreen<=0)||(setGreen>10000))setGreen=0;
    else if(setGreen>=255)setGreen=255;
    
    setBlue = map(aux_setBlue, 4095, 0, 0, 255);     //set the range to 0-255
    if((setBlue<=0)||(setBlue>10000))setBlue=0;
    else if(setBlue>=255)setBlue=255;

    LEDpower = (setRed + setGreen + setBlue)*9/765; //(0~9W) LED Power based on RGB Settings
    
      if(touchRead(pinoTouchMore) < 20)
      {
        Serial.print("Time: "); 
        Serial.println(count_time); 
        delay(100);
        if(count_time<1440)count_time++;
        delay(400);
       if(touchRead(pinoTouchMore) < 20)
        {
           if(count_time<1430)count_time=count_time+9;
        }
     }
      if(touchRead(pinoTouchLess) < 20)
      {
        Serial.print("Time: "); 
        Serial.println(count_time); 
        delay(100);
        if(count_time>1)count_time--;
        delay(400);
      if(touchRead(pinoTouchLess) < 20)
      {
        if(count_time>=10)count_time=count_time-9;
        if(count_time>10000)count_time=0;      
      }
     }
      int cont = 0;
      while(touchRead(pinoTouchStart) < 20)
      {
        delay(100);
        cont++;
        if(cont > 25)
        {
          Serial.print("Start button pressed."); 
          menu = 1;
          digitalWrite(buzzer,HIGH);
          delay(1500);
          digitalWrite(buzzer,LOW);
          delay(500);
       }
             
     }
 
    delay(5);
  }
   
  /* Delete the current task */ 
  vTaskDelete( NULL ); 
}

void TaskDisplayLCD( void * parameter)
{
  while(1)
  {
    delay(300);
    Serial.println("Task 2 - Display LCD");
    while(flagI2C == true){delay(5);} //if I2C is in use wait
    flagI2C = true;

    if(menu==0)//display settings menu
    {
      lcd.begin();
      //lcd.clear();
      //delay(10);      
      //delay(10);
      lcd.setCursor(0,0);
      lcd.print("R");
      lcd.print(setRed);
      
      //delay(10);
      lcd.setCursor(6,0);
      lcd.print("G");
      lcd.print(setGreen);
      
      //delay(10);
      lcd.setCursor(12,0);
      lcd.print("B");
      lcd.print(setBlue);      
      
      lcd.setCursor(0,1);
      lcd.print("T: ");
      lcd.print(count_time);
      lcd.print("min ");

      lcd.print("P:");
      lcd.print(LEDpower);
      lcd.print("W");
    }
    else if(menu==1) //display operation screen
    {      
      lcd.begin();
      //lcd.clear();
      //delay(10);
      
      //delay(10);
      lcd.setCursor(0,0);
      lcd.print(measured_lux);
      lcd.print(" lx ");
      lcd.print("TL:");
      lcd.print(time_left);
      lcd.print("'");

      lcd.setCursor(0,1);
      lcd.print("I:"); //Instantaneous: Irradiance
      
      lcd.print(irradiance);
      lcd.print("mJ/cm2s");
    
      delay(5000);
    }
    else if(menu==2) //display completed process
    {      
      lcd.begin();
      lcd.setCursor(0,0);
      lcd.print("Finished.");
      delay(3000);

      
        lcd.begin();
        lcd.setCursor(0,0);
        lcd.print("I:"); //Instantaneous: Irradiance
        lcd.print(irradiance_final);
        lcd.print("mJ/cm2s");

        lcd.setCursor(0,1);
        lcd.print("F:"); //Total Final: Fluence
        double fluence = irradiance_final*total_count_time*60/1000;
        lcd.print(fluence);
        lcd.print("J/cm2");
          
      while(1){delay(1000);}
    }

    flagI2C = false;
  }
  /* Delete the current task */ 
  vTaskDelete( NULL ); 
}

void TaskReadLux( void * parameter)
{
  while(1)
  {
    delay(4000);
    while(flagI2C == true){delay(5);} //if I2C is in use wait
    flagI2C = true;
    
    Serial.println("Task 3 - Read Lux.");
    /* Get a new sensor event */ 
    sensors_event_t event;
    tsl.getEvent(&event);
   
    /* Display the results (light is measured in lux) */
    if (event.light)
    {
      Serial.print(event.light);
      Serial.print(" lux - ");
    }
    else
    {
      /* If event.light = 0 lux the sensor is probably saturated
         and no reliable data could be generated! */
      Serial.println("Sensor overload.");
    }
      
    measured_lux = event.light;             //measurement in lux
    irradiance = LEDpower*1000/irradiated_area;  //irradiance in mW/cm² or mJ/(s.cm²)
    Serial.print(irradiance);
    Serial.println(" mJ/cm^2");
    
    if(irradiance > 0.05)irradiance_final = irradiance; //if it is still on, it assigns the reading to the final calculation
    
    flagI2C = false;
    delay(1000);
    if(menu==2)
    {
      while(1){delay(1000);}
    }
  }
  vTaskDelete(NULL);
}

void TaskCountTime( void * parameter)
{
  while(1)
  {
    Serial.println("Task 4 - Time Account");
    if(menu==0) //menu 0 - configures colors and time
    {
      time_left=count_time;
      total_count_time = count_time;
      delay(500);
    }
    else if (menu==1) //menu 1 - countdown timer operation
    {
      delay(60000);
      time_left--;
      if(time_left<=0)
      {time_left=0;
        menu = 2; //ends the process
        for(int i=0;i<4;i++)
        {
          digitalWrite(buzzer,HIGH);
          delay(1500);
          digitalWrite(buzzer,LOW);
          delay(500);
        }
        while(1){delay(1000);}
      }
    }    
  }
  vTaskDelete(NULL);
}

void TaskLedsRefresh( void * parameter)
{
  while(1)
  {
    Serial.println("Task 5 - LEDs");
    if(menu==0) //menu 0 - configures colors and time
    {
       pixels.clear(); //turn off the LEDs
       /*for(int i=0; i<NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(setRed, setGreen, setBlue));    
        pixels.show();   // update the color of the leds
        delay(30);*/
        startRed = setRed;
        startGreen = setGreen;
        startBlue = setBlue;
      //}
      delay(200);
    }
    else if(menu == 1) //menu 1 - operation with fixed LED color
    {
       for(int i=0; i<NUMPIXELS; i++)
       {
        pixels.setPixelColor(i, pixels.Color(startRed, startGreen, startBlue));    
        pixels.show();   // update the color of the leds
        delay(20);
       }
       while(menu==1){delay(1000);}
    }
    else if(menu == 2) //menu 2 - completed the process
    {
       pixels.clear(); //turn off the LEDs
       for(int i=0; i<NUMPIXELS; i++)
       {
        pixels.setPixelColor(i, pixels.Color(0, 0, 0));    
        pixels.show();   // update the color of the leds
        delay(20);
       }
       while(1){delay(1000);}
    }
  }
  vTaskDelete(NULL);
}

//Sensor Setup Functions
void displaySensorDetails(void)
{
  sensor_t sensor;
  tsl.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" lux");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" lux");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" lux");  
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

void configureSensor(void)
{
  /* You can also manually set the gain or enable auto-gain support */
  // tsl.setGain(TSL2561_GAIN_1X);      /* No gain ... use in bright light to avoid sensor saturation */
  // tsl.setGain(TSL2561_GAIN_16X);     /* 16x gain ... use in low light to boost sensitivity */
  tsl.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */
  
  /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */

  /* Update these values depending on what you've set above! */  
  Serial.println("------------------------------------");
  Serial.print  ("Gain:         "); Serial.println("Auto");
  Serial.print  ("Timing:       "); Serial.println("13 ms");
  Serial.println("------------------------------------");
}
