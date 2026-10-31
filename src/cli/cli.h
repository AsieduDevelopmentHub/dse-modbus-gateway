#pragma once

#include <Arduino.h>

#include "console/tty.h"
#include "devices/dse7320_registers.h"
#include "devices/dse_scf.h"
#include "devices/modbus_device.h"
#include "modbus/modbus_client.h"

enum class ConsoleMode : uint8_t {
  LoginUser,
  LoginPass,
  Shell,
  AskCur,
  AskNew,
  AskConfirm,
  Editor,
};

class Cli {
public:
  void begin();
  void present();
  void poll();
  void onHostDown();
  bool isShell() const { return _mode == ConsoleMode::Shell; }

private:
  void showPrompt();
  void buildPrompt(char *out, size_t n) const;
  bool secretMode() const;
  bool sessionOpen() const;
  void cancelEntry();
  void dropSession(bool tell);
  void onSubmitted(char *line);
  void adoptSettings();
  bool persistSetting(const char *key, const char *value);

  void printBrandIntro();
  void printHelp();
  void printStatus();
  void printConfig();
  bool dispatchShell(const char *cmd, char *rest);
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
  void cmdWrite(char *args, bool single);
  void cmdIterate(char *args);
  void cmdWatch(char *args);
  void cmdDcem(char *args);
  void cmdEnvolt(char *args);
  void cmdLogout();
  void cmdPasswd(char *rest);
  void cmdUser(char *rest);
  void cmdWhoami();
  void cmdHostname(char *rest);
  void cmdClear();
  void cmdUptime();
  void cmdReboot(char *rest);
  void cmdInfo(char *rest);
  void cmdWifi(char *rest);
  void cmdPwd();
  void cmdLs(char *rest);
  void cmdCd(char *rest);
  void cmdCat(char *rest);
  void cmdMkdir(char *rest);
  void cmdRmdir(char *rest);
  void cmdRm(char *rest);
  void cmdCp(char *rest);
  void cmdMv(char *rest);
  void cmdEdit(char *rest);
  void cmdConfig(char *rest);
  void applySelectedDevice();
  ModbusTxnInfo makeTxnInfo() const;
  bool ensureDseSelected() const;
  void readAndPrintRegister(const DseRegister &reg);
  bool batchReadPrint(uint16_t address, uint16_t quantity);
  void watchOnce();
  bool waitOrAbort(uint32_t ms);

  ConsoleMode _mode = ConsoleMode::Shell;
  char _loginUser[16] = {};
  char _pwNew[33] = {};
  char _deferredLine[Tty::kCap] = {};
  bool _addingUser = false;
  bool _inPoll = false;
  bool _hasDeferredLine = false;
  bool _deferredInterrupt = false;
  bool _deferredLogout = false;
  uint32_t _lastInputMs = 0;

  uint8_t _slaveId = 1;
  uint8_t _functionCode = 0x03;
  uint32_t _baudRate = 9600;
};

extern Cli cli;
