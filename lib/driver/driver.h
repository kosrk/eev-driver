#ifndef DRIVER_H

#include "Arduino.h"

struct driverConfig{
  int enablePin,
  stepPin,
  dirPin,
  busyPin,
  velocity,
  microsteps, 
  totalSteps,
  overdriveSteps,
  initOverdriveSteps,
  highTime,
  maxRelPosition,
  holdingTime;
};

bool ValidateConfig(driverConfig dConfig);

class Driver{
  public:  
    Driver (driverConfig dConfig);
    bool Init();
    bool Enable();
    bool Disable();
    bool InitialOverdrive();
  private:
    driverConfig config;
    int currentAbsPosition,  // положение задвижки в полных шагах, где 0 - полностью закрыто
    stepTimeout;             // пауза между импульсами Step в мкс
    bool busy,               // сигнализирует, что привод выполняет движение
    ready,                   // сигнализирует, что привод готов начать движение
    enabled;                 // сигнализирует, что подано питание на привод
    bool setBusy();
    bool setNotBusy();
    bool convertRelPositionToAbs(int relPosition, int *absPosition);
    bool calcStepsToRelPosition(int targetRelPosition, int *qtyOfSteps);
    bool makeSteps(int steps);
};
#define DRIVER_H
#endif