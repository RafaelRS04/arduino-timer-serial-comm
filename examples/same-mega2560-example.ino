#define USE_COMMS_RECEIVER
#define USE_COMMS_TRANSMITTER
#define USE_SERIAL_LOG
#define SERIAL_RX_NO 3
#define SERIAL_TX_NO 3
#define ITIMER_RX_NO 4
#define ITIMER_TX_NO 5
#include "Comms.hpp"

#define INPUT_BUFFER_SIZE COMMS_BUFFER_SIZE
#define BAUD_RATE 115200

char input[INPUT_BUFFER_SIZE];
int index = 0;

void setup() {
    Serial.begin(9600);
    beginCommsSerials(DEFAULT_BAUD_RATE);
    SerialReceiver.begin(RX_FREQUENCY(BAUD_RATE));
    SerialTransmitter.begin(TX_FREQUENCY(BAUD_RATE));
}

void loop() {
    /* Read input and send using Serial Transmitter */
    if(Serial.available() > 0) {
        if(Serial.peek() == '\n') {
            if(SerialTransmitter.isAvailable()) {
                Serial.read();
                input[index] = '\0';
                SerialTransmitter.sendMessage(input);
                index = 0;
            }
        } else if(index < INPUT_BUFFER_SIZE-1) {
            input[index++] = Serial.read();
        } else {
            if(SerialTransmitter.isAvailable()) {
                input[index] = '\0';
                SerialTransmitter.sendMessage(input);
                index = 0;
            }
        }
    }

    /* Receive messages with Serial Receiver */
    if(SerialReceiver.hasMessage()) {
        const char *msg = SerialReceiver.getMessage();
        Serial.print("Received: [");
        Serial.print(msg);
        Serial.println("]");
        SerialReceiver.resume();
    }
}