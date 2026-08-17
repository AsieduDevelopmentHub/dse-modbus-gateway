#pragma once

#include <Arduino.h>

#include "devices/dse7320_registers.h"
#include "devices/dse_scf.h"
#include "devices/modbus_device.h"
#include "modbus/modbus_client.h"

class Cli {
public:
  void begin();
  void poll();
  void printBanner();

private:
  void printHelp();
  void printStatus();
  void printConfig();
  void handleLine(char *line);
  void cmdRead(char *args, bool hexAddress);
  void cmdRawRead(char *args);
  void cmdRs485Raw(char *args);
  void cmdRs485RawRepeat(char *args);
  void cmdSet(char *args);
  void cmdDevice(char *args);
  void cmdRepeat(char *args);
  void cmdDiagnostic(char *args);
  void cmdRegs(char *args);
  void cmdGet(char *args);
  void cmdPoll(char *args);
  void cmdAlarms(char *args);
  void cmdScf(char *args);
  void applySelectedDevice();
  ModbusTxnInfo makeTxnInfo() const;
  bool ensureDseSelected() const;
  void readAndPrintRegister(const DseRegister &reg);

  static constexpr size_t kLineMax = 96;
  char _line[kLineMax];
  size_t _len = 0;
  bool _inEsc = false;

  uint8_t _slaveId;
  uint8_t _functionCode;
  uint32_t _baudRate;
};

extern Cli cli;
