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
  currentAbsPosition = 0L;
  busy = false;
  ready = false;
  enabled = false;
}

// Инициализация
//----------------------------------------
bool Driver::Init(driverConfig dConfig) {
  config = dConfig;
  // Вычисление паузы между импульсами Step
  stepTimeout = 1000000L / (config.velocity * config.microsteps) - config.highTime;
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
  long res = (long(currentAbsPosition) * config.maxRelPosition) / (long(config.totalSteps) * config.microsteps);
  return int(res);
};

// Пересчет относительной позиции в абсолютную
//----------------------------------------
bool Driver::convertRelPositionToAbs(int relPosition, long *absPosition) {
  if (relPosition > config.maxRelPosition || relPosition < 0) return false;
  long res = (long(relPosition) * config.totalSteps * config.microsteps) / config.maxRelPosition;
  *absPosition = res;
  return true;
};

// Вычислить число шагов до относительной позиции
//----------------------------------------
bool Driver::calcStepsToRelPosition(int targetRelPosition, long *qtyOfSteps) {
  long targetAbsPosition;
  if (!convertRelPositionToAbs(targetRelPosition, &targetAbsPosition)) return false;
  long steps = targetAbsPosition - currentAbsPosition;
  *qtyOfSteps = steps;
  return true;
};

// Выполнить шаги
//----------------------------------------
bool Driver::makeSteps(long steps) {
  if (busy || !enabled) return false;
  setBusy();
  if (steps < 0) {
    digitalWrite(config.dirPin, HIGH);
  }
  for (long i = 0; i < abs(steps); i++) {
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
  long overdriveSteps = -1L * (config.totalSteps + config.initOverdriveSteps) * config.microsteps; 
  if (!makeSteps(overdriveSteps)) {
    Disable();
    return false;
  };
  currentAbsPosition = 0L;
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
  long overdriveSteps;
  if (!calcStepsToRelPosition(0, &overdriveSteps)) {
    return false;
    Disable();
  };
  overdriveSteps -= config.overdriveSteps;
  if (!makeSteps(overdriveSteps)) {
    return false;
    Disable();
  };
  delay(config.holdingTime);
  currentAbsPosition = 0L;
  if (!calcStepsToRelPosition(oldPosition, &overdriveSteps)) {
    return false;
    Disable();
  };
  if (!makeSteps(overdriveSteps)) {
    return false;
    Disable();
  };
  delay(config.holdingTime);
  if (!convertRelPositionToAbs(oldPosition, &currentAbsPosition)) {
    return false;
    Disable();
  };
  Disable();
  return true;
};

// Выполнить перемещение в заданную относительную координату.
//----------------------------------------
bool Driver::GoToRelPosition(int x) {
  if (!enabled) Enable();
  long steps;
  if (!calcStepsToRelPosition(x, &steps)) {
    return false;
    Disable();
  };
  if (!makeSteps(steps)) {
    return false;
    Disable();
  };
  currentAbsPosition += steps;
  delay(config.holdingTime);
  Disable();
  return true;
};
