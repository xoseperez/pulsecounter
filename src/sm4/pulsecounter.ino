/*
  Smartmeter pulse counter
*/

#include <LowPower.h>
#include <SoftwareSerial.h>

// ===========================================
// Configuration
// ===========================================

#define DEBUG

#define LDR_INTERRUPT 0
#define XBEE_INTERRUPT 1
#define LDR_INTERRUPT_PIN 2
#define XBEE_INTERRUPT_PIN 3
#define BATT_PIN 3
#define XBEE_RX_PIN 5
#define XBEE_TX_PIN 6

#define PULSE_BOUNCE_DELAY 100
#define VOLTAGE_FACTOR 4.06
#define VOLTAGE_REFERENCE 1100
#define PULSES_PER_WATTHOUR 4

#define REPORTS_PER_HOUR 360

// ===========================================
// Debugging
// ===========================================

#ifdef DEBUG
    #define DEBUG_PRINT(s) Serial.print(s)
    #define DEBUG_PRINTLN(s) Serial.println(s)
#else
    #define DEBUG_PRINT(s)
    #define DEBUG_PRINTLN(s)
#endif

// ===========================================
// Globals
// ===========================================

volatile int pulses = 0;
int transmission_id = 0;
volatile int ready_to_send = false;
SoftwareSerial Xbee(XBEE_RX_PIN, XBEE_TX_PIN);

// ===========================================
// Methods
// ===========================================

void pulse() {
    pulses++;
}

void xbee_awake() {
    ready_to_send = true;
}

void sendBattery() {

    // Calculating the battery voltage
    int reading = analogRead(BATT_PIN);
    int mV = map(reading, 0, 1023, 0, VOLTAGE_REFERENCE) * VOLTAGE_FACTOR;

    // Sending data
    Xbee.print("BATT:");
    Xbee.println(mV);

}

void sendWatts() {

    // Calculating the watts to report
    int gather = pulses;
    int watt = REPORTS_PER_HOUR * gather / PULSES_PER_WATTHOUR;

    // Sending data
    Xbee.print("WATT:");
    Xbee.println(watt);

    // Lowering the pulse count by the number of pulses reported
    pulses -= gather;

}

void sendTID() {

    // Update TID
    ++transmission_id;

    // Sending data
    Xbee.print("TID:");
    Xbee.println(transmission_id);

}

void sendAll() {

    sendTID();
    sendWatts();
    sendBattery();

}

void setup() {

    pinMode(LDR_INTERRUPT_PIN, INPUT);
    pinMode(XBEE_INTERRUPT_PIN, INPUT);
    pinMode(BATT_PIN, INPUT);

    #ifdef DEBUG
       Serial.begin(9600);
    #endif
    Xbee.begin(9600);

    // Using the ADC against internal 1V1 reference for battery monitoring
    analogReference(INTERNAL);

    // Allow pulse to trigger interrupt on rising
    attachInterrupt(LDR_INTERRUPT, pulse, RISING);

    // Enable interrupt on xbee awaking
    attachInterrupt(XBEE_INTERRUPT, xbee_awake, FALLING);

}

void loop() {

    // Check if I have to send a report
    if (ready_to_send) {

        ready_to_send = false;
        DEBUG_PRINTLN("SENDING");
        sendAll();

    // If there is nothing to send then
    // enter power down state with ADC and BOD module disabled
    } else {

        DEBUG_PRINTLN("PULSE");
        #ifdef DEBUG
            delay(100);
        #endif
        LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);

    }

}
