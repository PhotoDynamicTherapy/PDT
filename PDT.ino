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
 * pinTouchStart 13  	//Button of Click
 * pinTouchLess 12  	//-
 * pinTouchMore 14  	//+
 *  
 * Tape of Led WS1228b: leds GPIO5
 * 
 * Lux sensor BH1750FVI:  SDA GPIO21
 *                        SCL GPIO22
 *                           
 * Buzzer: GPIO23   
*/

/*LIBRARIES*/
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>     // Required for 16 MHz Adafruit Trinket
#endif

LiquidCrystal_I2C lcd(0x27, 16, 2);

void displaySensorDetails();
void configureSensor();

Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

/*PIN DEFINITION*/

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

/*CREATION AND INITIALIZATION OF VARIABLES*/
byte ajusteRed = 0;    //Red color intensity adjustment
byte ajusteGreen = 0;  //Green color intensity adjustment
byte ajusteBlue = 0;   //Blue color intensity adjustment
byte operaRed = 0;     //Red color operating intensity
byte operaGreen = 0;   //Green color operating intensity
byte operaBlue = 0;    //Blue color operating intensity


//rotary encoder variables
int contPosicao = 0;     //account positions
int posicaoAnt;          //last position record
int verificaGiro;        //stores reading from rotating CLK pin
boolean verificaSentido; //checks the encoder clockwise/counterclockwise rotation direction

//configuration and operation variables
unsigned int tempo = 61;            //initial value of the time that appears on the display
unsigned int tempo_inicial = 60;    //process start time
byte menu = 0;                      //0-menu configuration | 1-display operation | 2-Finished process
int leitura_lux = 0;                //sensor reading in lux
double leitura_joule = 0;           //sensor reading in Joule/cm^2
double leitura_joule_final = 0;     //final reading in Joule/cm^2
unsigned int tempo_decorrido = 0;   //time in minutes
bool flagI2C = false;               //I2C shared between lux sensor and LCD
bool flagTempo = LOW;               //1 running, 0 off

//Start the task queue
QueueHandle_t fila;
int tamanhodafila = 50;

/*SETTINGS*/
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
  #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
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
    TarefaLeLux,      /* Task or Role  */
    "TarefaLeLux",    /* Task or Role Name  */
    10000,            /* Stack Size  */
    NULL,             /* Input Parameter  */
    2,                /* Task Priority  */
    NULL,             /* Task Identifier  */
    0                 /* Task Processor - 0 or 1  */
  ); 

   xTaskCreatePinnedToCore
  (
    TarefaContaTempo,  /* Task or Role  */
    "TarefaContaTempo",/* Task or Role Name  */
    10000,             /* Stack Size  */
    NULL,              /* Input Parameter  */
    2,                 /* Task Priority  */
    NULL,              /* Task Identifier  */
    1                  /* Task Processor - 0 or 1  */
  ); 
  
  xTaskCreatePinnedToCore
  (
    TarefaLeAjustes,  /* Task or Role  */
    "TarefaLeAjustes",/* Task or Role Name  */
    10000,            /* Stack Size  */
    NULL,             /* Input Parameter  */
    2,                /* Task Priority  */
    NULL,             /* Task Identifier  */
    0                 /* Task Processor - 0 or 1 */
  ); 

  xTaskCreatePinnedToCore
  (
    TarefaDisplayLCD,  /* Task or Role  */
    "TarefaDisplayLCD",/* Task or Role Name  */
    10000,             /* Stack Size  */
    NULL,              /* Input Parameter  */
    2,                 /* Task Priority  */
    NULL,              /* Task Identifier  */
    1                  /* Task Processor - 0 or 1  */
  ); 

  xTaskCreatePinnedToCore
  (
    TarefaAtualizaLeds,  /* Task or Role  */
    "TarefaAtualizaLeds",/* Task or Role Name  */
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
 
void TarefaLeAjustes( void * parameter)
{
  while(menu == 0)
  { 
    delay(300);
    Serial.println("Task 1 - Read settings.");
    int aux_ajusteRed = analogRead(pRed);       //Red LED adjustment
    int aux_ajusteGreen = analogRead(pGreen);   //Green LED adjustment
    int aux_ajusteBlue = analogRead(pBlue);     //Blue LED adjustment

    if((aux_ajusteRed<=0)||(aux_ajusteRed>10000))aux_ajusteRed=0;
    else if(aux_ajusteRed>=4095)aux_ajusteRed=4095;
    if((aux_ajusteGreen<=0)||(aux_ajusteGreen>10000))aux_ajusteGreen=0;
    else if(aux_ajusteGreen>=4095)aux_ajusteGreen=4095;
    if((aux_ajusteBlue<=0)||(aux_ajusteBlue>10000))aux_ajusteBlue=0;
    else if(aux_ajusteBlue>=4095)aux_ajusteBlue=4095;
        
    ajusteRed = map(aux_ajusteRed, 4095, 0, 0, 255);       //set the range to 0-255
    if((ajusteRed<=0)||(ajusteRed>10000))ajusteRed=0;
    else if(ajusteRed>=255)ajusteRed=255;
    
    ajusteGreen = map(aux_ajusteGreen, 4095, 0, 0, 255);   //set the range to 0-255
    if((ajusteGreen<=0)||(ajusteGreen>10000))ajusteGreen=0;
    else if(ajusteGreen>=255)ajusteGreen=255;
    
    ajusteBlue = map(aux_ajusteBlue, 4095, 0, 0, 255);     //set the range to 0-255
    if((ajusteBlue<=0)||(ajusteBlue>10000))ajusteBlue=0;
    else if(ajusteBlue>=255)ajusteBlue=255;
    
      if(touchRead(pinoTouchMore) < 20)
      {
        Serial.print("Time: "); 
        Serial.println(tempo); 
        delay(100);
        if(tempo<1440)tempo++;
        delay(400);
       if(touchRead(pinoTouchMore) < 20)
        {
           if(tempo<1430)tempo=tempo+9;
        }
     }
      if(touchRead(pinoTouchLess) < 20)
      {
        Serial.print("Time: "); 
        Serial.println(tempo); 
        delay(100);
        if(tempo>1)tempo--;
        delay(400);
      if(touchRead(pinoTouchLess) < 20)
      {
        if(tempo>=10)tempo=tempo-9;
        if(tempo>10000)tempo=0;      
      }
     }
      if(touchRead(pinoTouchStart) < 10)
      {
        delay(300);
        if(touchRead(pinoTouchStart) < 10){
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

void TarefaDisplayLCD( void * parameter)
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
      lcd.print(ajusteRed);
      
      //delay(10);
      lcd.setCursor(6,0);
      lcd.print("G");
      lcd.print(ajusteGreen);
      
      //delay(10);
      lcd.setCursor(12,0);
      lcd.print("B");
      lcd.print(ajusteBlue);      
      
      lcd.setCursor(0,1);
      lcd.print("Time:");
      lcd.print(tempo);
      lcd.print("min");
    }
    else if(menu==1) //display operation screen
    {      
      lcd.begin();
      //lcd.clear();
      //delay(10);
      
      //delay(10);
      lcd.setCursor(0,0);
      lcd.print(leitura_lux);
      lcd.print(" lx ");
      lcd.print("T:");
      lcd.print(tempo_decorrido);
      lcd.print("'");

      lcd.setCursor(0,1);
      lcd.print(leitura_joule);
      lcd.print(" J/cm2");
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
      lcd.print("Av:");
      lcd.print(leitura_joule_final);
      lcd.print("J/cm2");
      
      lcd.setCursor(0,1);
      lcd.print("Acc:");
      double aux = leitura_joule_final*tempo_inicial*60/1000;
      lcd.print(aux);
      lcd.print("kJ/cm2");

      while(1){delay(1000);}
    }

    flagI2C = false;
  }
  /* Delete the current task */ 
  vTaskDelete( NULL ); 
}

void TarefaLeLux( void * parameter)
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
      
    leitura_lux = event.light;                 //measurement in lux or lumens/m^2    
    leitura_joule = double(leitura_lux)/10000; //now we have lumens/com^2
    leitura_joule = leitura_joule *683;        //now we have Watts/cm^2 or J/cm^2.s
    Serial.print(leitura_joule);
    Serial.println(" J/cm^2");
    
    if(leitura_joule > 5)leitura_joule_final = leitura_joule; //if it is still on, it assigns the reading to the final calculation
    
    flagI2C = false;
    delay(1000);
    if(menu==2)
    {
      while(1){delay(1000);}
    }
  }
  vTaskDelete(NULL);
}

void TarefaContaTempo( void * parameter)
{
  while(1)
  {
    Serial.println("Task 4 - Time Account");
    if(menu==0) //menu 0 - configures colors and time
    {
      tempo_decorrido=tempo;
      tempo_inicial = tempo;
      delay(500);
    }
    else if (menu==1) //menu 1 - countdown timer operation
    {
      delay(60000);
      tempo_decorrido--;
      if(tempo_decorrido<=0)
      {tempo_decorrido=0;
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

void TarefaAtualizaLeds( void * parameter)
{
  while(1)
  {
    Serial.println("Task 5 - LEDs");
    if(menu==0) //menu 0 - configures colors and time
    {
       pixels.clear(); //turn off the LEDs
       /*for(int i=0; i<NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(ajusteRed, ajusteGreen, ajusteBlue));    
        pixels.show();   // update the color of the leds
        delay(30);*/
        operaRed = ajusteRed;
        operaGreen = ajusteGreen;
        operaBlue = ajusteBlue;
      //}
      delay(200);
    }
    else if(menu == 1) //menu 1 - operation with fixed LED color
    {
       for(int i=0; i<NUMPIXELS; i++)
       {
        pixels.setPixelColor(i, pixels.Color(operaBlue, operaRed, operaGreen));    
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
