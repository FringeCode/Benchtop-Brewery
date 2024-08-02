#include "DallasTemperature.h"
#include "PID_v1.h"

namespace DodgeBrewingSystems {

  namespace TemperatureControl {

    namespace TempControlPID {

      struct TempControllerPID {};

    }

  }

  struct TempPIDParams {

    double Kp, Ki, Kd;
    double setPoint, inputTemp;
    double output;

  };

  struct MashTempPIDParams : TempPIDParams {
    double strikeKp, strikeKi, strikeKd;
    float ratioWaterToGrain;
    double grainTemp;
    double initWaterTemp;
    double rampOffTemp;
    double startPIDTemp;

    MashTempPIDParams() {
      Kp = 0; Ki = 0; Kd = 0;
      strikeKp = 0; strikeKi = 0; strikeKd = 0;
      setPoint = 0; inputTemp = 0;
      output = 0;
      ratioWaterToGrain = 0;
      grainTemp = 0; initWaterTemp = 0;
      rampOffTemp = 0; startPIDTemp = 0;
    }
    ~MashTempPIDParams() {}

  };

  

  struct MashPIDv1DallasRelayController {
    MashPIDv1DallasRelayController () {} // default ctor
    ~MashPIDv1DallasRelayController() {} // default dtor
    MashPIDv1DallasRelayController(MashPIDv1DallasRelayController& m) = delete;
    //MashPIDv1DallasRelayController(MashPIDv1DallasRelayController&& m) = delete;
    //MashPIDv1DallasRelayController operator= (MashPIDv1DallasRelayController& m) = delete;
    //MashPIDv1DallasRelayController operator= (MashPIDv1DallasRelayController&& m) = delete;


    DodgeBrewingSystems::MashTempPIDParams& params;
    PID& mashPID;
    // DallasTemperature& thermometer;
    uint8_t relayPin; // turn on/off hotplate
    const uint8_t heatOn = 1;
    const uint8_t heatOff = 0;
    uint8_t windowStartTime, windowSize = 10000;

    void updateCycle (void) {

      mashPID.Compute();
      // PID output is a measure of time ?
      unsigned long windowElapsed = millis() - windowStartTime;
      if (windowElapsed > windowSize)
        windowStartTime += windowSize; // 10s 'window's

      // PID Magic ?
      if (params.output < 100)
        digitalWrite(relayPin, heatOff);
      else if (params.output > windowElapsed)
        digitalWrite(relayPin, heatOn);
      else
        digitalWrite(relayPin, heatOff);

    }
    
  };


}