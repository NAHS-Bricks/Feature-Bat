#include <nahs-Bricks-Feature-Bat.h>
#include <nahs-Bricks-Lib-SerHelp.h>
#include <nahs-Bricks-OS.h>

NahsBricksFeatureBat::NahsBricksFeatureBat() {
    _use_expander = false;
}

/*
Returns name of feature
*/
String NahsBricksFeatureBat::getName() {
    return "bat";
}

/*
Returns version of feature
*/
uint16_t NahsBricksFeatureBat::getVersion() {
    return version;
}

/*
Configures FSmem und RTCmem variables (prepares feature to be fully operational)
*/
void NahsBricksFeatureBat::begin() {
    if (!FSdata.containsKey("adc5V")) FSdata["adc5V"] = 1023;
    if (!RTCmem.isValid()) {
        RTCdata->batVoltageRequested = false;
        RTCdata->adc5VRequested = false;
        RTCdata->lastWasWallPowered = false;
        if (_use_expander) {
            _expander->setInput(_pin_bat_charging);
            _expander->setInput(_pin_bat_standby);
            _expander->setPullup(_pin_bat_charging);
            _expander->setPullup(_pin_bat_standby);
        }
    }
}

/*
Starts background processes like fetching data from other components
*/
void NahsBricksFeatureBat::start() {
    if (!_use_expander) {
        pinMode(_pin_bat_charging, INPUT_PULLUP);
        pinMode(_pin_bat_standby, INPUT_PULLUP);
    }
}

/*
Adds data to outgoing json, that is send to BrickServer
*/
void NahsBricksFeatureBat::deliver(JsonDocument* out_json) {
    // fetch battery states depending on used connecor
    bool bat_charging;
    bool bat_standby;
    if (_use_expander) {
        bat_charging = (_expander->readInput(_pin_bat_charging) == LOW);
        bat_standby = (_expander->readInput(_pin_bat_standby) == LOW);
    }
    else {
        bat_charging = (digitalRead(_pin_bat_charging) == LOW);
        bat_standby = (digitalRead(_pin_bat_standby) == LOW);
    }

    // read the battery voltage if there is any reason to do so
    float batVoltage = 0;
    if (bat_charging || bat_standby || RTCdata->lastWasWallPowered || RTCdata->batVoltageRequested) {
        batVoltage = _getBatVoltage();
    }

    // send if battery is charging (and Brick is not wall-powered)
    if (bat_charging && !RTCdata->lastWasWallPowered) {
        if (!out_json->containsKey("y")) out_json->createNestedArray("y");
        JsonArray y_array = out_json->getMember("y").as<JsonArray>();
        y_array.add("c");
    }

    // send if battery charging is standby (and Brick is not wall-powered)
    if (bat_standby && !RTCdata->lastWasWallPowered) {
        if (!out_json->containsKey("y")) out_json->createNestedArray("y");
        JsonArray y_array = out_json->getMember("y").as<JsonArray>();
        y_array.add("s");
    }

    // send if Brick is wall-powered
    if (RTCdata->lastWasWallPowered) {
        if (!out_json->containsKey("y")) out_json->createNestedArray("y");
        JsonArray y_array = out_json->getMember("y").as<JsonArray>();
        y_array.add("w");
    }

    // check if battery voltage is requested
    if (RTCdata->batVoltageRequested) {
        RTCdata->batVoltageRequested = false;
        out_json->getOrAddMember("b").set(batVoltage);
    }

    // check if adc5V is requested
    if (RTCdata->adc5VRequested) {
        RTCdata->adc5VRequested = false;
        out_json->getOrAddMember("a").set(FSdata["adc5V"].as<uint16_t>());
    }
}

/*
Processes feedback coming from BrickServer
*/
void NahsBricksFeatureBat::feedback(JsonDocument* in_json) {
    // check if new adc5V value is transmitted
    if (in_json->containsKey("a")) {
        FSdata["adc5V"] = in_json->getMember("a").as<uint16_t>();
        BricksOS.requestFSmemWrite();
    }

    // evaluate requests
    if (in_json->containsKey("r")) {
        for (JsonVariant value : in_json->getMember("r").as<JsonArray>()) {
            switch(value.as<uint8_t>()) {
                case 3:
                    RTCdata->batVoltageRequested = true;
                    break;
                case 10:
                    RTCdata->adc5VRequested = true;
                    break;
            }
        }
    }
}

/*
Finalizes feature (closes stuff)
*/
void NahsBricksFeatureBat::end() {
}

/*
Prints the features RTCdata in a formatted way to Serial (used for brickSetup)
*/
void NahsBricksFeatureBat::printRTCdata() {
    Serial.print("  batVoltageRequested: ");
    SerHelp.printlnBool(RTCdata->batVoltageRequested);
}

/*
Prints the features FSdata in a formatted way to Serial (used for brickSetup)
*/
void NahsBricksFeatureBat::printFSdata() {
    Serial.print("  adc5V: ");
    Serial.println(FSdata["adc5V"].as<uint16_t>());
}

/*
BrickSetup hands over to this function, when features-submenu is selected
*/
void NahsBricksFeatureBat::brickSetupHandover() {
    _printMenu();
    while (true) {
        Serial.println();
        Serial.print("Select: ");
        uint8_t input = SerHelp.readLine().toInt();
        switch(input) {
            case 1:
                _calibrateADC();
                _testADC();
                break;
            case 2:
                _manualADC();  // no break to automatically test adc afterwards
            case 3:
                _testADC();
                break;
            case 9:
                Serial.println("Returning to MainMenu!");
                return;
                break;
            default:
                Serial.println("Invalid input!");
                _printMenu();
                break;
        }
    }
}

/*
Assign an CoIC_Expander if battery state pins are not connected directly
*/
void NahsBricksFeatureBat::assignExpander(NahsBricksLibCoIC_Expander &expander) {
    _use_expander = true;
    _expander = &expander;
}

/*
Configures PINs to detect charging and charging_standby
*/
void NahsBricksFeatureBat::setPins(uint8_t bat_charging, uint8_t bat_standby, uint8_t bat_adc) {
    _pin_bat_charging = bat_charging;
    _pin_bat_standby = bat_standby;
    _pin_bat_adc = bat_adc;
}

/*
Helper to get an avaraged ADC reading
*/
uint16_t NahsBricksFeatureBat::_sampleADC() {
    uint32_t adcReading = 0;
    for(uint8_t i = 0; i < 3; ++i) {
        adcReading += analogRead(_pin_bat_adc);
        delay(20);
    }
    return adcReading /= 3;
}

/*
Helper to read and calculate the current Battery Voltage
*/
float NahsBricksFeatureBat::_getBatVoltage() {
    float reading = (_sampleADC() * 5.0) / FSdata["adc5V"].as<uint16_t>();
    if (reading >= 4.5) RTCdata->lastWasWallPowered = true;
    else RTCdata->lastWasWallPowered = false;
    return reading;
}

/*
Helper to print Feature submenu during BrickSetup
*/
void NahsBricksFeatureBat::_printMenu() {
    Serial.println("1) Calibrate ADC");
    Serial.println("2) Set manual adc5V");
    Serial.println("3) Test Bat Reading");
    Serial.println("9) Return to MainMenu");
}

/*
BrickSetup function to calibrate ADC
*/
void NahsBricksFeatureBat::_calibrateADC() {
    Serial.print("Voltage on TPB- and TPB+: ");
    float tpb = SerHelp.readLine().toFloat();
    if (tpb <= 0) {
        Serial.println("Invalid value, breaking...");
        return;
    }
    Serial.println("Sampling ADC...");
    uint32_t avg = _sampleADC();
    Serial.print("AVG: ");
    Serial.println(avg);

    FSdata["adc5V"] = (uint16_t)((avg * 5.0) / tpb);
    Serial.print("Calculated value for 5.0 Volt: ");
    Serial.println(FSdata["adc5V"].as<uint16_t>());
}

/*
BrickSetup function to set a manual value for adc5V
*/
void NahsBricksFeatureBat::_manualADC() {
    Serial.print("Enter value in range 1..1023: ");
    uint16_t input = SerHelp.readLine().toInt();
    if (input < 1 || input > 1023) {
        Serial.println("Invalid value, breaking...");
        return;
    }
    FSdata["adc5V"] = input;
    Serial.print("Set adc5V to: ");
    Serial.println(FSdata["adc5V"].as<uint16_t>());
}

/*
BrickSetup function to do a battery voltage reading
*/
void NahsBricksFeatureBat::_testADC() {
    Serial.println("Reading bat-voltage...");
    float voltage = _getBatVoltage();
    Serial.print("Voltage is: ");
    Serial.println(voltage);
}

//------------------------------------------
// globally predefined variable
#if !defined(NO_GLOBAL_INSTANCES)
NahsBricksFeatureBat FeatureBat;
#endif
