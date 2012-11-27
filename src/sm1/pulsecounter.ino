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
#define LDR_INTERRUPT_PIN 2
#define BATT_PIN 3
#define XBEE_SLEEP_PIN 4
#define XBEE_TX_PIN 5
#define XBEE_RX_PIN 6

#define PULSE_BOUNCE_DELAY 100
#define VOLTAGE_FACTOR 4.06
#define VOLTAGE_REFERENCE 1100
#define PULSES_PER_WATTHOUR 4

#define SEND_WATTS_EVERY_N_PULSES 20
#define SEND_BATTERY_EVERY_N_TRANSMISSIONS 10
#define XBEE_WAKEUP_TIME 20

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
SoftwareSerial Xbee(XBEE_RX_PIN, XBEE_TX_PIN);

// ===========================================
// Methods
// ===========================================

void pulse() {
    pulses++;
}

void xbeeSleep() {
    digitalWrite(XBEE_SLEEP_PIN, HIGH);
}

void xbeeWake() {
    digitalWrite(XBEE_SLEEP_PIN, LOW);
    delay(XBEE_WAKEUP_TIME);
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
    int gather = pulses - pulses % SEND_WATTS_EVERY_N_PULSES;
    int wh = gather / PULSES_PER_WATTHOUR;

    // Sending data
    Xbee.print("WH:");
    Xbee.println(wh);

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

    // Awaking radio
    xbeeWake();

    sendTID();
    sendWatts();
    if (transmission_id % SEND_BATTERY_EVERY_N_TRANSMISSIONS == 0) {
        sendBattery();
    }

    // Turning radios to sleep
    xbeeSleep();

}

void setup() {

    pinMode(LDR_INTERRUPT_PIN, INPUT);
    pinMode(BATT_PIN, INPUT);
    pinMode(XBEE_SLEEP_PIN, OUTPUT);

    // Turn on radio and allow 1 second to link to coordinator
    xbeeWake();
    delay(1000);

    #ifdef DEBUG
       Serial.begin(9600);
    #endif
    Xbee.begin(9600);

    // Using the ADC against internal 1V1 reference for battery monitoring
    analogReference(INTERNAL);

    // Send HELLO
    Xbee.println("STATUS:1");
    DEBUG_PRINTLN("HELLO");

    // Sleep radio
    xbeeSleep();

    // Allow pulse to trigger interrupt on rising
    attachInterrupt(LDR_INTERRUPT, pulse, RISING);

}

void loop() {

    // Check if I have to send a report
    // I am sending every SEND_WATTS_EVERY_N_PULSES pulses
    // If the frequency is too high the module operation
    // will space the transmissions
    if (pulses >= SEND_WATTS_EVERY_N_PULSES and pulses % SEND_WATTS_EVERY_N_PULSES == 0) {

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
