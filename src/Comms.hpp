/******************************************************************************************
    MIT License

    Copyright (c) 2025 Rafael Rodrigues Sanches

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

******************************************************************************************/

#include <Arduino.h>

/* =========================== SERIALS AND TIMERS SAFEGUARDS =========================== */

#ifndef SERIAL_RX_NO
    #error Comms need SERIAL_RX macro to be defined
#endif /* SERIAL_RX */

#ifndef SERIAL_TX_NO
    #error Comms need SERIAL_TX macro to be defined
#endif /* SERIAL_TX */

#if SERIAL_RX_NO == 0 || SERIAL_TX_NO == 0
    #error Serial must be other than HardwareSerial0
#endif /* SERIAL ASSERTION */

#ifndef ITIMER_RX_NO
    #error Comms need ITIMER_RX macro to be defined
#endif /* ITIMER_RX */

#ifndef ITIMER_TX_NO
    #error Comms need ITIMER_TX macro to be defined
#endif /* ITIMER_TX */

#if ITIMER_RX_NO == 0 || ITIMER_TX_NO == 0
    #error ITimers must be other than Timer0
#endif /* ITIMER ASSERTION */

#if ITIMER_RX_NO == ITIMER_TX_NO
    #error ITIMER_RX and ITIMER_TX must be diferent
#endif /* ITIMER ASSERTION */

/* ================================ SERIALS AND TIMERS ================================= */

#if SERIAL_RX_NO == 1
    #define SERIAL_RX Serial1
#elif SERIAL_RX_NO == 2
    #define SERIAL_RX Serial2
#elif SERIAL_RX_NO == 3
    #define SERIAL_RX Serial3
#else
    #error Undefined SERIAL_RX_NO, use serials 1-3
#endif

#if SERIAL_TX_NO == 1
    #define SERIAL_TX Serial1
#elif SERIAL_TX_NO == 2
    #define SERIAL_TX Serial2
#elif SERIAL_TX_NO == 3
    #define SERIAL_TX Serial3
#else
    #error Undefined SERIAL_TX_NO, use serials 1-3
#endif

#if ITIMER_RX_NO == 1
    #define ITIMER_RX ITimer1
    #define USE_TIMER_1 true
#elif ITIMER_RX_NO == 2
    #define ITIMER_RX ITimer2
    #define USE_TIMER_2 true
#elif ITIMER_RX_NO == 3
    #define ITIMER_RX ITimer3
    #define USE_TIMER_3 true
#elif ITIMER_RX_NO == 4
    #define ITIMER_RX ITimer4
    #define USE_TIMER_4 true
#elif ITIMER_RX_NO == 5
    #define ITIMER_RX ITimer5
    #define USE_TIMER_5 true
#else
    #error Undefined ITIMER_RX_NO, use ITimers 1-5 (verify board compatibility)
#endif

#if ITIMER_TX_NO == 1
    #define ITIMER_TX ITimer1
    #define USE_TIMER_1 true
#elif ITIMER_TX_NO == 2
    #define ITIMER_TX ITimer2
    #define USE_TIMER_2 true
#elif ITIMER_TX_NO == 3
    #define ITIMER_TX ITimer3
    #define USE_TIMER_3 true
#elif ITIMER_TX_NO == 4
    #define ITIMER_TX ITimer4
    #define USE_TIMER_4 true
#elif ITIMER_TX_NO == 5
    #define ITIMER_TX ITimer5
    #define USE_TIMER_5 true
#else
    #error Undefined ITIMER_TX_NO, use ITimers 1-5 (verify board compatibility)
#endif

/* =============================== CONSTANTS AND MACROS ================================ */

#if SERIAL_RX_NO == SERIAL_TX_NO
    #define beginCommsSerials(baud) \
        do{ \
            SERIAL_RX.begin((baud)); \
        }while(0)
#else
    #define beginCommsSerials(baud) \
        do{ \
            SERIAL_RX.begin((baud)); \
            SERIAL_TX.begin((baud)); \
        }while(0)
#endif /* beginCommsSerials */

#define DEFAULT_BAUD_RATE 9600
#define SIZE_NOT_DEFINED -1
#define CSTRING_SIZE -1

/*
    Buffer size allways can be handled
    by a byte/uint8_t variable
*/
#define COMMS_BUFFER_SIZE 255

/* 
    Maximum transmission unit is less than
    buffer size due to null terminator 
*/
#define COMMS_MTU (COMMS_BUFFER_SIZE - 1)

/* Serial log control */
#ifdef USE_SERIAL_LOG
    #define LOG(x) Serial.println(F(x))
#else
    #define LOG(x)
#endif /* USE_SERIAL_LOG */

/* 
    Frequencies calculations
    The transmitter has a low frequency
    because it writes data in blocks
*/
#define RX_FREQUENCY(baud) (float)(baud>>3)
#define TX_FREQUENCY(baud) (float)(baud>>10)

#define TO_BYTE(exp) (byte)(exp)

/* ====================================== CLASSES ====================================== */

#ifdef USE_COMMS_RECEIVER

class Receiver {
public:
    Receiver();
    void begin(float freq=RX_FREQUENCY(DEFAULT_BAUD_RATE));
    boolean hasMessage();
    const byte *getMessage();
    void resume();

    /* Just for internal use */
    void handler();
    
    byte xor_key;

private:

    byte rx_buffer[COMMS_BUFFER_SIZE];
    volatile int rx_index;
    volatile int msg_size;
};

Receiver SerialReceiver;

#endif /* USE_COMMS_RECEIVER */

#ifdef USE_COMMS_TRANSMITTER

class Transmitter {
public:
    Transmitter();
    void begin(float freq=TX_FREQUENCY(DEFAULT_BAUD_RATE));
    boolean isAvailable();
    void sendMessage(const byte *buffer, int size=CSTRING_SIZE);

    /* Just for internal use */
    void handler();

    byte xor_key;

private:

    boolean send_header;
    byte tx_buffer[COMMS_BUFFER_SIZE];
    volatile int tx_index;
    volatile int msg_size;
};

Transmitter SerialTransmitter;

#endif /* USE_COMMS_TRANSMITTER */

/* +================================+ IMPLEMENTATION +=================================+ */

#include "TimerInterrupt.h"

/* ===================================== RECEIVER ====================================== */

#ifdef USE_COMMS_RECEIVER

void receiverHandler() {
    SerialReceiver.handler();
}

void Receiver::handler() {
    if(SERIAL_RX.available() > 0) {
        if(msg_size == SIZE_NOT_DEFINED) {
            msg_size = SERIAL_RX.read();
            rx_buffer[msg_size] = '\0';
            return;
        }

        if(rx_index < msg_size) {
            rx_buffer[rx_index] = TO_BYTE(SERIAL_RX.read()) ^ xor_key;

            if(rx_index + 1 == msg_size) {
                ITIMER_RX.pauseTimer();
            }

            rx_index++;
        }
    }
}

Receiver::Receiver() {
    xor_key = 0;
    rx_index = 0;
    msg_size = SIZE_NOT_DEFINED;
}

void Receiver::begin(float freq=RX_FREQUENCY(DEFAULT_BAUD_RATE)) {
    ITIMER_RX.init(); 

    if(ITIMER_RX.attachInterrupt(freq, receiverHandler)) {
        LOG("SERIAL LOG: Starting  ITimerRX");
    } else {
        LOG("SERIAL LOG ERROR: Interrupt not attached to ITimerRX");
    }
}

boolean Receiver::hasMessage() {
    return rx_index == msg_size;
}

const byte *Receiver::getMessage() {
    return rx_buffer;
}

void Receiver::resume() {
    rx_index = 0;
    msg_size = SIZE_NOT_DEFINED;
    ITIMER_RX.resumeTimer();
}

#endif /* USE_COMMS_RECEIVER */

/* ==================================== TRANSMITTER ==================================== */

#ifdef USE_COMMS_TRANSMITTER

void transmitterHandler() {
    SerialTransmitter.handler();
}

void Transmitter::handler() {
    if(msg_size == SIZE_NOT_DEFINED) return;

    int available = SERIAL_TX.availableForWrite();

    if(available <= 0) {
        LOG("SERIAL LOG ERROR: SERIAL_TX not available to write");
        return;
    }

    if(send_header) {
        SERIAL_TX.write(TO_BYTE(msg_size));
        send_header = false;
        return;
    }

    int nbytes = min(msg_size-tx_index, available);

    if(nbytes > 0) {
        byte *head = &SerialTransmitter.tx_buffer[tx_index];        
        SERIAL_TX.write(head, nbytes);
        tx_index += nbytes;
    } else {
        ITIMER_TX.pauseTimer();
    }
}

Transmitter::Transmitter() {
    xor_key = 0;
    send_header = true;
    tx_index = 0;
    msg_size = SIZE_NOT_DEFINED;
}

void Transmitter::begin(float freq=TX_FREQUENCY(DEFAULT_BAUD_RATE)) {
    ITIMER_TX.init(); 

    if(ITIMER_TX.attachInterrupt(freq, transmitterHandler)) {
        LOG("SERIAL LOG: Starting  ITimerTX");
    } else {
        LOG("SERIAL LOG ERROR: Interrupt not attached to ITimerTX");
    }


    ITIMER_TX.pauseTimer();
    
    /* Turn transmitter available */
    msg_size = 0;
}

boolean Transmitter::isAvailable() {
    return tx_index == msg_size;
}

void Transmitter::sendMessage(const byte *buffer, int size=CSTRING_SIZE) {
    if(buffer == NULL) {
        LOG("SERIAL LOG ERROR: NULL pointer passed to send message");
        return;
    }

    if(size <= CSTRING_SIZE) {
        size = strlen(buffer);
    }

    if(size == 0) return;

    ITIMER_TX.pauseTimer();

    msg_size = min(COMMS_MTU, size);
    tx_index = 0;
    send_header = true;

    for(int i = 0; i < msg_size; i++) {
        tx_buffer[i] = buffer[i] ^ xor_key;
    }

    tx_buffer[msg_size] = '\0';
    ITIMER_TX.resumeTimer();
}

#endif /* USE_COMMS_TRANSMITTER */