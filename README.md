# ESP32 Relay Control System

A robust ESP32-based relay control system with web interface, remote management, and OTA update capabilities.

## Features

- Control up to 8 relays with physical buttons
- Web interface for remote control
- Secure authentication
- OTA (Over-The-Air) firmware updates
- Event logging with timestamps
- WiFi configuration via Access Point mode
- Support for static and dynamic IP configuration
- Remote server communication for state updates

## Hardware Requirements

- ESP32 development board
- 8-channel relay module
- Push buttons for local control
- Power supply (5V/2A recommended)

## Pin Configuration

### Default Pinout

| Function      | Default Pin |
|---------------|-------------|
| Input 1       | 34          |
| Relay 1       | 12          |
| Relay 2       | 14          |
| Relay 3       | 27          |
| Relay 4       | 26          |
| Relay 5       | 25          |
| Relay 6       | 33          |
| Relay 7       | 32          |
| Relay 8       | 13          |
| Button 1      | 4           |
| Button 2      | 16          |
| Button 3      | 17          |
| Button 4      | 5           |
| Button 5      | 18          |
| Button 6      | 23          |
| Button 7      | 19          |
| Button 8      | 22          |

### Flexible Configuration

The system features a flexible architecture that adapts to various hardware configurations:

1. **Pin Configuration** - All pins are configured during the first flash or when `INIT_MAGIC` is changed
2. **Device ID** - Unique identifier set during initial setup
3. **Relay Logic Levels** - Configurable active state (HIGH/LOW) for relays
4. **Settings Persistence** - All parameters are stored in EEPROM

This architecture enables:
- Using a single firmware for different hardware configurations
- Updating all devices via OTA, regardless of their specific settings
- Easy adaptation to different boards and relay modules
- Preserving unique device settings after firmware updates

To modify configuration after initial setup:
1. Change the `INIT_MAGIC` value in the code
2. Flash the device
3. Perform initial configuration

## Configuration

1. Create a `set_board.h` file with your configuration (see `set_board_example.h` for reference)
2. Configure WiFi credentials and server settings
3. Compile and upload to your ESP32

## First Time Setup

1. On first boot, the device will start in Access Point mode
2. Connect to the ESP32's WiFi network
3. Open a web browser and navigate to `192.168.4.1`
4. Configure your WiFi settings and save
5. The device will reboot and connect to your network

## OTA Updates

The system supports Over-The-Air updates. Place the following files on your web server:
- `firmware_vers.txt` - Contains the current firmware version
- `firmware.bin` - The firmware binary
- `firmware_update.sha256` - SHA256 checksum of the firmware

## Security

- All server communications use HTTPS
- Password protection for web interface
- Session-based authentication
- Configuration stored in EEPROM with checksum verification

## Logging

The system maintains an event log that includes:
- Relay state changes
- System events
- Errors
- Clock synchronization

## Dependencies

- Arduino ESP32 Core
- Button2
- Ticker
- WiFi
- WebServer
- EEPROM
- WiFiClientSecure
- DNSServer


## Version History

- v1.18 - Added PIN field support

## Server-Side Implementation

### Data Structure

```cpp
struct Rele {
    char rr_status[9];      // Current relay states (8 relays + null terminator)
    char rr_command[9];     // Pending relay commands
    int acp_status;         // ACP status code
    int sn;                 // Serial number
    int time_onl;           // Last online timestamp
    bool put;               // Command execution flag
    int online;             // Online status
    bool get_log_file_flag; // Log file request flag
    int userID_set_GFF;     // User ID for log access
    time_t time_set_GFF;    // Timestamp for log access
    std::string log_file;   // Log file content
    unsigned short pin;     // Security PIN
    MyRand my_rand;         // Random number generator for security
};

static std::vector<Rele> rele_list;  // In-memory storage of relay devices
```

### Request Handler

```cpp
std::string obrobnyk_relay(
    const char* sn_txt,     // 10-digit serial number
    const char* acp_txt,    // 10-digit ACP code
    const char* relays,     // 8-character relay state string
    const char* init_txt,   // 4-character init flag ("true" or "fals")
    const char* cs_txt,     // Checksum for request validation
    const char* pin_txt = nullptr  // Optional PIN for authentication
);
```

#### Request Parameters
- `sn_txt`: 10-digit device serial number
- `acp_txt`: 10-digit ACP code (firmware version)
- `relays`: 8-character string representing relay states ('0' or '1' for each relay)
- `init_txt`: Initialization flag ("true" or "fals")
- `cs_txt`: Checksum for request validation
- `pin_txt`: Optional PIN for additional security

#### Return Value
JSON string with the following format:
```json
{
    "relays": "XXXXXXXX",  // 8-character command or "XXXXXXXX" if no command
    "online": "0",         // Online status
    "cs": "1234"           // Response checksum
}
```

#### Security Features
- Checksum validation for all requests
- Device authentication using serial number
- Optional PIN protection
- Session-based command handling
- Automatic cleanup of inactive devices

#### Error Handling
Returns error response for invalid requests:
```json
{
    "relays": "XXXXXXXX",
    "online": "0",
    "cs": "0"
}
```

### Database Integration
- Relays are stored in a database for persistence
- Automatic creation of new relay entries when devices first connect
- Log file management for each device

### State Management
- Tracks device online/offline status
- Manages pending commands
- Handles initialization and authentication
- Maintains session state

### Logging
- Detailed logging of device communications
- Timestamp tracking for all events
- Error logging for invalid requests
- Debug logging for troubleshooting

## Server Implementation

The server-side implementation for this project is available in a separate repository:

[servak_na_cpp_relays]([https://github.com/viteck1048/servak_na_cpp_relays](https://github.com/viteck1048/javaIoTserver/tree/main/servak_na_winapi_relays))

The server handles device communication, command processing, and state management as described in the Server-Side Implementation section above.
