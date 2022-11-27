#ifndef NAHS_BRICKS_FEATURE_BAT_H
#define NAHS_BRICKS_FEATURE_BAT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <nahs-Bricks-Lib-CoIC.h>
#include <nahs-Bricks-Feature-BaseClass.h>
#include <nahs-Bricks-Lib-RTCmem.h>
#include <nahs-Bricks-Lib-FSmem.h>

class NahsBricksFeatureBat : public NahsBricksFeatureBaseClass {
    private:  // Variables
        static const uint16_t version = 3;
        typedef struct {
            bool batVoltageRequested;
            bool adc5VRequested;
            bool lastWasWallPowered;  // this is set to true if the last bat reading was over 4.5Volts
        } _RTCdata;
        _RTCdata* RTCdata = RTCmem.registerData<_RTCdata>();
        JsonObject FSdata = FSmem.registerData("b");
        NahsBricksLibCoIC_Expander* _expander;
        bool _use_expander = false;
        uint8_t _pin_bat_charging;
        uint8_t _pin_bat_standby;
        uint8_t _pin_bat_adc;

    public: // BaseClass implementations
        NahsBricksFeatureBat();
        String getName();
        uint16_t getVersion();
        void begin();
        void start();
        void deliver(JsonDocument* out_json);
        void feedback(JsonDocument* in_json);
        void end();
        void printRTCdata();
        void printFSdata();
        void brickSetupHandover();

    public:  // Brick-Specific setter
        void assignExpander(NahsBricksLibCoIC_Expander &expander);
        void setPins(uint8_t bat_charging, uint8_t bat_standby, uint8_t bat_adc);

    private:  // internal Helpers
        uint16_t _sampleADC();
        float _getBatVoltage();

    private:  // BrickSetup Helpers
        void _printMenu();
        void _calibrateADC();
        void _manualADC();
        void _testADC();
};

#if !defined(NO_GLOBAL_INSTANCES)
extern NahsBricksFeatureBat FeatureBat;
#endif

#endif // NAHS_BRICKS_FEATURE_BAT_H
