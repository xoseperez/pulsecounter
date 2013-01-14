/*

  Smartmeter pulse counter
  Copyright (C) 2012 by Xose Pérez <xose dot perez at gmail dot com>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <LowPower.h>

// ===========================================
// Configuration
// ===========================================

//#define DEBUG

#define LDR_INTERRUPT 0
#define XBEE_INTERRUPT 1
#define LDR_INTERRUPT_PIN 2
#define XBEE_INTERRUPT_PIN 3
#define BATT_PIN 3
#define XBEE_SLEEP_PIN 4
#define DEBUG_PIN 13

#define VOLTAGE_FACTOR 4.06
#define VOLTAGE_REFERENCE 1100
#define PULSES_PER_WATTHOUR 4
#define XBEE_ASSOCIATION_TIME 2000
#define XBEE_AFTER_SEND_DELAY 20
#define SEND_BATTERY_EVERY_N_TRANSMISSIONS 10
#define REPORTS_PER_HOUR 60

// ===========================================
// Globals
// ===========================================

volatile int pulses = 0;
int transmission_id = 0;
volatile int ready_to_send = false;

// ===========================================
// Interrupt routines
// ===========================================

void pulse() {
    ++pulses;
}

void xbee_awake() {
    ready_to_send = true;
}

// ===========================================
// Methods
// ===========================================

void xbeeSleep() {
    digitalWrite(XBEE_SLEEP_PIN, HIGH);
}

void xbeeWake() {
    digitalWrite(XBEE_SLEEP_PIN, LOW);
}

void sendBattery() {

    // Calculating the battery voltage
    int reading = analogRead(BATT_PIN);
    int mV = map(reading, 0, 1023, 0, VOLTAGE_REFERENCE) * VOLTAGE_FACTOR;

    // Sending data
    Serial.print("battery:");
    Serial.println(mV);
    delay(XBEE_AFTER_SEND_DELAY);

}

void sendPower() {

    // Calculating the watts to report
    int gather = pulses;
    int watt = REPORTS_PER_HOUR * gather / PULSES_PER_WATTHOUR;

    // Sending data
    Serial.print("power:");
    Serial.println(watt);
    delay(XBEE_AFTER_SEND_DELAY);

    // Lowering the pulse count by the number of pulses reported
    pulses -= gather;

}

void sendAll() {

    sendPower();
    if (++transmission_id % SEND_BATTERY_EVERY_N_TRANSMISSIONS == 0) {
        sendBattery();
    }

}

void sendStatus() {
    xbeeWake();
    for (byte i=0; i<5; i++) {
        Serial.println("status:1");
        delay(XBEE_ASSOCIATION_TIME);
    }
    xbeeSleep();
}

void setup() {

    pinMode(LDR_INTERRUPT_PIN, INPUT);
    pinMode(XBEE_INTERRUPT_PIN, INPUT);
    pinMode(XBEE_SLEEP_PIN, OUTPUT);

    #ifdef DEBUG
        pinMode(DEBUG_PIN, OUTPUT);
        digitalWrite(DEBUG_PIN, LOW);
    #endif
    Serial.begin(9600);

    // Using the ADC against internal 1V1 reference for battery monitoring
    analogReference(INTERNAL);

    // Send welcome message
    sendStatus();

    // Allow pulse to trigger interrupt on rising
    attachInterrupt(LDR_INTERRUPT, pulse, RISING);

    // Enable interrupt on xbee awaking
    attachInterrupt(XBEE_INTERRUPT, xbee_awake, FALLING);

}

void loop() {

    // Enter power down state with ADC and BOD module disabled
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);

    // Check if I have to send a report
    if (ready_to_send) {
        ready_to_send = false;

        #ifdef DEBUG
            digitalWrite(DEBUG_PIN, HIGH);
        #endif
        sendAll();
        #ifdef DEBUG
            digitalWrite(DEBUG_PIN, LOW);
        #endif

    }

}
