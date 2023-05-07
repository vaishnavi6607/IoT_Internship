/*************************************************************
Title         :   Home automation using blynk
Description   :   To control light's brigntness with brightness,monitor temperature , monitor water level in the tank through blynk app
Pheripherals  :   Arduino UNO , Temperature system, LED, LDR module, Serial Tank, Blynk cloud, Blynk App.
 *************************************************************/

// Template ID, Device Name and Auth Token are provided by the Blynk.Cloud
// See the Device Info tab, or Template settings
#define BLYNK_TEMPLATE_ID "TMPL3TtalORf0"
#define BLYNK_TEMPLATE_NAME "Home automation"
#define BLYNK_AUTH_TOKEN "RzNJtAYUjo0JPOGXjrF7X4aNwbpcBjPj"



// Comment this out to disable prints
//#define BLYNK_PRINT Serial

//include library for blynk
#include <SPI.h>
#include <Ethernet.h>
#include <BlynkSimpleEthernet.h>
//include library  for CLCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "main.h"
#include "temperature_system.h"
#include "ldr.h"
#include "serial_tank.h"

char auth[] = BLYNK_AUTH_TOKEN;
bool heater_sw, cooler_sw;
bool inlet_sw, outlet_sw;
unsigned int tank_volume;

BlynkTimer timer;

LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

// This function is called every time the Virtual Pin 0 state changes
/*To turn ON and OFF cooler based virtual PIN value*/
BLYNK_WRITE(COOLER_V_PIN) {
  //to read the value on virtual pin connected to cooler
  cooler_sw = param.asInt();
  if (cooler_sw) {
    cooler_control(ON);
    lcd.setCursor(7, 0);
    lcd.print("CO_LR ON ");
  } else {
    cooler_control(OFF);
    lcd.setCursor(7, 0);
    lcd.print("CO_LR OFF");
  }
}
/*To turn ON and OFF heater based virtual PIN value & printing notification on the CLCD*/
BLYNK_WRITE(HEATER_V_PIN) {
  heater_sw = param.asInt();
  if (heater_sw) {
    heater_control(ON);
    lcd.setCursor(7, 0);
    lcd.print("HT_R ON  ");
  } else {
    heater_control(OFF);
    lcd.setCursor(7, 0);
    lcd.print("HT_R OFF ");
  }
}

// /*To turn ON and OFF inlet vale based virtual PIN value*/
BLYNK_WRITE(INLET_V_PIN) {
  //to read the value on inlet pin
  inlet_sw = param.asInt();
  if (inlet_sw) {
    enable_inlet();
    lcd.setCursor(7, 1);
    lcd.print("IN_FL_ON ");
  } else {
    disable_inlet();
    lcd.setCursor(7, 1);
    lcd.print("IN_FL_OFF");
  }
}

/*To turn ON and OFF outlet value based virtual switch value*/
BLYNK_WRITE(OUTLET_V_PIN) {
  outlet_sw = param.asInt();
  if (outlet_sw) {
    enable_outlet();
    lcd.setCursor(7, 1);
    lcd.print("OUT_FL_ON ");
  } else {
    disable_outlet();
    lcd.setCursor(7, 1);
    lcd.print("OUT_FL_OFF");
  }
}
// /* To display temperature and water volume as gauge on the Blynk App*/
void update_temperature_reading() {
  // You can send any value at any time.
  // Please don't send more that 10 values per second.

  Blynk.virtualWrite(TEMPERATURE_GAUGE, read_temperature());

  Blynk.virtualWrite(WATER_VOL_GAUGE, volume());
}

/*To turn off the heater if the temperature raises above 35 deg C*/
void handle_temp(void) {
  //to read temperature and compare with 35 and check if heater is ON
  if (read_temperature() > float(35) && heater_sw) {

    heater_sw = 0;
    //turn of heater
    heater_control(OFF);
    //to print heater status on dashboard
    lcd.setCursor(7, 0);
    lcd.print("HT_R OFF ");

    //to print notification on the Blynk App
    Blynk.virtualWrite(BLYNK_TERMINAL_V_PIN, "temperature is above 35 degrees celcius \n");
    Blynk.virtualWrite(BLYNK_TERMINAL_V_PIN, "turning OFF the heater\n");

    //to reflect on the button widget on heater pin
    Blynk.virtualWrite(HEATER_V_PIN, 0);
  }
}

// /*To control water volume above 2000ltrs*/
void handle_tank(void)
{
//to check if the volume of the water is less than 2000 ltrs & inlet is off ,enable inlet valve
if((tank_volume <2000) &&  (inlet_sw == 0))
{
  enable_inlet();
  inlet_sw = 1;
  /*to print status on the dashboard*/
  lcd.setCursor(7, 1);
  lcd.print("IN_FL_ON ");

//to print notification on the Blynk App
    Blynk.virtualWrite(BLYNK_TERMINAL_V_PIN, "Volume of water in the tank is less than 2000 turning ON the inlet Valve\n");
    Blynk.virtualWrite(BLYNK_TERMINAL_V_PIN, "turning ON inlet Valve\n");

  //to reflect on the button widget on heater pin
   Blynk.virtualWrite(INLET_V_PIN, 1);
}

/*If volume of tank is full & inlet valve is ON then turn OFF*/
  if((tank_volume == 3000) &&  (inlet_sw == 1))
{
  disable_inlet();
  inlet_sw = 0;
  /*to print status on the dashboard*/
  lcd.setCursor(7, 1);
  lcd.print("IN_FL_OFF");

//to print notification on the Blynk App
    Blynk.virtualWrite(BLYNK_TERMINAL_V_PIN, "Tank is full turning OFF inlet valve\n");
    Blynk.virtualWrite(BLYNK_TERMINAL_V_PIN, "turning OFF inlet Valve\n");

  //to reflect on the button widget on heater pin
   Blynk.virtualWrite(INLET_V_PIN, 0);
}


}


void setup(void) {
  /*To config garden lights as output*/
  init_ldr();

  /*Initialize the LCD*/
  lcd.init();
  /*turn the backlight */
  lcd.backlight();
  /*clear the clcd*/
  lcd.clear();
  /*cursor to the home */
  lcd.home();

  /*To display string*/
  lcd.setCursor(0, 0);
  lcd.print("T=");
  /*To display set cursor at second line first position*/
  lcd.setCursor(0, 1);
  lcd.print("V=");
  /*to connect arduino to the blynk cloud*/
  Blynk.begin(auth);

  /*to initialize temperature system */
  init_temperature_system();

  /*TO initialize the serial tank*/
  init_serial_tank();

  /*to update temperature to blynk app for every 0.5 sec*/
  timer.setInterval(500L, update_temperature_reading);
}

void loop(void) {
  Blynk.run();
  /*keep timer running*/
  timer.run();


  /*To control brightness of led*/
  brightness_control();
  //to read the temperature and display it on the CLCD
  String temperature;
  temperature = String(read_temperature(), 2);
  lcd.setCursor(2, 0);
  lcd.print(temperature);

  //to read the volume of water and display it on the CLCD
  tank_volume = volume();
  lcd.setCursor(2, 1);
  lcd.print(tank_volume);

//to maintain volume of water for 2000
  handle_tank();
  //to maintain threshold temeprature
  handle_temp();
}


/*Include the serial remote tank from tools and connect to COM2*/