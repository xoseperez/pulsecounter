/*
  Smartmeter pulse counter
*/

#include <LowPower.h>
#include <SoftwareSerial.h>

// ===========================================
// Configuration
// ===========================================

#define DEBUG

// Define the Xbee mode (one or the other, not both)
#define PIN_SLEEP_MODE
//#define CYCLE_SLEEP_MODE

#define LDR_INTERRUPT 0
#define XBEE_INTERRUPT 1
#define LDR_INTERRUPT_PIN 2
#define XBEE_INTERRUPT_PIN 3
#define BATT_PIN 3
#define XBEE_SLEEP_PIN 4
#define XBEE_TX_PIN 5
#define XBEE_RX_PIN 6

#define PULSE_BOUNCE_DELAY 100
#define VOLTAGE_FACTOR 4.06
#define VOLTAGE_REFERENCE 1100
#define PULSES_PER_WATTHOUR 4

#define REPORTS_PER_HOUR 60
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
volatile int ready_to_send = false;
SoftwareSerial Xbee(XBEE_RX_PIN, XBEE_TX_PIN);

// ===========================================
// Methods
// ===========================================

void pulse() {
    pulses++;
}

#ifdef PIN_SLEEP_MODE
    void xbeeSleep() {
        digitalWrite(XBEE_SLEEP_PIN, HIGH);
    }

    void xbeeWake() {
        digitalWrite(XBEE_SLEEP_PIN, LOW);
        delay(XBEE_WAKEUP_TIME);
    }
#endif

#ifdef CYCLE_SLEEP_MODE
    void xbee_awake() {
        ready_to_send = true;
    }
#endif

void sendBattery() {

    // Calculating the battery voltage
    int reading = analogRead(BATT_PIN);
    int mV = map(reading, 0, 1023, 0, VOLTAGE_REFERENCE) * VOLTAGE_FACTOR;

    // Sending data
    Xbee.print("BATT:");
    Xbee.println(mV);

}

#ifdef PIN_SLEEP_MODE
    void sendPower() {

        // Calculating the watts to report
        int gather = pulses - pulses % SEND_WATTS_EVERY_N_PULSES;
        int wh = gather / PULSES_PER_WATTHOUR;

        // Sending data
        Xbee.print("WH:");
        Xbee.println(wh);

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
        Xbee.print("W:");
        Xbee.println(watt);

        // Lowering the pulse count by the number of pulses reported
        pulses -= gather;

    }
#endif

void sendTID() {

    // Update TID
    ++transmission_id;

    // Sending data
    Xbee.print("TID:");
    Xbee.println(transmission_id);

}

void sendAll() {

    #ifdef PIN_SLEEP_MODE
        // Awaking radio
        xbeeWake();
    #endif

    sendTID();
    sendPower();
    if (transmission_id % SEND_BATTERY_EVERY_N_TRANSMISSIONS == 0) {
        sendBattery();
    }

    #ifdef PIN_SLEEP_MODE
        // Turning radio to sleep
        xbeeSleep();
    #endif

}

void setup() {

    pinMode(LDR_INTERRUPT_PIN, INPUT);
    pinMode(XBEE_INTERRUPT_PIN, INPUT);
    pinMode(BATT_PIN, INPUT);
    pinMode(XBEE_SLEEP_PIN, OUTPUT);

    #ifdef PIN_SLEEP_MODE
        // Turn on radio and allow 1 second to link to coordinator
        xbeeWake();
        delay(1000);
    #endif

    #ifdef DEBUG
       Serial.begin(9600);
    #endif
    Xbee.begin(9600);

    // Using the ADC against internal 1V1 reference for battery monitoring
    analogReference(INTERNAL);

    // Send HELLO
    // Don't know what happens if the radio is off when sending this,
    // probably the message gets lost...
    Xbee.println("STATUS:1");
    DEBUG_PRINTLN("HELLO");

    #ifdef PIN_SLEEP_MODE
        // Sleep radio
        xbeeSleep();
    #endif

    // Allow pulse to trigger interrupt on rising
    attachInterrupt(LDR_INTERRUPT, pulse, RISING);

    #ifdef CYCLE_SLEEP_MODE
        // Enable interrupt on xbee awaking
        attachInterrupt(XBEE_INTERRUPT, xbee_awake, FALLING);
    #endif

}

void loop() {

    #ifdef PIN_SLEEP_MODE
        // I am sending every SEND_WATTS_EVERY_N_PULSES pulses
        // If the frequency is too high the module operation
        // will space the transmissions
        ready_to_send = (pulses >= SEND_WATTS_EVERY_N_PULSES and pulses % SEND_WATTS_EVERY_N_PULSES == 0);
    #endif

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
