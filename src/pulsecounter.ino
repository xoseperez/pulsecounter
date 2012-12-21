/*
  Smartmeter pulse counter
*/

#include <LowPower.h>

// ===========================================
// Configuration
// ===========================================

//#define DEBUG

// Define the Xbee mode (one or the other, not both)
//#define PIN_SLEEP_MODE
#define CYCLE_SLEEP_MODE

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
#define XBEE_WAKEUP_TIME 20
#define XBEE_ASSOCIATION_TIME 2000
#define XBEE_AFTER_SEND_DELAY 20
#define SEND_BATTERY_EVERY_N_TRANSMISSIONS 10

// Parameters for CYCLE_SLEEP_MODE
#define REPORTS_PER_HOUR 60

// Parameters for PIN_SLEEP_MODE
#define SEND_WATTS_EVERY_N_PULSES 20

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

#ifdef CYCLE_SLEEP_MODE
    void xbee_awake() {
        ready_to_send = true;
    }
#endif

// ===========================================
// Methods
// ===========================================

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
    Serial.print("battery:");
    Serial.println(mV);
    delay(XBEE_AFTER_SEND_DELAY);

}

#ifdef PIN_SLEEP_MODE
    void sendPower() {

        // Calculating the watts to report
        int gather = pulses - pulses % SEND_WATTS_EVERY_N_PULSES;
        int wh = gather / PULSES_PER_WATTHOUR;

        // Sending data
        Serial.print("energy:");
        Serial.println(wh);
        delay(XBEE_AFTER_SEND_DELAY);

        // Lowering the pulse count by the number of pulses reported
        pulses -= gather;

    }
#endif

#ifdef CYCLE_SLEEP_MODE
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
#endif

void sendAll() {

    #ifdef PIN_SLEEP_MODE
        // Awaking radio
        xbeeWake();
    #endif

    sendPower();
    if (++transmission_id % SEND_BATTERY_EVERY_N_TRANSMISSIONS == 0) {
        sendBattery();
    }

    #ifdef PIN_SLEEP_MODE
        // Turning radio to sleep
        // It should wait for the current transmission to finish
        xbeeSleep();
    #endif

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

    #ifdef CYCLE_SLEEP_MODE
        // Enable interrupt on xbee awaking
        attachInterrupt(XBEE_INTERRUPT, xbee_awake, FALLING);
    #endif

}

void loop() {

    // Enter power down state with ADC and BOD module disabled
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);

    #ifdef PIN_SLEEP_MODE
        // I am sending every SEND_WATTS_EVERY_N_PULSES pulses
        // If the frequency is too high the module operation
        // will space the transmissions
        ready_to_send = (pulses >= SEND_WATTS_EVERY_N_PULSES and pulses % SEND_WATTS_EVERY_N_PULSES == 0);
    #endif

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
