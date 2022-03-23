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
  maxPosition,
  holdingTime;
};

class Driver{
  public:
    // Конструктор класса
    Driver (driverConfig config){
      config = driverConfig;
      absPosition = 0;
      busy = false;
      ready = false;
      enabled = false;
      stepTimeout = ceil(1 / (velocity * microstep)) - highTime;
      if (stepTimeout < 5) stepTimeout = 5;
    }
    // Инициализация
    void init() {
        pinMode(enablePin, OUTPUT);
        pinMode(stepPin, OUTPUT);
        pinMode(dirPin, OUTPUT);
        pinMode(busyPin, OUTPUT);
    }
    // Подать питание на мотор
    bool enable() {
      digitalWrite(enablePin, HIGH);
      enabled = true;
      return true;
    };
    // Отключить питание мотора
    bool disable() {
      digitalWrite(enablePin, LOW);
      enabled = false;
      return true;
    };
    // Выполнить перегрузку при неизвестном положении. Сбросить положение на 0.
    // Положительное направление вращения - в сторону открытия клапана.
    bool initialOverload() {
      if (!enabled) {
        return true;
      };
      int overloadSteps = ceil(totalSteps*microstep*(100.0+overload)/100);
      makeSteps(overloadSteps, true);
      absPosition = 0;
      ready = true;
      return true;
    };
  private:
    driverConfig config;
    int enablePin, stepPin, dirPin, busyPin;
    int absPosition, velocity, totalSteps, microstep, overload, stepTimeout, highTime, maxPosition;
    bool busy, ready, enabled;
    // Перейти в состояние "занят" 
    void setBusy() {
      busy = true;
      digitalWrite(busyPin, HIGH);
    };
    // Перейти в состояние "свободен"
    void setNotBusy() {
      busy = false;
      digitalWrite(busyPin, LOW);
    };
    // Пересчет относительной позиции в абсолютную
    int convertRelPosition(int relPosition) {
      int absPos = ceil(1.0*relPosition/maxPosition)*totalSteps*microstep;
    };
    // Вычислить число шагов
    int calculateSteps(int targetRelPosition) {
      // TODO
      int mul = 0;
      return 0;
    };
    // Выполнить шаги
    bool makeSteps(int steps, bool reverse) {
      setBusy();
      if (reverse) {
        digitalWrite(dirPin, HIGH);
      }
      for (int i = 0; i < steps; i++) {
        digitalWrite(stepPin, HIGH);
        delay(highTime);
        digitalWrite(stepPin, LOW);
        delay(stepTimeout);
      };
      if (reverse) {
        digitalWrite(dirPin, LOW);
      }
      setNotBusy();
      return true;
    };
};