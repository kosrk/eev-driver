#include <Arduino.h>
#include <driver.h>

// ПРОВЕРИТЬ НАПРАВЛЕНИЕ ВРАЩЕНИЯ ЗАКРЫТИЯ НА ДРАЙВЕРЕ!
// Положительное направление вращения - в сторону открытия клапана.

// Рекомендуемые параметры для ТРВ Danfoss ETS 24C-22 Colibri:
// -----------------------------------------------------------
// Stepper motor type:            Bi-polar - permanent magnet
// Step mode:                     Microstepping (recommended), 2 phase full step or half step
// Phase current:                 800 mA peak / 600 mA RMS
// Holding current:               No permanent holding current needed. Max. 20% permanent holding current
//                                allowed with refrigerant flow through valve
//                                For optimal performance, driver should keep 100% current on coils 10ms
//                                after last step.
// Phase resistance:              10 Ω ±10% at +20 °C / +68 °F
// Inductance:                    14 mH ±25%
// Duty cycle:                    100% possible, requiring refrigerant flow through valve
//                                Less than 50% over 120 sec period recommended
// Nominal Power consumption:     7.44 W RMS at 20 °C (total, both coils)
// Total number of full steps:    600
// Step rate:                     Current control driver:
//                                a. Step type: Microstep (1⁄4 th or higher): 240 full steps/sec. recommended
//                                b. Step type: Full step or Half steps: 240 full steps/sec. recommended
//                                Emergency close : 240 full steps/sec.
// Step translation:              0.0167 mm / step
// Full travel time:              2.5 at 240 steps / sec
// Opening stroke :               10 mm / 0.4 in
// Reference position :           Overdriving against the full close position
// Overdriving performance:       1% (6 full steps) Overdrive is recommended for optimum performance
//                                628 steps in closing direction recommended for initialisation
//                                Overdriving in open position not recommended

#define ENABLE_PIN           4            // D4 ENABLE pin драйвера
#define STEP_PIN             3            // D3 STEP pin драйвер
#define DIR_PIN              2            // D2 DIR pin драйвера
#define BUSY_PIN             LED_BUILTIN  // встроенный светодиод для тестов
#define VELOCITY             240          // скорость в полных шагах в секунду
#define MICROSTEPS           128          // делитель микрошага, установленный на драйвере
#define TOTAL_STEPS          600          // полное рабочее число шагов БЕЗ учета микрошага
#define OVERDRIVE_STEPS      6            // запас полных шагов перегрузки
#define INIT_OVERDRIVE_STEPS 28           // запас полных шагов перегрузки для первичной инициализации
#define HIGH_TIME            5            // длительность состояния HIGH для STEP pin в миллисекундах
#define MAX_POSITION         1000         // Положение задвижки задатеся числом 0-X, где 0 - полностью закрыто, X - полностью открыто
#define HOLDING_TIME         10           // Время (в мс) до отключения питания обмоток после последнего шага
#define DEBUG                true         // вывод отладочной информации в консоль

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

