#include <Arduino.h>

// ПРОВЕРИТЬ НАПРАВЛЕНИЕ ВРАЩЕНИЯ ЗАКРЫТИЯ НА ДРАЙВЕРЕ!
// Положительное направление вращения - в сторону открытия клапана.

#define ENABLE_PIN 4         // D4 ENABLE pin драйвера
#define STEP_PIN 3           // D3 STEP pin драйвер
#define DIR_PIN 2            // D2 DIR pin драйвера
#define BUSY_PIN LED_BUILTIN // встроенный светодиод для тестов
#define VELOCITY 100         // скорость в полных шагах в секунду
#define MICROSTEP 128        // делитель микрошага, установленный на драйвере
#define TOTAL_STEPS 600      // полное рабочее число шагов БЕЗ учета микрошага
#define OVERLOAD_RATE 5      // запас шагов при перегрузке в %. при 5 перегрузка выполняет TOTAL_STEPS*1.05 полных шагов
#define HIGH_TIME 5          // длительность состояния HIGH для STEP pin в миллисекундах
#define MAX_POSITION 1000    // Положение задвижки задатеся числом 0-X, где 0 - полностью закрыто, X - полностью открыто
#define DEBUG true           // вывод отладочной информации в консоль

struct driverConfig{
  int enablePin,
  stepPin, 
  dirPin, 
  busyPin, 
  velocity, 
  totalSteps, 
  microstep, 
  overload,
  highTime,
  maxPosition;
};

class Driver{
  public:
    // Конструктор класса
    Driver (driverConfig config){
      enablePin = config.enablePin;
      stepPin = config.stepPin;
      dirPin = config.dirPin;
      busyPin = config.busyPin;
      velocity = config.velocity;
      totalSteps = config.totalSteps;
      microstep = config.microstep;
      overload = config.overload;
      highTime = config.highTime;
      maxPosition = config.maxPosition;
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

void debugLog(String msg) {
  if (DEBUG) {
    Serial.println(msg);
  }
}

void setup()
{
  if (DEBUG) {
    Serial.begin(115200);
  }
  struct driverConfig config = {      
      ENABLE_PIN,
      STEP_PIN,
      DIR_PIN,
      BUSY_PIN,
      VELOCITY,
      TOTAL_STEPS,
      MICROSTEP,
      OVERLOAD_RATE,
      MAX_POSITION
      };
  Driver driver(config);
  driver.init();
  driver.initialOverload();
  debugLog("Driver initialization");
}

void loop() {
}

