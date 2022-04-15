#include "driver.h"

// Валидация данных конфигурации
//----------------------------------------
bool ValidateConfig(driverConfig dConfig) {
  if (dConfig.velocity < 1) return false;
  if (dConfig.microsteps < 1) return false; 
  if (dConfig.totalSteps < 1) return false;
  if (dConfig.overdriveSteps < 1) return false;
  if (dConfig.initOverdriveSteps < 1) return false;
  if (dConfig.highTime < 1) return false;
  if (dConfig.maxRelPosition < 10) return false;
  if (dConfig.holdingTime < 0) return false;
  return true;
}

// Конструктор класса
//----------------------------------------
Driver::Driver() {
  currentAbsPosition = 0;
  busy = false;
  ready = false;
  enabled = false;
}

// Инициализация
//----------------------------------------
bool Driver::Init(driverConfig dConfig) {
  config = dConfig;
    // Вычисление паузы между импульсами Step
  stepTimeout = 1000000 / (config.velocity * config.microsteps) - config.highTime;
  if (stepTimeout < 10) stepTimeout = 10;  
  pinMode(config.enablePin, OUTPUT);
  pinMode(config.stepPin, OUTPUT);
  pinMode(config.dirPin, OUTPUT);
  pinMode(config.busyPin, OUTPUT);
  return true;
}

// Подать питание на мотор
//----------------------------------------
bool Driver::Enable() {
  digitalWrite(config.enablePin, HIGH);
  enabled = true;
  return true;
}
    
// Отключить питание мотора
//----------------------------------------
bool Driver::Disable() {
  digitalWrite(config.enablePin, LOW);
  enabled = false;
  return true;
};

// Перейти в состояние "занят" 
//----------------------------------------
bool Driver::setBusy() {
  busy = true;
  digitalWrite(config.busyPin, HIGH);
  return true;
};

// Перейти в состояние "свободен"
//----------------------------------------
bool Driver::setNotBusy() {
  busy = false;
  digitalWrite(config.busyPin, LOW);
  return true;
};

// Получить текущую относительную позицию
//----------------------------------------
int Driver::GetRelPosition() {
  int res = (currentAbsPosition * config.maxRelPosition) / (config.totalSteps * config.microsteps);
  return res;
};

// Пересчет относительной позиции в абсолютную
//----------------------------------------
bool Driver::convertRelPositionToAbs(int relPosition, int *absPosition) {
  if (relPosition > config.maxRelPosition) return false;
  int res = (relPosition * config.totalSteps * config.microsteps) / config.maxRelPosition;
  *absPosition = res;
  return true;
};

// Вычислить число шагов до относительной позиции
//----------------------------------------
bool Driver::calcStepsToRelPosition(int targetRelPosition, int *qtyOfSteps) {
  int targetAbsPosition;
  if (!convertRelPositionToAbs(targetRelPosition, &targetAbsPosition)) return false;
  int steps = targetAbsPosition - currentAbsPosition;
  *qtyOfSteps = steps;
  return true;
};

// Выполнить шаги
//----------------------------------------
bool Driver::makeSteps(int steps) {
  if (busy || !enabled) return false;
  setBusy();
  if (steps < 0) {
    digitalWrite(config.dirPin, HIGH);
  }
  for (int i = 0; i < abs(steps); i++) {
    digitalWrite(config.stepPin, HIGH);
    delayMicroseconds(config.highTime);
    digitalWrite(config.stepPin, LOW);
    delayMicroseconds(stepTimeout);
  };
  if (steps < 0) {
    digitalWrite(config.dirPin, LOW);
  }
  setNotBusy();
  return true;
};

// Выполнить перегрузку при неизвестном положении. Сбросить положение на 0.
// Положительное направление вращения - в сторону открытия клапана.
//----------------------------------------
bool Driver::InitialOverdrive() {
  if (!enabled) Enable();
  int overdriveSteps = -1 * (config.totalSteps + config.initOverdriveSteps) * config.microsteps; 
  makeSteps(overdriveSteps);
  currentAbsPosition = 0;
  delay(config.holdingTime);
  Disable();
  ready = true;
  return true;
};

// Выполнить перегрузку при известном положении. Вернуться в исходное положение.
// Положительное направление вращения - в сторону открытия клапана.
//----------------------------------------
bool Driver::Overdrive() {
  int oldPosition = GetRelPosition();
  if (!enabled) Enable();
  int overdriveSteps;
  calcStepsToRelPosition(0, &overdriveSteps);
  overdriveSteps -= config.overdriveSteps;
  makeSteps(overdriveSteps);
  delay(config.holdingTime);
  currentAbsPosition = 0;
  calcStepsToRelPosition(oldPosition, &overdriveSteps);
  makeSteps(overdriveSteps);
  delay(config.holdingTime);
  currentAbsPosition = oldPosition;
  Disable();
  return true;
};

// Выполнить перемещение в заданную относительную координату.
//----------------------------------------
bool Driver::GoToRelPosition(int x) {
  if (!enabled) Enable();
  int steps;
  calcStepsToRelPosition(x, &steps);
  makeSteps(steps);
  currentAbsPosition += steps;
  delay(config.holdingTime);
  Disable();
  return true;
};
