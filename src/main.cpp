#include <driver.h>
#include "ModbusRtu.h"

// Коды ошибок (регистр кодов ошибок)
//----------------------------------------
#define ERR_NO               0            // ошибок нет 
#define ERR_OVERLOAD         1            // ошибка перегрузки
#define ERR_GO_TO_POS        2            // ошибка перемещения в целевую координату
#define ERR_TOTAL_OVERLOAD   3            // ошибка полной перегрузки
#define ERR_INIT             4            // ошибка инициализации

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
    au16data[5] = au16data[2];            // выставляем целевую координату в ноль для безопасности                      
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
  } else {
    au16data[3] = ERR_INIT;
  }
}

// Основной цикл обработки
//----------------------------------------
void loop() {
  polling();
}
