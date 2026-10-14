// The MIT License (MIT)
//
// Copyright (c) INTERNET OF THINGER SL
// Author: alvarolb@gmail.com (Alvaro Luis Bustamante)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef THINGER_NB_CORE_H
#define THINGER_NB_CORE_H

#include <Arduino.h>
#include <EEPROM.h>
#include "ThingerClient.h"


#define TINY_GSM_MODEM_BC660

// uncomment to enable AT debug messages
//#define THINGER_DUMP_AT_COMMANDS

// uncomment to enable tiny gsm debug messages
//#define TINY_GSM_DEBUG Serial

// #define DUMP_AT_COMMANDS
#ifndef TINY_GSM_RX_BUFFER
// Maximum BC660 RX data length size is 1400 bytes
#define TINY_GSM_RX_BUFFER 1536
#endif

// include TinyGSM library
#include <TinyGsmClient.h>
#include <ThingerTinyGSM.h>

#ifndef THINGER_NETWORK_REGISTER_TIMEOUT
#define THINGER_NETWORK_REGISTER_TIMEOUT 300000
#endif

#ifdef THINGER_DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(Serial1, Serial);
#define SerialAT debugger
#else
#define SerialAT Serial1
#endif

// PINS for Hexacore Development
#ifndef THINGER_NB_M2CORE
#define PIN_MODEM_RESET 13
#define PIN_MODEM_WAKEUP 12
#define PIN_MODEM_PWR_KEY 19
#define PIN_MODEM_RX 32
#define PIN_MODEM_TX 33
// PINS for M2-based modules
#else
#define PIN_MODEM_RESET 5
#define PIN_MODEM_WAKEUP 18
#define PIN_MODEM_PWR_KEY 23
#define PIN_MODEM_RX 4
#define PIN_MODEM_TX 13
#endif

// EEPROM address for storing config CRC (1 byte)
#define EEPROM_CRC_ADDR 0

class ThingerNBCore : public ThingerClient
{

public:
    ThingerNBCore(const char *user, const char *device, const char *device_credential) : 
        ThingerClient(client_, user, device, device_credential),
        modem_(SerialAT),
        client_(modem_)
    {
    }

    ~ThingerNBCore()
    {
    }

private:

    /// @brief Calculate CRC8 checksum.
    /// @param data Pointer to the input data buffer.
    /// @param len Length of the data buffer.
    /// @param crc Initial CRC value (default is 0x00).
    /// @return Calculated CRC8 checksum.
    uint8_t crc8(const uint8_t *data, size_t len, uint8_t crc = 0x00) {
        while (len--) {
            crc ^= *data++;
            for (uint8_t i = 0; i < 8; ++i)
                crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
        }
        return crc;
    }

    /// @brief  Calculate configuration CRC
    /// @return
    /// @note   This function calculates the CRC8 checksum of the current configuration
    ///         parameters (APN, username, password, preferred bands, preferred operator,
    ///         power save mode, and network LED) and returns the result.
    uint8_t calc_crc_config()
    {
        uint8_t crc = 0xFF;
        if (apn_) crc = crc8((const uint8_t *)apn_, strlen(apn_), crc);
        if (apn_username_) crc = crc8((const uint8_t *)apn_username_, strlen(apn_username_), crc);
        if (apn_password_) crc = crc8((const uint8_t *)apn_password_, strlen(apn_password_), crc);
        if (preferred_bands_) crc = crc8((const uint8_t *)preferred_bands_, strlen(preferred_bands_), crc);
        if (preferred_operator_) crc = crc8((const uint8_t *)preferred_operator_, strlen(preferred_operator_), crc);
        crc = crc8((const uint8_t *)&power_save_, sizeof(power_save_), crc);
        crc = crc8((const uint8_t *)&network_led_, sizeof(network_led_), crc);   
        return ~crc;
    }

    /// @brief Initialize the modem with default settings
    /// @return true if the modem was initialized successfully, false otherwise
    bool initialize_defaults()
    {
        // read current module configuration
        EEPROM.begin(1);
        uint8_t stored_crc = EEPROM.read(EEPROM_CRC_ADDR);

        // calculate current configuration CRC
        uint8_t current_crc = calc_crc_config();

        // check if the stored CRC matches the current configuration
        if(stored_crc == current_crc)
        {
            THINGER_DEBUG("NB-IOT", "Configuration unchanged, skipping...");
            EEPROM.end();
            return true;
        }

        THINGER_DEBUG("NB-IOT", "Setting NB-IOT module configuration");

        {   // stop modem functionality (saved to nvram)
            modem_.sendAT("+CFUN=0");
            modem_.waitResponse();
        }
        
        {   // configure default APN (saved to nvram)
            if(apn_username_ != nullptr && apn_password_ != nullptr){
                THINGER_DEBUG_VALUE("NB-IOT", "Setting APN: ", apn_);
                THINGER_DEBUG_VALUE("NB-IOT", "Setting APN username: ", apn_username_);
                THINGER_DEBUG_VALUE("NB-IOT", "Setting APN password: ", apn_password_);
                modem_.sendAT("+QCGDEFCONT=\"IP\",", "\"", apn_, "\",", "\"", apn_username_, "\",", "\"", apn_password_, "\"");
            }
            else{
                THINGER_DEBUG_VALUE("NB-IOT", "Setting APN: ", apn_);
                modem_.sendAT("+QCGDEFCONT=\"IP\",", "\"", apn_, "\"");
            }
            modem_.waitResponse();
        }

        {   // preferred search bands (saved to nvram)
            if (preferred_bands_ == nullptr){
                THINGER_DEBUG("NB-IOT", "No bands configured, using all bands");
                modem_.sendAT("+QBAND=0");
            }
            else{
                THINGER_DEBUG_VALUE("NB-IOT", "Setting preferred bands: ", preferred_bands_);
                modem_.sendAT("+QBAND=", preferred_bands_);
            }
            modem_.waitResponse();
        }

        {   // set preferred operators (saved to nvram)
            if (preferred_operator_ == nullptr){
                THINGER_DEBUG("NB-IOT", "No preferred operators configured, using default");
                modem_.sendAT("+COPS=0");
            }
            else{
                THINGER_DEBUG_VALUE("NB-IOT", "Setting preferred operators: ", preferred_operator_);
                modem_.sendAT("+COPS=4,2,\"", preferred_operator_, "\"");
            }
            modem_.waitResponse();
        }

        {   // set power save mode (saved to nvram)
            THINGER_DEBUG_VALUE("NB-IOT", "Setting power save mode:", power_save_ ? "enabled" : "disabled");

            // disable power save mode (saved to nvram)
            modem_.sendAT("+CPSMS=", power_save_ ? "1" : "0");
            modem_.waitResponse();

            // disable eDRX (saved to nvram)
            modem_.sendAT("+CEDRXS=", power_save_ ? "1" : "0");
            modem_.waitResponse();

            // edRX and PTW -> disabled (saved to nvram)
            modem_.sendAT("+QEDRXCFG=", power_save_ ? "1" : "0");
            modem_.waitResponse();
        }
        
        {   // network net led (saved to nvram)
            THINGER_DEBUG_VALUE("NB-IOT", "Setting network LED:", network_led_ ? "enabled" : "disabled");
            modem_.sendAT("+QLEDMODE=", network_led_ ? "1" : "0");
            modem_.waitResponse();
        }

        // restart modem functionality (saved to nvram)
        modem_.sendAT("+CFUN=1");
        modem_.waitResponse();

        THINGER_DEBUG("NB-IOT", "Module configuration done!");

        {   // store crc configuration in EEPROM
            EEPROM.write(EEPROM_CRC_ADDR, current_crc);
            EEPROM.commit();
        }

        THINGER_DEBUG("NB-IOT", "Module reset...");
        
        // reset modem, as some parameters like QCGDEFCONT are not applied until reset
        reset();
        EEPROM.end();
        return modem_.init(pin_);
    }

public:

    bool init_modem(bool reset_modem = false)
    {
        // ensure pins are configured as output
        pinMode(PIN_MODEM_RESET, OUTPUT);
        pinMode(PIN_MODEM_WAKEUP, OUTPUT);
        pinMode(PIN_MODEM_PWR_KEY, OUTPUT);

        // start modem serial
        Serial1.begin(115200, SERIAL_8N1, PIN_MODEM_RX, PIN_MODEM_TX);

        // physically reset module
        if (reset_modem) reset();

        // init modem with sim card PIN if needed
        bool init = modem_.init(pin_);

        if (!init)
        {
            THINGER_DEBUG("NB-IOT", "Cannot init modem! Is it connected?");
            reset();
            return false;
        }

        // configure preferred configuration        
        return initialize_defaults();
    }

protected:

    bool network_connected() override
    {
        RegStatus s = modem_.getRegistrationStatus();
        return (s == REG_OK_HOME || s == REG_OK_ROAMING);
    }

    void connect_socket_success() override
    {
        connection_errors_ = 0;
    }

    void connect_socket_error() override
    {
        connection_errors_++;
        if (connection_errors_ == 6)
        {
            // soft reset the modem if socket cannot be connected in a while
            THINGER_DEBUG("NETWORK", "Soft-Restarting modem_...");
            modem_.restart(pin_);
        }
        else if (connection_errors_ == 12)
        {
            // still cannot connect after a soft-reset? -> hard reset
            THINGER_DEBUG("NETWORK", "Hard-Restarting modem_...");
            init_modem(true);
        }
    }

    virtual bool connect_network()
    {
        THINGER_DEBUG("NETWORK", "Waiting for network...");
        unsigned long network_timeout = millis();
        while (1)
        {
            RegStatus cereg = modem_.getRegistrationStatus();
            THINGER_DEBUG_VALUE("NETWORK", "CEREG: ", cereg);

            // connected
            if (cereg == REG_OK_HOME || cereg == REG_OK_ROAMING)
            {
                break;
            }

            // registration denied
            if (cereg == REG_DENIED)
            {
                THINGER_DEBUG("NETWORK", "Connection denied by operator")
                // wait some time to not flood operator if sim is legitimately disabled
                delay(30000);
                // reset device
                init_modem(true);
                return false;
            }

            // other ? i.e., 2, searching, or 0. reboot device if timed out
            if (millis() - network_timeout > THINGER_NETWORK_REGISTER_TIMEOUT)
            {
                THINGER_DEBUG("NETWORK", "Cannot connect network!")
                // reset device
                init_modem(true);
                return false;
            }

            delay(500);
        }

        delay(100);
        THINGER_DEBUG("NETWORK", "Network registered...");
        return true;
    }

#ifdef _DISABLE_TLS_
    virtual bool secure_connection()
    {
        return false;
    }
#endif

public:

    /// @brief  Get the TinyGSM modem object
    /// @return  Reference to the TinyGSM modem object
    /// @note   This function returns a reference to the TinyGSM modem object, which can be used
    ///         to access the modem's functions and properties.
    /// @note   This function is useful for advanced users who want to access the modem's
    ///         functions directly, without going through the ThingerNBCore class.
    TinyGsm &getTinyGsm()
    {
        return modem_;
    }

    /// @brief  Get the TinyGSM client object
    /// @return  Reference to the TinyGSM client object
    /// @note   This function returns a reference to the TinyGSM client object, which can be used
    ///         to access the client's functions and properties.
    TinyGsmClient &getTinyGsmClient()
    {
        return client_;
    }

    /// @brief  Set APN for the modem
    /// @param APN  String with the APN to be used. For example: "iot.1nce.net"
    /// @param user  String with the username to be used. For example: ""
    /// @param password  String with the password to be used. For example: ""
    void set_apn(const char *APN, const char *user = "", const char *password = "")
    {
        apn_ = APN;
        apn_username_ = user;
        apn_password_ = password;
    }

    /// @brief  Set preferred operator for the modem
    /// @param operator_name  String with the operator id to be used. For example: 21407 for "movistar"
    void set_preferred_operator(const char *operator_name)
    {
        preferred_operator_ = operator_name;
    }

    /// @brief  Set preferred bands for the modem
    /// @param bands  String with the bands to be used. For example: "3,20,8,3"
    void set_preferred_bands(const char *bands)
    {
        preferred_bands_ = bands;
    }

    /// @brief  Set the PIN for the modem
    /// @param pin  String with the PIN to be used. For example: "1234"
    /// @note   This is only needed if the SIM card has a PIN code
    /// @note   This is not needed for the 1NCE SIM card
    void set_pin(const char *pin)
    {
        pin_ = pin;
    }

    /// @brief  Reset the modem by toggling the reset pin
    /// @note   This is only needed if the modem is not responding or is in a bad state
    void reset()
    {
        // modem reset
        THINGER_DEBUG("NETWORK", "Reseting modem...");
        digitalWrite(PIN_MODEM_RESET, 1);
        delay(100);
        digitalWrite(PIN_MODEM_RESET, 0);
        delay(100);
    }

    int16_t get_batt_voltage()
    {
        return modem_.getBattVoltage();
    }

    String get_modem_info()
    {
        return modem_.getModemInfo();
    }

    String get_sim_iccid()
    {
        return modem_.getSimCCID();
    }

    String get_modem_imei()
    {
        return modem_.getIMEI();
    }

    String get_modem_imsi()
    {
        return modem_.getIMSI();
    }

    String get_operator()
    {
        return modem_.getOperator();
    }

    void set_power_save(bool power_save)
    {
        power_save_ = power_save;
    }

    void set_network_led(bool network_led)
    {
        network_led_ = network_led;
    }

private:
    // nb-iot configuration
    const char *apn_                = "iot.1nce.net";
    const char *apn_username_       = nullptr;
    const char *apn_password_       = nullptr;
    const char *preferred_operator_ = nullptr;
    const char *preferred_bands_    = nullptr;
    bool power_save_                = false;
    bool network_led_               = true;

    // thinger configuration
    const char *user_               = nullptr;
    const char *password_           = nullptr;
    const char *pin_                = nullptr;

    // tinygsm 
    uint8_t connection_errors_      = 0;
    TinyGsm modem_;

#ifdef _DISABLE_TLS_
    TinyGsmClient client_;
#else
    TinyGsmClientSecure client_;
#endif

};

#endif
