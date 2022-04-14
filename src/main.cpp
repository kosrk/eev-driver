#include <driver.h>
#include "ModbusRtu.h"

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

// Карта Modbus регистров прибора
//---------------------------------------------------------------------------------------------------------------
// регистр      | бит | название | тип     | modbus адрес | доступ     | назначение
// --------------------------------------------------------------------------------------------------------------
// au16data[0]  | 0   | DT0      | coil    | 0            | read       | 1 - прибор готов, 0 - не готов
// au16data[1]  | 0   | CL16     | coil    | 17           | read/write | 1 - выполнить перегрузку
// au16data[1]  | 1   | CL17     | coil    | 18           | read/write | 1 - начать движение к целевой координате
// au16data[1]  | 2   | CL18     | coil    | 19           | read/write | 1 - выполнить полную перегрузку
// au16data[2]  |     | INPT3    | input   | 2            | read       | чтение текущего положения задвижки
// au16data[3]  |     | INPT4    | input   | 3            | read       | чтение кода ошибки
// au16data[5]  |     | HOLD6    | holding | 5            | read/write | записать целевую координату

// Коды ошибок (регистр кода ошибок)
//----------------------------------------
#define ERR_NO               0            // ошибок нет 
#define ERR_OVERLOAD         1            // ошибка перегрузки
#define ERR_GO_TO_POS        2            // ошибка перемещения в целевую координату
#define ERR_TOTAL_OVERLOAD   3            // ошибка полной перегрузки

// Параметры привода
//----------------------------------------
#define ENABLE_PIN           4            // D4 ENABLE pin драйвера
#define STEP_PIN             3            // D3 STEP pin драйвер
#define DIR_PIN              2            // D2 DIR pin драйвера
#define BUSY_PIN             LED_BUILTIN  // встроенный светодиод для тестов
#define VELOCITY             240          // скорость в полных шагах в секунду
#define MICROSTEPS           32           // делитель микрошага, установленный на драйвере
#define TOTAL_STEPS          600          // полное рабочее число шагов БЕЗ учета микрошага
#define OVERDRIVE_STEPS      6            // запас полных шагов перегрузки
#define INIT_OVERDRIVE_STEPS 28           // запас полных шагов перегрузки для первичной инициализации
#define HIGH_TIME            5            // длительность состояния HIGH для STEP pin в микросекундах
#define MAX_REL_POSITION     1000         // Положение задвижки задатеся числом 0-X, где 0 - полностью закрыто, X - полностью открыто
#define HOLDING_TIME         10           // Время (в миллисекундах) до отключения питания обмоток после последнего шага

// Параметры сети
//----------------------------------------
#define MODBUS_ADDR          1            // адрес устройства в сети Modbus
#define MODBUS_TX_CONTROL    0            // номер выхода управления TX
#define MODBUS_SPEED         9600         // скорость передачи данных по RS-485

// Параметры отладки
//----------------------------------------
#define DEBUG                false        // вывод отладочной информации в консоль

// Функция вывода отладочной информации
//----------------------------------------
void debugLog(String msg) {
  if (DEBUG) {
    Serial.println(msg);
  }
}

// Глобальные переменные
//----------------------------------------
bool initFlag = false;                                  // флаг инициализации устройства
Modbus modbus(MODBUS_ADDR, Serial, MODBUS_TX_CONTROL);  // ведомое modbus устройство
uint16_t au16data[11] = {0};                            // массив регистров Modbus
Driver driver;

// Инициализация привода
//----------------------------------------
bool driverInit(Driver *driver) {
  struct driverConfig config = {      
      ENABLE_PIN,
      STEP_PIN,
      DIR_PIN,
      BUSY_PIN,
      VELOCITY,
      MICROSTEPS,
      TOTAL_STEPS,
      OVERDRIVE_STEPS,
      INIT_OVERDRIVE_STEPS,
      HIGH_TIME,
      MAX_REL_POSITION,
      HOLDING_TIME
      };
  if (!ValidateConfig(config)) {
    debugLog("Invalid driver config");
    return false;
  };     
  Driver drv;  
  if (!drv.Init(config)) {
    debugLog("Driver init failed");
    return false;
  };
  if (!drv.InitialOverdrive()) {
    debugLog("Initial overdrive failed");
    return false;
  };
  debugLog("Driver initialization finished"); 
  *driver = drv;
  return true;
};

// Инициализация сети
//----------------------------------------
bool modbusInit() {
  Serial.begin(MODBUS_SPEED);
  modbus.start();
  return true;
};

// Планировщик
//----------------------------------------
bool polling() {
  int8_t state;
  state = modbus.poll(au16data, 11);
  if (state <= 4) {                        // state > 4 - получен пакет без ошибок 
    return false;
  }
  if (bitRead(au16data[1],0)) {            // команда выполнить перегрузку
    if (!driver.Overdrive()) {
      au16data[3] = ERR_OVERLOAD;
      bitWrite(au16data[0], 0, false);
      return false;
    };
    au16data[2] = driver.GetRelPosition(); // записать текущую координату
    bitWrite(au16data[1], 0, false);
  }
  if (bitRead(au16data[1],1)) {            // команда выполнить перемещение в целевую координату
    uint16_t pos;
    pos = au16data[5];                     // получение целевой координаты из регистра
    if (!driver.GoToRelPosition(pos)) {
      au16data[3] = ERR_GO_TO_POS;
      bitWrite(au16data[0], 0, false);
      return false;
    };
    au16data[2] = driver.GetRelPosition();
    bitWrite(au16data[1], 1, false);
  }
  if (bitRead(au16data[1],2)) {            // команда выполнить полную перегрузку
    if (!driver.InitialOverdrive()) {
      au16data[3] = ERR_TOTAL_OVERLOAD;
      bitWrite(au16data[0], 0, false);
      return false;
    };
    au16data[3] = ERR_NO;
    bitWrite(au16data[0], 0, true);
    au16data[2] = driver.GetRelPosition();
    bitWrite(au16data[1], 2, false);
  }
  return true;
};

// Инициализация
//----------------------------------------
void setup()
{
  initFlag = modbusInit() && driverInit(&driver);
  if (initFlag) {
    bitWrite(au16data[0], 0, true); // прибор готов
  }
}

// Основной цикл обработки
//----------------------------------------
void loop() {
  polling();
}
