#include <FreeRTOS.h>
#include <task.h>

// #include "Arduino.h"
#include "ap3_initialization.h"

#include <Arduino.h>
#include <SparkFun_ATECCX08a_Arduino_Library.h> //Click here to get the library: http://librarymanager/All#SparkFun_ATECCX08a
#include <Wire.h>

// Weak empty variant initialization function.
// May be redefined by variant files.
extern "C" void ap3_variant_init(void) __attribute__((weak));
extern "C" void ap3_variant_init(void) {}

// Initialize C library
extern "C" void __libc_init_array(void);
extern "C" void _init(void)
{
  // Empty definition to resolve linker error within '__libc_init_array'
}

void *__dso_handle; // providing the __dso_handle symbol manually. Todo: more research for the *correct* solution

ATECCX08A atecc;

static void printInfo()
{
  // Read all 128 bytes of Configuration Zone
  // These will be stored in an array within the instance named: atecc.configZone[128]
  atecc.readConfigZone(false); // Debug argument false (OFF)

  // Print useful information from configuration zone data
  Serial.println();

  Serial.print("Serial Number: \t");
  for (int i = 0 ; i < 9 ; i++)
  {
    if ((atecc.serialNumber[i] >> 4) == 0) Serial.print("0"); // print preceeding high nibble if it's zero
    Serial.print(atecc.serialNumber[i], HEX);
  }
  Serial.println();

  Serial.print("Rev Number: \t");
  for (int i = 0 ; i < 4 ; i++)
  {
    if ((atecc.revisionNumber[i] >> 4) == 0) Serial.print("0"); // print preceeding high nibble if it's zero
    Serial.print(atecc.revisionNumber[i], HEX);
  }
  Serial.println();

  Serial.print("Config Zone: \t");
  if (atecc.configLockStatus) Serial.println("Locked");
  else Serial.println("NOT Locked");

  Serial.print("Data/OTP Zone: \t");
  if (atecc.dataOTPLockStatus) Serial.println("Locked");
  else Serial.println("NOT Locked");

  Serial.print("Data Slot 0: \t");
  if (atecc.slot0LockStatus) Serial.println("Locked");
  else Serial.println("NOT Locked");

  Serial.println();

  // if everything is locked up, then configuration is complete, so let's print the public key
  if (atecc.configLockStatus && atecc.dataOTPLockStatus && atecc.slot0LockStatus) 
  {
    if(atecc.generatePublicKey() == false)
    {
      Serial.println("Failure to generate This device's Public Key");
      Serial.println();
    }
  }
}

void setup()
{
  Wire.begin();
  Serial.begin(115200);
  if (atecc.begin() == true)
  {
    Serial.println("Successful wakeUp(). I2C connections are good.");
  }
  else
  {
    Serial.println("Device not found. Check wiring.");
    while (1); // stall out forever
  }

  printInfo(); // see function below for library calls and data handling
}

static TaskHandle_t xSetupTask;

static void setup_task(void *pvParameters)
{
	Serial.println("Inside serial task");

	while (1) ;
}

extern "C" int main(void)
{
  ap3_init();
  __libc_init_array();

  ap3_variant_init();

  // ap3_adc_setup();

  setup();

  xTaskCreate(setup_task, "Setup", 128, 0, 3, &xSetupTask);

  vTaskStartScheduler();

  return 0;
}
