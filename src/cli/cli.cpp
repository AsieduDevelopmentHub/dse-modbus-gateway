#include "cli/cli.h"

#include <stdlib.h>
#include <string.h>

#include "config/config.h"
#include "devices/modbus_device.h"
#include "modbus/modbus_client.h"
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
  rs485.applyConfig(d.baudRate, d.dataBits, d.parity, d.stopBits);
}

ModbusTxnInfo Cli::makeTxnInfo() const {
  const ModbusDeviceConfig &d = devices.selected();
  ModbusTxnInfo info;
  info.deviceName = d.name;
  info.deviceSerial = d.serial;
  info.baud = d.baudRate;
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
  Serial.println(F("  read <addr> <qty> | readhex <hex> <qty>"));
  Serial.println(F("  rawread <slave> <fc> <hex_addr> <qty>"));
  Serial.println(F("  set slave <id> | set fc <03|04>"));
  Serial.println(F("  crc-test"));
  Serial.println(F("  diagnostic on|off"));
  Serial.println(F("  repeat <n> <slave> <fc> <hex_addr> <qty>"));
  Serial.println();
  Serial.println(F("Phase 2A probe (no register map assumed):"));
  Serial.println(F("  device select et1641"));
  Serial.println(F("  crc-test"));
  Serial.println(F("  diagnostic on"));
  Serial.println(F("  rawread 50 04 0000 1"));
  Serial.println(F("  rawread 50 03 0000 1"));
  Serial.println(F("  repeat 20 50 04 0000 1"));
}

void Cli::printStatus() {
  const ModbusDeviceConfig &d = devices.selected();
  Serial.println(F("Status: ready"));
  Serial.print(F("Device: "));
  Serial.println(d.name);
  Serial.print(F("Slave: "));
  Serial.println(_slaveId);
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
    Serial.println(F("Usage: set slave <id> | set fc <03|04>"));
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
  Serial.println(F("Unknown set target"));
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
  modbus.readRegisters(_slaveId, _functionCode,
                       static_cast<uint16_t>(address),
                       static_cast<uint16_t>(quantity), &info);
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
  const long slave = strtol(slaveStr, nullptr, 0);
  const long fc = strtol(fcStr, nullptr, 16);
  const long address = strtol(addrStr, nullptr, 16);
  const long quantity = strtol(qtyStr, nullptr, 10);
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
  } else if (strcasecmp(cmd, "read") == 0) {
    cmdRead(rest, false);
  } else if (strcasecmp(cmd, "readhex") == 0) {
    cmdRead(rest, true);
  } else if (strcasecmp(cmd, "rawread") == 0) {
    cmdRawRead(rest);
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
