#include "cli/cli.h"

#include <stdlib.h>
#include <string.h>

#include "config/config.h"
#include "devices/dse7320_registers.h"
#include "devices/dse_scf.h"
#include "devices/modbus_device.h"
#include "modbus/modbus_client.h"
#include "modbus/modbus_crc.h"
#include "rs485/rs485.h"

Cli cli;

void Cli::begin() {
  devices.begin();
  _functionCode = 0x03;
  _len = 0;
  _inEsc = false;
  applySelectedDevice();
  printBanner();
}

void Cli::applySelectedDevice() {
  const ModbusDeviceConfig &d = devices.selected();
  _slaveId = d.slaveId;
  _baudRate = d.baudRate;
  rs485.applyConfig(_baudRate, d.dataBits, d.parity, d.stopBits);
}

ModbusTxnInfo Cli::makeTxnInfo() const {
  const ModbusDeviceConfig &d = devices.selected();
  ModbusTxnInfo info;
  info.deviceName = d.name;
  info.deviceSerial = d.serial;
  info.baud = _baudRate;
  info.dataBits = d.dataBits;
  info.parity = d.parity;
  info.stopBits = d.stopBits;
  return info;
}

void Cli::printBanner() {
  const ModbusDeviceConfig &d = devices.selected();
  Serial.println();
  Serial.println(F("DSE 7320 Modbus RTU Gateway"));
  Serial.println(F("---------------------------"));
  Serial.println(F("Phase 2A diagnostics enabled via CLI"));
  Serial.println();
  Serial.print(F("Selected: "));
  Serial.println(d.name);
  Serial.print(F("Slave ID: "));
  Serial.println(_slaveId);
  Serial.print(F("Baud: "));
  Serial.println(_baudRate);
  Serial.print(F("Diagnostic: "));
  Serial.println(modbus.diagnostic() ? F("ON") : F("OFF"));
  Serial.println();
  Serial.println(F("Type 'help' / 'crc-test' to start."));
  Serial.print(F("> "));
}

void Cli::printHelp() {
  Serial.println(F("Commands:"));
  Serial.println(F("  help | status | config"));
  Serial.println(F("  devices | device list|show|select <key>"));
  Serial.println(F("  regs | registers                  (DSE map list)"));
  Serial.println(F("  get <key|#|addr>                   (named scaled read)"));
  Serial.println(F("  poll [basic|engine|power|alarms|leds|identity|scf|all]"));
  Serial.println(F("  iterate [basic|power|engine|all]   (batch readable dump)"));
  Serial.println(F("  iterate <n> [mode]                 (repeat n times)"));
  Serial.println(F("  watch [n]                          (loop: comm/regs/scf/alarms, no stop)"));
  Serial.println(F("  alarms                             (active alarms only)"));
  Serial.println(F("  scf list|flags|<cmd>|raw <key>     (GenComm control write)"));
  Serial.println(F("  set slave <id> | set fc <03|04> | set baud <4800|9600|19200>"));
  Serial.println(F("  read <addr> <qty> | readhex <hex> <qty>"));
  Serial.println(F("  rawread <slave> <fc> <hex_addr> <qty>"));
  Serial.println(F("  rs485raw <slave> <fc> <hex_addr> <qty>"));
  Serial.println(F("  rs485raw-repeat <n> <slave> <fc> <hex_addr> <qty>"));
  Serial.println(F("  crc-test"));
  Serial.println(F("  diagnostic on|off"));
  Serial.println(F("  repeat <n> <slave> <fc> <hex_addr> <qty>"));
  Serial.println();
  Serial.println(F("DSE examples:"));
  Serial.println(F("  iterate | iterate basic | iterate 3 power"));
  Serial.println(F("  watch | watch 8"));
  Serial.println(F("  get battery_v | get eng_rpm | get eng_status"));
  Serial.println(F("  set slave 1 | set baud 9600"));
  Serial.println(F("  scf auto | scf telemetry_start | scf stop"));
}

void Cli::printStatus() {
  const ModbusDeviceConfig &d = devices.selected();
  Serial.println(F("Status: ready"));
  Serial.print(F("Device: "));
  Serial.println(d.name);
  Serial.print(F("Slave: "));
  Serial.println(_slaveId);
  Serial.print(F("Baud: "));
  Serial.println(_baudRate);
  Serial.print(F("FC: 0x"));
  Serial.println(_functionCode, HEX);
  Serial.print(F("Diagnostic: "));
  Serial.println(modbus.diagnostic() ? F("ON") : F("OFF"));
}

void Cli::printConfig() {
  const ModbusDeviceConfig &d = devices.selected();
  Serial.println(F("## CURRENT DEVICE"));
  Serial.print(F("Name: "));
  Serial.println(d.name);
  Serial.print(F("Model: "));
  Serial.println(d.model);
  if (d.serial) {
    Serial.print(F("Serial: "));
    Serial.println(d.serial);
  }
  Serial.println();
  Serial.println(F("## MODBUS"));
  Serial.print(F("Slave ID: "));
  Serial.println(_slaveId);
  Serial.print(F("Baud: "));
  Serial.println(rs485.baud());
  Serial.print(F("Parity: "));
  Serial.println(DeviceRegistry::parityName(rs485.parity()));
  Serial.print(F("Stop bits: "));
  Serial.println(rs485.stopBits());
  Serial.print(F("Function: 0x"));
  if (_functionCode < 0x10) {
    Serial.print('0');
  }
  Serial.println(_functionCode, HEX);
  Serial.println();
  Serial.println(F("## RS485"));
  Serial.println(F("UART: UART1"));
  Serial.print(F("RX: GPIO"));
  Serial.println(RS485_RX_PIN);
  Serial.print(F("TX: GPIO"));
  Serial.println(RS485_TX_PIN);
  Serial.print(F("DE/RE: GPIO"));
  Serial.println(RS485_DE_RE_PIN);
  Serial.println(F("MAX485 supply: ESP32-C3 3.3V (current lab setup)"));
  Serial.println(F("USB CDC and RS485 UART1 are separate interfaces."));
}

void Cli::cmdDiagnostic(char *args) {
  char *tok = strtok(args, " \t");
  if (!tok) {
    Serial.print(F("Diagnostic: "));
    Serial.println(modbus.diagnostic() ? F("ON") : F("OFF"));
    Serial.println(F("Usage: diagnostic on|off"));
    return;
  }
  if (strcasecmp(tok, "on") == 0) {
    modbus.setDiagnostic(true);
    Serial.println(F("Diagnostic ON (verbose TX/RX timing)"));
  } else if (strcasecmp(tok, "off") == 0) {
    modbus.setDiagnostic(false);
    Serial.println(F("Diagnostic OFF"));
  } else {
    Serial.println(F("Usage: diagnostic on|off"));
  }
}

void Cli::cmdDevice(char *args) {
  char *tok = strtok(args, " \t");
  if (!tok || strcasecmp(tok, "list") == 0) {
    devices.printList();
    return;
  }
  if (strcasecmp(tok, "show") == 0) {
    devices.printSelected();
    return;
  }
  if (strcasecmp(tok, "select") == 0) {
    char *key = strtok(nullptr, " \t");
    if (!key) {
      Serial.println(F("Usage: device select <name|index>"));
      return;
    }
    if (!devices.selectByKey(key)) {
      Serial.println(F("ERROR: unknown device"));
      return;
    }
    applySelectedDevice();
    Serial.print(F("Selected "));
    Serial.println(devices.selected().name);
    devices.printSelected();
    return;
  }
  Serial.println(F("Usage: device list|show|select <key>"));
}

void Cli::cmdSet(char *args) {
  char *tok = strtok(args, " \t");
  char *val = strtok(nullptr, " \t");
  if (!tok || !val) {
    Serial.println(
        F("Usage: set slave <id> | set fc <03|04> | set baud <4800|9600|19200>"));
    return;
  }
  if (strcasecmp(tok, "slave") == 0) {
    const long id = strtol(val, nullptr, 0);
    if (id < 1 || id > 247) {
      Serial.println(F("ERROR: slave ID must be 1..247"));
      return;
    }
    _slaveId = static_cast<uint8_t>(id);
    Serial.print(F("Slave ID override: "));
    Serial.println(_slaveId);
    return;
  }
  if (strcasecmp(tok, "fc") == 0) {
    const long fc = strtol(val, nullptr, 16);
    if (fc != 0x03 && fc != 0x04) {
      Serial.println(F("ERROR: FC must be 03 or 04"));
      return;
    }
    _functionCode = static_cast<uint8_t>(fc);
    Serial.print(F("FC set to 0x"));
    Serial.println(_functionCode, HEX);
    return;
  }
  if (strcasecmp(tok, "baud") == 0) {
    const long baud = strtol(val, nullptr, 10);
    if (baud != 4800 && baud != 9600 && baud != 19200) {
      Serial.println(F("ERROR: baud must be 4800, 9600, or 19200"));
      return;
    }
    const ModbusDeviceConfig &d = devices.selected();
    _baudRate = static_cast<uint32_t>(baud);
    rs485.applyConfig(_baudRate, d.dataBits, d.parity, d.stopBits);
    Serial.print(F("UART baud set to "));
    Serial.print(_baudRate);
    Serial.println(F(" (runtime override; device select resets to profile)"));
    return;
  }
  Serial.println(F("Unknown set target"));
}

bool Cli::ensureDseSelected() const {
  if (strcasecmp(devices.selected().id, "dse7320") != 0) {
    Serial.println(F("NOTE: named DSE registers apply to dse7320."));
    Serial.println(F("      Use: device select dse7320"));
    return false;
  }
  return true;
}

void Cli::readAndPrintRegister(const DseRegister &reg) {
  const uint16_t qty = static_cast<uint16_t>(reg.width);
  ModbusTxnInfo info = makeTxnInfo();
  info.address = reg.address;
  info.quantity = qty;
  // Named reads always use FC03 holding registers (map definition).
  const ModbusResponse resp =
      modbus.readRegisters(_slaveId, 0x03, reg.address, qty, &info);
  if (resp.result != ModbusResult::Ok) {
    Serial.print(F("FAIL: "));
    Serial.print(reg.key);
    Serial.print(F(" @"));
    Serial.println(reg.address);
    return;
  }
  const DseDecodedValue decoded =
      DseRegisters::decode(reg, resp.values, resp.valueCount);
  DseRegisters::printDecoded(decoded);
}

void Cli::cmdRegs(char *args) {
  (void)args;
  DseRegisters::printList();
}

void Cli::cmdGet(char *args) {
  char *tok = strtok(args, " \t");
  if (!tok) {
    Serial.println(F("Usage: get <key|#|address>"));
    Serial.println(F("  get battery_v | get 1 | get 1029"));
    return;
  }
  if (!ensureDseSelected()) {
    return;
  }

  const DseRegister *reg = DseRegisters::findByKey(tok);
  if (!reg) {
    char *end = nullptr;
    const long n = strtol(tok, &end, 0);
    if (end && *end == '\0' && n > 0) {
      reg = DseRegisters::findByIndex1(static_cast<size_t>(n));
      if (!reg) {
        reg = DseRegisters::findByAddress(static_cast<uint16_t>(n));
      }
    }
  }
  if (!reg) {
    Serial.println(F("ERROR: unknown register (try 'regs')"));
    return;
  }

  Serial.print(F("GET "));
  DseRegisters::printEntry(*reg);
  readAndPrintRegister(*reg);
}

void Cli::cmdPoll(char *args) {
  char *tok = strtok(args, " \t");
  const char *mode = tok ? tok : "basic";
  if (tok && strcasecmp(mode, "basic") != 0 && strcasecmp(mode, "power") != 0 &&
      strcasecmp(mode, "leds") != 0 && strcasecmp(mode, "all") != 0 &&
      strcasecmp(mode, "engine") != 0 && strcasecmp(mode, "alarms") != 0 &&
      strcasecmp(mode, "identity") != 0 && strcasecmp(mode, "scf") != 0 &&
      strcasecmp(mode, "state") != 0) {
    Serial.println(
        F("Usage: poll [basic|engine|power|alarms|leds|identity|scf|state|all]"));
    return;
  }
  if (!ensureDseSelected()) {
    return;
  }

  auto want = [&](const DseRegister &r) -> bool {
    if (strcasecmp(mode, "all") == 0) {
      return true;
    }
    if (strcasecmp(mode, "leds") == 0) {
      return strncmp(r.key, "led_", 4) == 0;
    }
    if (strcasecmp(mode, "alarms") == 0) {
      return r.kind == DseDecodeKind::AlarmPacked;
    }
    if (strcasecmp(mode, "scf") == 0) {
      return strncmp(r.key, "scf_", 4) == 0;
    }
    if (strcasecmp(mode, "identity") == 0) {
      return strcmp(r.key, "mfr_code") == 0 || strcmp(r.key, "model") == 0 ||
             strcmp(r.key, "serial") == 0;
    }
    if (strcasecmp(mode, "state") == 0) {
      return strcmp(r.key, "op_mode") == 0 || strcmp(r.key, "eng_status") == 0;
    }
    if (strcasecmp(mode, "engine") == 0) {
      return strcmp(r.key, "oil_press") == 0 ||
             strcmp(r.key, "coolant_temp") == 0 ||
             strcmp(r.key, "oil_temp") == 0 || strcmp(r.key, "fuel_level") == 0 ||
             strcmp(r.key, "battery_v") == 0 ||
             strcmp(r.key, "charge_alt_v") == 0 ||
             strcmp(r.key, "eng_rpm") == 0 || strcmp(r.key, "eng_status") == 0 ||
             strcmp(r.key, "op_mode") == 0 || strcmp(r.key, "eng_hours") == 0 ||
             strcmp(r.key, "eng_starts") == 0 ||
             strcmp(r.key, "gen_pct_kw") == 0 ||
             strcmp(r.key, "oil_level") == 0 ||
             strcmp(r.key, "coolant_level") == 0;
    }
    if (strcasecmp(mode, "power") == 0) {
      return strncmp(r.key, "gen_p", 5) == 0 ||
             strncmp(r.key, "gen_q", 5) == 0 ||
             strncmp(r.key, "gen_s", 5) == 0 ||
             strncmp(r.key, "gen_pf", 6) == 0 ||
             strcmp(r.key, "gen_kwh_exp") == 0 ||
             strcmp(r.key, "gen_pct_kw") == 0;
    }
    // basic
    return strcmp(r.key, "battery_v") == 0 || strcmp(r.key, "charge_alt_v") == 0 ||
           strcmp(r.key, "eng_rpm") == 0 || strcmp(r.key, "eng_status") == 0 ||
           strcmp(r.key, "op_mode") == 0 || strcmp(r.key, "gen_freq") == 0 ||
           strcmp(r.key, "gen_van") == 0 || strcmp(r.key, "gen_vbn") == 0 ||
           strcmp(r.key, "gen_vcn") == 0 || strcmp(r.key, "gen_ia") == 0 ||
           strcmp(r.key, "gen_ib") == 0 || strcmp(r.key, "gen_ic") == 0 ||
           strcmp(r.key, "gen_p_tot") == 0 || strcmp(r.key, "gen_pf") == 0 ||
           strcmp(r.key, "gen_pct_kw") == 0 || strcmp(r.key, "oil_press") == 0 ||
           strcmp(r.key, "coolant_temp") == 0 ||
           strcmp(r.key, "fuel_level") == 0 || strcmp(r.key, "mains_freq") == 0 ||
           strcmp(r.key, "mains_van") == 0;
  };

  Serial.print(F("## DSE poll ("));
  Serial.print(mode);
  Serial.println(')');
  size_t n = 0;
  for (size_t i = 0; i < DseRegisters::count(); i++) {
    const DseRegister &r = DseRegisters::at(i);
    if (!want(r)) {
      continue;
    }
    readAndPrintRegister(r);
    n++;
    delay(20);
  }
  Serial.print(F("Polled "));
  Serial.print(n);
  Serial.println(F(" registers."));
}

bool Cli::batchReadPrint(uint16_t address, uint16_t quantity) {
  ModbusTxnInfo info = makeTxnInfo();
  info.address = address;
  info.quantity = quantity;
  info.quiet = true;
  const ModbusResponse resp =
      modbus.readRegisters(_slaveId, 0x03, address, quantity, &info);
  if (resp.result != ModbusResult::Ok) {
    Serial.print(F("BATCH FAIL @"));
    Serial.print(address);
    Serial.print(F(" qty="));
    Serial.print(quantity);
    Serial.print(F(" result="));
    Serial.println(static_cast<int>(resp.result));
    return false;
  }
  DseRegisters::printBatchReadable(address, resp.values, resp.valueCount);
  return true;
}

void Cli::cmdIterate(char *args) {
  // iterate [mode]
  // iterate <n> [mode]
  char *tok1 = strtok(args, " \t");
  char *tok2 = strtok(nullptr, " \t");
  int loops = 1;
  const char *mode = "basic";

  if (tok1) {
    char *end = nullptr;
    const long n = strtol(tok1, &end, 10);
    if (end && *end == '\0' && n >= 1) {
      loops = static_cast<int>(n > 50 ? 50 : n);
      mode = tok2 ? tok2 : "basic";
    } else {
      mode = tok1;
    }
  }

  if (strcasecmp(mode, "basic") != 0 && strcasecmp(mode, "power") != 0 &&
      strcasecmp(mode, "engine") != 0 && strcasecmp(mode, "all") != 0) {
    Serial.println(F("Usage: iterate [basic|power|engine|all]"));
    Serial.println(F("       iterate <n> [basic|power|engine|all]"));
    return;
  }
  if (!ensureDseSelected()) {
    return;
  }

  for (int pass = 0; pass < loops; pass++) {
    if (loops > 1) {
      Serial.print(F("## Iterate "));
      Serial.print(pass + 1);
      Serial.print('/');
      Serial.println(loops);
    } else {
      Serial.print(F("## DSE iterate ("));
      Serial.print(mode);
      Serial.print(F(")  slave="));
      Serial.print(_slaveId);
      Serial.print(F("  baud="));
      Serial.println(_baudRate);
    }

    // Contiguous FC03 batches (max 32 regs). Quiet TX/RX; readable lines only.
    if (strcasecmp(mode, "basic") == 0 || strcasecmp(mode, "engine") == 0 ||
        strcasecmp(mode, "all") == 0) {
      // Identity + mode: 768..775
      batchReadPrint(768, 8);
      delay(30);
      // Engine + generator instrumentation: 1024..1055 (32 regs)
      batchReadPrint(1024, 32);
      delay(30);
      // Phase C power + utility: 1056..1065
      batchReadPrint(1056, 10);
      delay(30);
      // Engine status
      batchReadPrint(1408, 1);
      delay(30);
    }

    if (strcasecmp(mode, "power") == 0 || strcasecmp(mode, "all") == 0) {
      // Derived power / PF block: 1536..1558
      batchReadPrint(1536, 23);
      delay(30);
      // Accumulators: 1798..1809
      batchReadPrint(1798, 12);
      delay(30);
    }

    if (strcasecmp(mode, "engine") == 0 || strcasecmp(mode, "all") == 0) {
      // Extended levels
      batchReadPrint(1357, 3);
      delay(30);
    }

    if (pass + 1 < loops) {
      delay(500);
    }
  }
}

bool Cli::waitOrAbort(uint32_t ms) {
  const uint32_t t0 = millis();
  while ((millis() - t0) < ms) {
    if (Serial.available()) {
      return true;
    }
    delay(20);
  }
  return false;
}

void Cli::watchOnce() {
  uint8_t ok = 0;
  uint8_t fail = 0;

  Serial.println(F("--- COMM ---"));
  Serial.print(F("Slave "));
  Serial.print(_slaveId);
  Serial.print(F("  baud "));
  Serial.print(_baudRate);
  Serial.print(F(" 8N1  FC03"));
  Serial.println();

  auto batch = [&](uint16_t addr, uint16_t qty) {
    if (batchReadPrint(addr, qty)) {
      ok++;
    } else {
      fail++;
    }
    delay(25);
  };

  Serial.println(F("--- REGS (batch) ---"));
  batch(768, 8);      // identity / op_mode
  batch(1024, 32);    // engine + gen electrical
  batch(1056, 10);    // phase C + mains
  batch(1408, 1);     // engine status
  batch(1536, 23);    // power / PF
  batch(1798, 12);    // hours / energy / starts

  Serial.println(F("--- SCF (read flags only, no writes) ---"));
  {
    ModbusTxnInfo info = makeTxnInfo();
    info.quiet = true;
    info.address = DseScf::kRegFlagsBase;
    info.quantity = DseScf::kFlagRegCount;
    const ModbusResponse resp = modbus.readRegisters(
        _slaveId, 0x03, DseScf::kRegFlagsBase, DseScf::kFlagRegCount, &info);
    if (resp.result != ModbusResult::Ok ||
        resp.valueCount < DseScf::kFlagRegCount) {
      Serial.println(F("SCF flags: FAIL"));
      fail++;
    } else {
      ok++;
      Serial.print(F("Flags:"));
      for (uint8_t i = 0; i < DseScf::kFlagRegCount; i++) {
        Serial.print(' ');
        Serial.print(F("0x"));
        Serial.print(resp.values[i], HEX);
      }
      Serial.println();
      Serial.println(F("Supported (read):"));
      for (size_t i = 0; i < DseScf::count(); i++) {
        const DseScfCommand &c = DseScf::at(i);
        if (c.code == DseScf::kKeyStop) {
          continue;  // never advertise stop as an action
        }
        if (DseScf::isSupported(c.code, resp.values)) {
          Serial.print(F("  [+] "));
          Serial.println(c.key);
        }
      }
    }
  }

  Serial.println(F("--- ALARMS (active only) ---"));
  {
    ModbusTxnInfo info = makeTxnInfo();
    info.quiet = true;
    info.address = 39425;
    info.quantity = 21;
    const ModbusResponse resp =
        modbus.readRegisters(_slaveId, 0x03, 39425, 21, &info);
    if (resp.result != ModbusResult::Ok) {
      Serial.println(F("Alarm page 154: FAIL"));
      fail++;
    } else {
      ok++;
      for (uint8_t i = 0; i < resp.valueCount; i++) {
        DseRegisters::printAlarmActive(
            static_cast<uint16_t>(39425 + i), resp.values[i]);
      }
    }
  }

  Serial.print(F("Batches OK="));
  Serial.print(ok);
  Serial.print(F(" FAIL="));
  Serial.println(fail);
  if (fail == 0) {
    Serial.println(F("LINK: OK"));
  } else {
    Serial.println(F("LINK: DEGRADED"));
  }
}

void Cli::cmdWatch(char *args) {
  char *tok = strtok(args, " \t");
  int loops = 8;
  if (tok) {
    const long n = strtol(tok, nullptr, 10);
    if (n >= 1 && n <= 40) {
      loops = static_cast<int>(n);
    } else {
      Serial.println(F("Usage: watch [n]   (n=1..40, default 8)"));
      Serial.println(F("       abort: type any key"));
      return;
    }
  }
  if (!ensureDseSelected()) {
    return;
  }

  Serial.println(F("## WATCH loop — comm / regs / scf-flags / alarms"));
  Serial.println(F("    SCF stop is HARD-BLOCKED. Type any key to abort."));
  for (int pass = 0; pass < loops; pass++) {
    Serial.println();
    Serial.print(F("======== cycle "));
    Serial.print(pass + 1);
    Serial.print('/');
    Serial.print(loops);
    Serial.println(F(" ========"));
    watchOnce();
    if (pass + 1 < loops) {
      Serial.println(F("(next in 3s, any key aborts)"));
      if (waitOrAbort(3000)) {
        Serial.println(F("WATCH aborted."));
        return;
      }
    }
  }
  Serial.println(F("WATCH complete."));
}

void Cli::cmdAlarms(char *args) {
  (void)args;
  if (!ensureDseSelected()) {
    return;
  }
  Serial.println(F("## Active DSE alarms"));
  size_t n = 0;
  for (size_t i = 0; i < DseRegisters::count(); i++) {
    const DseRegister &r = DseRegisters::at(i);
    if (r.kind != DseDecodeKind::AlarmPacked) {
      continue;
    }
    ModbusTxnInfo info = makeTxnInfo();
    info.address = r.address;
    info.quantity = 1;
    const ModbusResponse resp =
        modbus.readRegisters(_slaveId, 0x03, r.address, 1, &info);
    if (resp.result != ModbusResult::Ok || resp.valueCount < 1) {
      Serial.print(F("FAIL: "));
      Serial.println(r.key);
      continue;
    }
    DseRegisters::printAlarmActive(r.address, resp.values[0]);
    n++;
    delay(20);
  }
  Serial.print(F("Checked "));
  Serial.print(n);
  Serial.println(F(" alarm registers."));
}

void Cli::cmdScf(char *args) {
  char *tok = strtok(args, " \t");
  if (!tok || strcasecmp(tok, "list") == 0 || strcasecmp(tok, "help") == 0) {
    DseScf::printList();
    return;
  }
  if (!ensureDseSelected()) {
    return;
  }

  if (strcasecmp(tok, "flags") == 0) {
    ModbusTxnInfo info = makeTxnInfo();
    info.address = DseScf::kRegFlagsBase;
    info.quantity = DseScf::kFlagRegCount;
    const ModbusResponse resp = modbus.readRegisters(
        _slaveId, 0x03, DseScf::kRegFlagsBase, DseScf::kFlagRegCount, &info);
    if (resp.result != ModbusResult::Ok ||
        resp.valueCount < DseScf::kFlagRegCount) {
      Serial.println(F("FAIL: could not read SCF support flags"));
      return;
    }
    Serial.println(F("## SCF support flags (4096-4103)"));
    for (uint8_t i = 0; i < DseScf::kFlagRegCount; i++) {
      Serial.print(F("  scf_f"));
      Serial.print(i);
      Serial.print(F(" @"));
      Serial.print(DseScf::kRegFlagsBase + i);
      Serial.print(F(" = 0x"));
      Serial.println(resp.values[i], HEX);
    }
    Serial.println(F("Supported commands:"));
    for (size_t i = 0; i < DseScf::count(); i++) {
      const DseScfCommand &c = DseScf::at(i);
      const bool ok = DseScf::isSupported(c.code, resp.values);
      Serial.print(ok ? F("  [+] ") : F("  [-] "));
      Serial.print(c.key);
      Serial.print(F("  "));
      Serial.println(c.name);
    }
    return;
  }

  uint16_t keyCode = 0;
  const char *label = nullptr;
  if (strcasecmp(tok, "raw") == 0) {
    char *num = strtok(nullptr, " \t");
    if (!num) {
      Serial.println(F("Usage: scf raw <key>"));
      return;
    }
    keyCode = static_cast<uint16_t>(strtoul(num, nullptr, 0));
    label = "raw";
  } else {
    const DseScfCommand *cmd = DseScf::findByKey(tok);
    if (!cmd) {
      Serial.println(F("ERROR: unknown SCF command (try 'scf list')"));
      return;
    }
    keyCode = cmd->code;
    label = cmd->key;
  }

  if (keyCode == DseScf::kKeyStop ||
      (label && strcasecmp(label, "stop") == 0)) {
    Serial.println(F("BLOCKED: scf stop is disabled while the generator is running."));
    Serial.println(F("         (read-only SCF: use 'scf flags' / 'watch')"));
    return;
  }

  Serial.println(F("WARNING: SCF write can change controller mode / start DG."));
  Serial.print(F("SCF WRITE "));
  Serial.print(label);
  Serial.print(F(" key="));
  Serial.print(keyCode);
  Serial.print(F(" complement="));
  Serial.println(static_cast<uint16_t>(65535u - keyCode));

  // Best-effort support check (non-fatal if flags unread).
  {
    ModbusTxnInfo finfo = makeTxnInfo();
    const ModbusResponse flags = modbus.readRegisters(
        _slaveId, 0x03, DseScf::kRegFlagsBase, DseScf::kFlagRegCount, &finfo);
    if (flags.result == ModbusResult::Ok &&
        flags.valueCount >= DseScf::kFlagRegCount) {
      if (!DseScf::isSupported(keyCode, flags.values)) {
        Serial.println(
            F("NOTE: support flag bit is clear for this key on this unit."));
        Serial.println(F("      Sending write anyway (controller may reject)."));
      }
    }
  }

  ModbusTxnInfo info = makeTxnInfo();
  info.address = DseScf::kRegKey;
  info.quantity = 2;
  const ModbusResponse wr = DseScf::writeKey(_slaveId, keyCode, &info);
  if (wr.result == ModbusResult::Ok) {
    Serial.println(F("SCF write ACK OK"));
  } else {
    Serial.println(F("SCF write FAILED"));
  }
}

void Cli::cmdRead(char *args, bool hexAddress) {
  char *addrStr = strtok(args, " \t");
  char *qtyStr = strtok(nullptr, " \t");
  if (!addrStr || !qtyStr) {
    Serial.println(hexAddress ? F("Usage: readhex <hex> <qty>")
                              : F("Usage: read <addr> <qty>"));
    return;
  }
  const long address = strtol(addrStr, nullptr, hexAddress ? 16 : 10);
  const long quantity = strtol(qtyStr, nullptr, 10);
  if (address < 0 || address > 0xFFFF || quantity < 1 || quantity > 32) {
    Serial.println(F("ERROR: bad address/quantity"));
    return;
  }
  ModbusTxnInfo info = makeTxnInfo();
  info.address = static_cast<uint16_t>(address);
  info.quantity = static_cast<uint16_t>(quantity);
  const ModbusResponse resp = modbus.readRegisters(
      _slaveId, _functionCode, static_cast<uint16_t>(address),
      static_cast<uint16_t>(quantity), &info);
  if (resp.result == ModbusResult::Ok &&
      strcasecmp(devices.selected().id, "dse7320") == 0) {
    DseRegisters::annotateRead(static_cast<uint16_t>(address),
                               static_cast<uint16_t>(quantity), resp.values,
                               resp.valueCount);
  }
}

void Cli::cmdRawRead(char *args) {
  char *slaveStr = strtok(args, " \t");
  char *fcStr = strtok(nullptr, " \t");
  char *addrStr = strtok(nullptr, " \t");
  char *qtyStr = strtok(nullptr, " \t");
  if (!slaveStr || !fcStr || !addrStr || !qtyStr) {
    Serial.println(F("Usage: rawread <slave> <fc> <hex_addr> <qty>"));
    return;
  }
  if (modbus.diagnostic()) {
    Serial.println(F("RAW COMMAND TOKENS (rawread):"));
    Serial.print(F("TOKEN 0: "));
    Serial.println(slaveStr);
    Serial.print(F("TOKEN 1: "));
    Serial.println(fcStr);
    Serial.print(F("TOKEN 2: "));
    Serial.println(addrStr);
    Serial.print(F("TOKEN 3: "));
    Serial.println(qtyStr);
  }
  const long slave = strtol(slaveStr, nullptr, 0);
  const long fc = strtol(fcStr, nullptr, 16);
  const long address = strtol(addrStr, nullptr, 16);
  const long quantity = strtol(qtyStr, nullptr, 10);
  if (modbus.diagnostic()) {
    Serial.print(F("PARSED: slave="));
    Serial.print(slave);
    Serial.print(F(" function="));
    Serial.print(fc, HEX);
    Serial.print(F(" address="));
    Serial.print(address, HEX);
    Serial.print(F(" quantity="));
    Serial.println(quantity);
  }
  if (slave < 1 || slave > 247 || (fc != 0x03 && fc != 0x04) ||
      address < 0 || address > 0xFFFF || quantity < 1 || quantity > 32) {
    Serial.println(F("ERROR: bad rawread args"));
    return;
  }
  ModbusTxnInfo info = makeTxnInfo();
  info.deviceName = "rawread";
  modbus.readRegisters(static_cast<uint8_t>(slave), static_cast<uint8_t>(fc),
                       static_cast<uint16_t>(address),
                       static_cast<uint16_t>(quantity), &info);
}

void Cli::cmdRs485Raw(char *args) {
  char *slaveStr = strtok(args, " \t");
  char *fcStr = strtok(nullptr, " \t");
  char *addrStr = strtok(nullptr, " \t");
  char *qtyStr = strtok(nullptr, " \t");
  if (!slaveStr || !fcStr || !addrStr || !qtyStr) {
    Serial.println(F("Usage: rs485raw <slave> <fc> <hex_addr> <qty>"));
    return;
  }
  const long slave = strtol(slaveStr, nullptr, 0);
  const long fc = strtol(fcStr, nullptr, 16);
  const long address = strtol(addrStr, nullptr, 16);
  const long quantity = strtol(qtyStr, nullptr, 10);
  if (slave < 1 || slave > 247 || (fc != 0x03 && fc != 0x04) ||
      address < 0 || address > 0xFFFF || quantity < 1 || quantity > 32) {
    Serial.println(F("ERROR: bad rs485raw args"));
    return;
  }
  modbus.rawCapture(static_cast<uint8_t>(slave), static_cast<uint8_t>(fc),
                    static_cast<uint16_t>(address),
                    static_cast<uint16_t>(quantity));
}

void Cli::cmdRs485RawRepeat(char *args) {
  char *nStr = strtok(args, " \t");
  char *slaveStr = strtok(nullptr, " \t");
  char *fcStr = strtok(nullptr, " \t");
  char *addrStr = strtok(nullptr, " \t");
  char *qtyStr = strtok(nullptr, " \t");
  if (!nStr || !slaveStr || !fcStr || !addrStr || !qtyStr) {
    Serial.println(
        F("Usage: rs485raw-repeat <n> <slave> <fc> <hex_addr> <qty>"));
    return;
  }
  const long n = strtol(nStr, nullptr, 10);
  const long slave = strtol(slaveStr, nullptr, 0);
  const long fc = strtol(fcStr, nullptr, 16);
  const long address = strtol(addrStr, nullptr, 16);
  const long quantity = strtol(qtyStr, nullptr, 10);
  if (n < 1 || n > 100 || slave < 1 || slave > 247 ||
      (fc != 0x03 && fc != 0x04) || address < 0 || address > 0xFFFF ||
      quantity < 1 || quantity > 32) {
    Serial.println(F("ERROR: bad rs485raw-repeat args"));
    return;
  }

  uint16_t zero = 0, five = 0, other = 0, crcOk = 0, crcBad = 0;
  for (long i = 0; i < n; i++) {
    Serial.print(F("--- rs485raw-repeat "));
    Serial.print(i + 1);
    Serial.print('/');
    Serial.println(n);
    const RawRs485Capture cap = modbus.rawCapture(
        static_cast<uint8_t>(slave), static_cast<uint8_t>(fc),
        static_cast<uint16_t>(address), static_cast<uint16_t>(quantity));
    if (cap.rxLen == 0) {
      zero++;
    } else if (cap.rxLen == 5) {
      five++;
      if (modbusCrcOk(cap.rx, 5)) {
        crcOk++;
      } else {
        crcBad++;
      }
    } else {
      other++;
      if (cap.rxLen >= 4 && modbusCrcOk(cap.rx, cap.rxLen)) {
        crcOk++;
      } else {
        crcBad++;
      }
    }
    delay(50);
  }

  Serial.println();
  Serial.println(F("## RS485 RAW REPEAT SUMMARY"));
  Serial.print(F("Attempts: "));
  Serial.println(n);
  Serial.print(F("0-byte timeout: "));
  Serial.println(zero);
  Serial.print(F("5-byte response: "));
  Serial.println(five);
  Serial.print(F("other-length response: "));
  Serial.println(other);
  Serial.print(F("CRC valid: "));
  Serial.println(crcOk);
  Serial.print(F("CRC invalid: "));
  Serial.println(crcBad);
}

void Cli::cmdRepeat(char *args) {
  char *nStr = strtok(args, " \t");
  char *slaveStr = strtok(nullptr, " \t");
  char *fcStr = strtok(nullptr, " \t");
  char *addrStr = strtok(nullptr, " \t");
  char *qtyStr = strtok(nullptr, " \t");
  if (!nStr || !slaveStr || !fcStr || !addrStr || !qtyStr) {
    Serial.println(F("Usage: repeat <n> <slave> <fc> <hex_addr> <qty>"));
    return;
  }

  const long n = strtol(nStr, nullptr, 10);
  const long slave = strtol(slaveStr, nullptr, 0);
  const long fc = strtol(fcStr, nullptr, 16);
  const long address = strtol(addrStr, nullptr, 16);
  const long quantity = strtol(qtyStr, nullptr, 10);
  if (n < 1 || n > 100 || slave < 1 || slave > 247 ||
      (fc != 0x03 && fc != 0x04) || address < 0 || address > 0xFFFF ||
      quantity < 1 || quantity > 32) {
    Serial.println(F("ERROR: bad repeat args (n=1..100)"));
    return;
  }

  uint16_t ok = 0, exc = 0, crc = 0, partial = 0, timeout = 0, unexp = 0,
           other = 0;
  ModbusTxnInfo info = makeTxnInfo();
  info.deviceName = "repeat";

  // Temporarily quieter? Keep full diagnostic per request for Phase 2A.
  for (long i = 0; i < n; i++) {
    Serial.print(F("--- repeat "));
    Serial.print(i + 1);
    Serial.print(F("/"));
    Serial.println(n);
    const ModbusResponse r = modbus.readRegisters(
        static_cast<uint8_t>(slave), static_cast<uint8_t>(fc),
        static_cast<uint16_t>(address), static_cast<uint16_t>(quantity),
        &info);
    switch (r.result) {
      case ModbusResult::Ok:
        ok++;
        break;
      case ModbusResult::Exception:
        exc++;
        break;
      case ModbusResult::CrcError:
        crc++;
        break;
      case ModbusResult::Partial:
        partial++;
        break;
      case ModbusResult::Timeout:
        timeout++;
        break;
      case ModbusResult::UnexpectedFunction:
        unexp++;
        break;
      default:
        other++;
        break;
    }
    delay(50);
  }

  Serial.println();
  Serial.println(F("## MODBUS REPEAT TEST"));
  Serial.print(F("Requests: "));
  Serial.println(n);
  Serial.print(F("Valid responses: "));
  Serial.println(ok);
  Serial.print(F("Valid exceptions: "));
  Serial.println(exc);
  Serial.print(F("CRC errors: "));
  Serial.println(crc);
  Serial.print(F("Partial frames: "));
  Serial.println(partial);
  Serial.print(F("Timeouts: "));
  Serial.println(timeout);
  Serial.print(F("Unexpected function: "));
  Serial.println(unexp);
  Serial.print(F("Other errors: "));
  Serial.println(other);
}

void Cli::handleLine(char *line) {
  while (*line == ' ' || *line == '\t') {
    line++;
  }
  if (*line == '\0') {
    return;
  }

  char *cmd = strtok(line, " \t");
  char *rest = strtok(nullptr, "");
  if (rest) {
    while (*rest == ' ' || *rest == '\t') {
      rest++;
    }
  }
  char empty[] = "";
  if (!rest) {
    rest = empty;
  }

  if (modbus.diagnostic()) {
    Serial.print(F("RAW COMMAND: "));
    Serial.println(line);
    Serial.print(F("TOKEN CMD: "));
    Serial.println(cmd);
    if (rest[0]) {
      Serial.print(F("TOKEN REST: "));
      Serial.println(rest);
    }
  }

  if (strcasecmp(cmd, "help") == 0 || strcasecmp(cmd, "?") == 0) {
    printHelp();
  } else if (strcasecmp(cmd, "status") == 0) {
    printStatus();
  } else if (strcasecmp(cmd, "config") == 0) {
    printConfig();
  } else if (strcasecmp(cmd, "devices") == 0) {
    devices.printList();
  } else if (strcasecmp(cmd, "device") == 0) {
    cmdDevice(rest);
  } else if (strcasecmp(cmd, "regs") == 0 ||
             strcasecmp(cmd, "registers") == 0) {
    cmdRegs(rest);
  } else if (strcasecmp(cmd, "get") == 0) {
    cmdGet(rest);
  } else if (strcasecmp(cmd, "poll") == 0) {
    cmdPoll(rest);
  } else if (strcasecmp(cmd, "iterate") == 0) {
    cmdIterate(rest);
  } else if (strcasecmp(cmd, "watch") == 0 || strcasecmp(cmd, "loop") == 0) {
    cmdWatch(rest);
  } else if (strcasecmp(cmd, "alarms") == 0) {
    cmdAlarms(rest);
  } else if (strcasecmp(cmd, "scf") == 0) {
    cmdScf(rest);
  } else if (strcasecmp(cmd, "read") == 0) {
    cmdRead(rest, false);
  } else if (strcasecmp(cmd, "readhex") == 0) {
    cmdRead(rest, true);
  } else if (strcasecmp(cmd, "rawread") == 0) {
    cmdRawRead(rest);
  } else if (strcasecmp(cmd, "rs485raw") == 0) {
    cmdRs485Raw(rest);
  } else if (strcasecmp(cmd, "rs485raw-repeat") == 0) {
    cmdRs485RawRepeat(rest);
  } else if (strcasecmp(cmd, "set") == 0) {
    cmdSet(rest);
  } else if (strcasecmp(cmd, "crc-test") == 0 ||
             strcasecmp(cmd, "crctest") == 0) {
    modbus.runCrcSelfTest();
  } else if (strcasecmp(cmd, "diagnostic") == 0) {
    cmdDiagnostic(rest);
  } else if (strcasecmp(cmd, "repeat") == 0) {
    cmdRepeat(rest);
  } else {
    Serial.print(F("Unknown command: "));
    Serial.println(cmd);
  }
}

void Cli::poll() {
  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());

    // Ignore ANSI / CSI escape sequences (arrow keys etc.)
    if (c == 0x1B) {
      _inEsc = true;
      continue;
    }
    if (_inEsc) {
      if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '~') {
        _inEsc = false;
      }
      continue;
    }
    // Bare CSI leftovers like "[A" without ESC — drop if line starts with [
    // and looks like cursor codes when Enter pressed (handled as unknown).

    if (c == '\r' || c == '\n') {
      if (c == '\n' && _len == 0) {
        continue;
      }
      _line[_len] = '\0';
      Serial.println();
      if (_len > 0) {
        handleLine(_line);
      }
      _len = 0;
      Serial.print(F("> "));
      continue;
    }

    if (c == 0x08 || c == 0x7F) {
      if (_len > 0) {
        _len--;
        Serial.print(F("\b \b"));
      }
      continue;
    }

    if (c < 0x20 || c > 0x7E) {
      continue;
    }

    if (_len + 1 < kLineMax) {
      _line[_len++] = c;
      Serial.write(static_cast<uint8_t>(c));
      Serial.flush();
    }
  }
}
