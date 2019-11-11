/*
  SigFox Event Trigger tutorial

  This sketch demonstrates the usage of a MKRFox1200
  to build a battery-powered alarm sensor with email notifications

  A couple of sensors (normally open) should we wired between pins 1 and 2 and GND.

  This example code is in the public domain.
*/

#include <SigFox.h>
#include <ArduinoLowPower.h>

// Set debug to false to enable continuous mode
// and disable serial prints
int debug = true;

volatile int alarm_source = 0;

// Variables for battery SoC calculation
float voltage = 0;
int sensorValue = 0;
uint8_t battery_percentage = 0;

void setup() {

  if (debug == true) {

    // We are using Serial1 instead than Serial because we are going in standby
    // and the USB port could get confused during wakeup. To read the debug prints,
    // connect pins 13-14 (TX-RX) to a 3.3V USB-to-serial converter

    Serial1.begin(115200);
    while (!Serial1) {}
  }

  if (!SigFox.begin()) {
    //something is really wrong, try rebooting
    reboot();
  }

  //Send module to standby until we need to send a message
  SigFox.end();

  if (debug == true) {
    // Enable debug prints and LED indication if we are testing
    SigFox.debug();
  }

  // attach pin 1 to a switch and enable the interrupt on voltage rising event

  pinMode(1, INPUT_PULLUP);
  LowPower.attachInterruptWakeup(1, alarmEvent1, RISING);
}


void alarmEvent1() {
  alarm_source = 1;

  // battery SoC calculation
  analogReadResolution(10);
  analogReference(AR_INTERNAL1V0);

  sensorValue = analogRead(ADC_BATTERY);
  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 4.3V):
  // if you're using a 3.7 V Lithium battery pack - adjust the 3 to 3.7 in the following formulas
  voltage = sensorValue * (3 / 1023.0);

  //battery percentage calculation
  // 2.2 is the cutoff voltage so adjust it if you're using a 3.7 V battery pack

  battery_percentage = ((voltage - 2.2) / (3 - 2.2)) * 100;

  analogReference(AR_DEFAULT);
}


void loop()
{
  // Sleep until an event is recognized
  LowPower.sleep();

  // if we get here it means that an event was received
  SigFox.begin();

  if (debug == true) {
    Serial1.println("Alarm event on sensor " + String(alarm_source));
  }
  delay(100);

  // sending the payload to Sigfox server
  SigFox.beginPacket();
  SigFox.write(battery_percentage);
  int ret = SigFox.endPacket();

  // shut down module, back to standby
  SigFox.end();

  if (debug == true) {
    if (ret > 0) {
      Serial1.println("No transmission");
    } else {
      Serial1.println("Transmission ok");
    }

    Serial1.println(SigFox.status(SIGFOX));
    Serial1.println(SigFox.status(ATMEL));

  }

}


void reboot() {
  NVIC_SystemReset();
  while (1);
}
