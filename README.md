## `Comms.hpp` - Arduino Serial Communications Library

This single header library provides a timer interrupt-driven and configurable layer for serial communication on Arduino platforms, particularly those with multiple hardware serial ports like Arduino Mega 2560. It supports separate definitions for transmit and receive serial ports and timers, and includes an optional XOR-based encryption/decryption mechanism.

### Features

  * **Interrupt-Driven:** Uses independent hardware timers (`ITimer`) for non-blocking serial reception and transmission.
  * **Configurable Serial Ports:** Supports using different hardware serial ports for receiving (`SERIAL_RX`) and transmitting (`SERIAL_TX`), from `Serial1` to `Serial3`. **Note:** `Serial0` is intentionally excluded.
  * **Configurable Timers:** Uses separate timers (`ITIMER_RX`, `ITIMER_TX`) from `ITimer1` to `ITimer5`. **Note:** `Timer0` is intentionally excluded, and both must be different.
  * **Length-Based:** Messages are prefixed with a size byte, allowing the receiver to know the expected message length.
  * **Basic Security:** Includes an optional `xor_key` for simple XOR-based message encryption/decryption.
  * **Maximum Transmission Unit (MTU):** Defines a max message size of 254 bytes (`COMMS_MTU`). **Note:** The internal buffer's default size is 255 bytes, but it is zero-terminated.

### Prerequisites

You need to include the [**`TimerInterrupt`**](https://github.com/khoih-prog/TimerInterrupt) library by khoih-prog in your project or similar versions, depending on your board.

### Configuration

Before including `Comms.hpp`, you **must** define the following preprocessor macros, typically in your main sketch file:

| Macro | Description | Valid Values |
| :--- | :--- | :--- |
| `SERIAL_RX_NO` | The hardware serial port number for receiving. | 1, 2, or 3 |
| `SERIAL_TX_NO` | The hardware serial port number for transmitting. | 1, 2, or 3 |
| `ITIMER_RX_NO` | The `ITimer` number for the receive interrupt. | 1 to 5 |
| `ITIMER_TX_NO` | The `ITimer` number for the transmit interrupt. | 1 to 5 |
| `USE_COMMS_RECEIVER` | **(Optional)** Define to include the `Receiver` class. | - |
| `USE_COMMS_TRANSMITTER` | **(Optional)** Define to include the `Transmitter` class. | - |
| `USE_SERIAL_LOG` | **(Optional)** Define to enable serial logging for debugging on `Serial` (HardwareSerial0). | - |

**Example Configuration (in your sketch before `#include "Comms.hpp"`):**

```cpp
#define SERIAL_RX_NO 1 // Use Serial1 for receiving
#define SERIAL_TX_NO 2 // Use Serial2 for transmitting
#define ITIMER_RX_NO 1 // Use ITimer1 for RX interrupt
#define ITIMER_TX_NO 2 // Use ITimer2 for TX interrupt

#define USE_COMMS_RECEIVER
#define USE_COMMS_TRANSMITTER
// #define USE_SERIAL_LOG // Uncomment to enable debugging logs

#include "Comms.hpp"
```

### Usage

#### Global Objects

If `USE_COMMS_RECEIVER` and/or `USE_COMMS_TRANSMITTER` are defined, the library creates global instances:

  * `Receiver SerialReceiver;`
  * `Transmitter SerialTransmitter;`

#### Initialization

1.  **Set Baud Rate:** Use the provided macro to initialize the serial ports. This must be done once.

    ```cpp
    beginCommsSerials(DEFAULT_BAUD_RATE);
    ```
    or with a custom baud rate
    ```cpp
    beginCommsSerials(115200);
    ```

2.  **Initialize Receiver/Transmitter:** Call the `begin()` method on the required object(s). The default frequencies are calculated based on the library's default baud rate.

    ```cpp
    SerialReceiver.begin();
    SerialTransmitter.begin();
    ```
    or with a custom baud rate
    ```cpp
    SerialReceiver.begin(RX_FREQUENCY(115200));
    SerialTransmitter.begin(TX_FREQUENCY(115200));
    ```
3. **(Optional) Set XOR key for encoding/decoding:**
    ```cpp
    SerialReceiver.xor_key = 0xAA;
    SerialTransmitter.xor_key = 0xAA;
    ```

#### Receiver (`SerialReceiver`)

| Method | Description |
| :--- | :--- |
| `begin()` | Starts the RX timer interrupt. |
| `hasMessage()` | Returns `true` if a complete message has been received. |
| `getMessage()` | Returns a pointer to the received message buffer (null-terminated). |
| `resume()` | Clears the buffer and restarts the receiver to listen for the next message. **Important:** Call this after processing a message. |

#### Transmitter (`SerialTransmitter`)

| Method | Description |
| :--- | :--- |
| `begin()` | Starts the TX timer and immediately pauses it, setting the transmitter to an available state. |
| `isAvailable()` | Returns `true` if the last transmission is complete and the transmitter is ready for a new message. |
| `sendMessage(const byte *buffer, int size)` | Copies the data, prefixes the size, encrypts (with `xor_key`), and starts the TX timer interrupt for transmission. `size` defaults to the cstring length if not specified. |