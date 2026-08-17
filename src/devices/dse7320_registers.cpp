#include "devices/dse7320_registers.h"

#include <string.h>

#include <Arduino.h>

namespace {

// Macro helpers for table density
#define R16S(addr, fac, key, name, unit, spn)                                  \
  {addr, DseDataWidth::Int16, DseDecodeKind::Scaled, true, fac, key, name,     \
   unit, spn}
#define R16U(addr, fac, key, name, unit, spn)                                  \
  {addr, DseDataWidth::Int16, DseDecodeKind::Scaled, false, fac, key, name,    \
   unit, spn}
#define R32S(addr, fac, key, name, unit, spn)                                  \
  {addr, DseDataWidth::Int32, DseDecodeKind::Scaled, true, fac, key, name,     \
   unit, spn}
#define R32U(addr, fac, key, name, unit, spn)                                  \
  {addr, DseDataWidth::Int32, DseDecodeKind::Scaled, false, fac, key, name,    \
   unit, spn}

const DseRegister kMap[] = {
    // --- Identity (GenComm page 3) ---
    {768, DseDataWidth::Int16, DseDecodeKind::IdentityU16, false, 1.0f,
     "mfr_code", "Manufacturer Code", "", 0},
    {769, DseDataWidth::Int16, DseDecodeKind::IdentityU16, false, 1.0f, "model",
     "Model Number", "", 0},
    {770, DseDataWidth::Int32, DseDecodeKind::IdentityU32, false, 1.0f, "serial",
     "Serial Number", "", 0},

    // --- Generator / controller state ---
    {772, DseDataWidth::Int16, DseDecodeKind::EnumOpMode, false, 1.0f, "op_mode",
     "Operating Mode / Switch State", "", 0},
    {774, DseDataWidth::Int16, DseDecodeKind::IdentityU16, false, 1.0f,
     "ctrl_status", "Control Unit Status", "", 0},
    {785, DseDataWidth::Int16, DseDecodeKind::IdentityU16, false, 1.0f,
     "chg_stages", "Battery Charger Stages", "", 0},
    {795, DseDataWidth::Int16, DseDecodeKind::IdentityU16, false, 1.0f,
     "chg_status", "Battery Charger Status", "", 0},
    {1408, DseDataWidth::Int16, DseDecodeKind::EnumEngStatus, false, 1.0f,
     "eng_status", "Engine Operating State", "", 0},
    {1418, DseDataWidth::Int16, DseDecodeKind::EnumEngStatus, false, 1.0f,
     "eng_status_alt", "Engine State (alt)", "", 0},

    // --- Comm / misc status (7320MKII clone JSON) ---
    {6, DseDataWidth::Int16, DseDecodeKind::IdentityU16, false, 1.0f,
     "password_status", "Password Status", "", 0},
    {128, DseDataWidth::Int32, DseDecodeKind::Bitmask, false, 1.0f, "gen_status",
     "Generator Status (packed)", "", 0},
    {258, DseDataWidth::Int16, DseDecodeKind::IdentityU16, false, 1.0f,
     "device_ident", "Device Identity", "", 0},

    // --- Engine / mechanical ---
    R16U(1024, 1.0f, "oil_press", "Oil Pressure", "kPa", 0),
    R16S(1025, 1.0f, "coolant_temp", "Coolant Temperature", "C", 0),
    R16S(1026, 1.0f, "oil_temp", "Oil Temperature", "C", 0),
    R16U(1027, 1.0f, "fuel_level", "Fuel / Tank Level", "%", 0),
    R16U(1028, 0.1f, "charge_alt_v", "Charger Alternator Voltage", "V", 0),
    R16U(1029, 0.1f, "battery_v", "Battery / Starter Voltage", "V", 167),
    R16U(1030, 1.0f, "eng_rpm", "Engine Speed", "rpm", 589),
    R16U(1357, 1.0f, "oil_level", "Oil Level", "%", 0),
    R16U(1359, 1.0f, "coolant_level", "Coolant Level", "%", 0),
    R32S(1256, 0.001f, "chg_i_out", "Battery Charger Output Current", "A", 0),
    R32S(1258, 0.001f, "chg_v_out", "Battery Charger Output Voltage", "V", 0),

    // --- AC electrical (S6 + GenComm) ---
    R16U(1031, 0.1f, "gen_freq", "Generator Average AC Frequency", "Hz", 2436),
    R32U(1032, 0.1f, "gen_van", "Generator Phase A Line-Neutral AC RMS Voltage",
         "V", 2445),
    R32U(1034, 0.1f, "gen_vbn", "Generator Phase B Line-Neutral AC RMS Voltage",
         "V", 2446),
    R32U(1036, 0.1f, "gen_vcn", "Generator Phase C Line-Neutral AC RMS Voltage",
         "V", 2447),
    R32U(1038, 0.1f, "gen_vab", "Generator Phase AB Line-Line AC RMS Voltage",
         "V", 2441),
    R32U(1040, 0.1f, "gen_vbc", "Generator Phase BC Line-Line AC RMS Voltage",
         "V", 2442),
    R32U(1042, 0.1f, "gen_vca", "Generator Phase CA Line-Line AC RMS Voltage",
         "V", 2443),
    R32U(1044, 0.1f, "gen_ia", "Generator Phase A AC RMS Current", "A", 2449),
    R32U(1046, 0.1f, "gen_ib", "Generator Phase B AC RMS Current", "A", 2450),
    R32U(1048, 0.1f, "gen_ic", "Generator Phase C AC RMS Current", "A", 2451),
    R32S(1052, 1.0f, "gen_pa", "Generator Phase A Real Power", "W", 2453),
    R32S(1054, 1.0f, "gen_pb", "Generator Phase B Real Power", "W", 2454),
    R32S(1056, 1.0f, "gen_pc", "Generator Phase C Real Power", "W", 2455),
    // Utility / mains (page 4) — from 7320MKII clone JSON
    R16U(1059, 0.1f, "mains_freq", "Utility / Mains Frequency", "Hz", 0),
    R16U(1060, 0.1f, "mains_van", "Utility Voltage L1", "V", 0),
    R16U(1061, 0.1f, "mains_van_test", "Utility Voltage L1 (test/alt)", "V", 0),
    R32U(1062, 0.1f, "mains_vbn", "Utility Voltage L2", "V", 0),
    R32U(1064, 0.1f, "mains_vcn", "Utility Voltage L3", "V", 0),
    R32U(1666, 0.1f, "gen_i_avg", "Generator Average AC RMS Current", "A",
         2448),
    R32S(1536, 1.0f, "gen_p_tot", "Generator Total Real Power", "W", 2452),
    R32S(1538, 1.0f, "gen_sa", "Generator Phase A Apparent Power", "VA", 2461),
    R32S(1540, 1.0f, "gen_sb", "Generator Phase B Apparent Power", "VA", 2462),
    R32S(1542, 1.0f, "gen_sc", "Generator Phase C Apparent Power", "VA", 2463),
    R32S(1544, 1.0f, "gen_s_tot", "Generator Total Apparent Power", "VA", 2460),
    R32S(1546, 1.0f, "gen_qa", "Generator Phase A Reactive Power", "VAr", 2457),
    R32S(1548, 1.0f, "gen_qb", "Generator Phase B Reactive Power", "VAr", 2458),
    R32S(1550, 1.0f, "gen_qc", "Generator Phase C Reactive Power", "VAr", 2459),
    R32S(1552, 1.0f, "gen_q_tot", "Generator Total Reactive Power", "VAr",
         2456),
    R16S(1554, 0.01f, "gen_pfa", "Generator Phase A Power Factor", "", 2465),
    R16S(1555, 0.01f, "gen_pfb", "Generator Phase B Power Factor", "", 2466),
    R16S(1556, 0.01f, "gen_pfc", "Generator Phase C Power Factor", "", 2467),
    R16S(1557, 0.01f, "gen_pf", "Generator Overall Power Factor", "", 2464),
    R16S(1558, 0.1f, "gen_pct_kw", "Engine / Generator Load Percent", "%",
         3590),

    // --- Accumulators ---
    {1798, DseDataWidth::Int32, DseDecodeKind::HoursFromSeconds, false, 1.0f,
     "eng_hours", "Engine Operating Hours", "h", 0},
    R32U(1800, 0.1f, "gen_kwh_exp", "AC Energy Forward / kWh Export", "kWh",
         2468),
    R32U(1808, 1.0f, "eng_starts", "Engine Start Count", "", 0),

    // --- Alarm page headers (7320MKII clone JSON) ---
    {2048, DseDataWidth::Int16, DseDecodeKind::IdentityU16, false, 1.0f,
     "alm_count_old", "Number of Named Alarms (old page 8)", "", 0},
    {39168, DseDataWidth::Int16, DseDecodeKind::Bitmask, false, 1.0f,
     "alm_unnamed", "Unnamed Alarm Conditions (page 153)", "", 0},
    {39424, DseDataWidth::Int16, DseDecodeKind::IdentityU16, false, 1.0f,
     "alm_named_count", "Named Alarm Conditions count (page 154)", "", 0},

    // --- SCF support flags (read); control writes are FC16 @4104/4105 ---
    {4096, DseDataWidth::Int16, DseDecodeKind::Bitmask, false, 1.0f, "scf_f0",
     "SCF Support Flags[0]", "", 0},
    {4097, DseDataWidth::Int16, DseDecodeKind::Bitmask, false, 1.0f, "scf_f1",
     "SCF Support Flags[1]", "", 0},
    {4098, DseDataWidth::Int16, DseDecodeKind::Bitmask, false, 1.0f, "scf_f2",
     "SCF Support Flags[2]", "", 0},
    {4099, DseDataWidth::Int16, DseDecodeKind::Bitmask, false, 1.0f, "scf_f3",
     "SCF Support Flags[3]", "", 0},
    {4100, DseDataWidth::Int16, DseDecodeKind::Bitmask, false, 1.0f, "scf_f4",
     "SCF Support Flags[4]", "", 0},
    {4101, DseDataWidth::Int16, DseDecodeKind::Bitmask, false, 1.0f, "scf_f5",
     "SCF Support Flags[5]", "", 0},
    {4102, DseDataWidth::Int16, DseDecodeKind::Bitmask, false, 1.0f, "scf_f6",
     "SCF Support Flags[6]", "", 0},
    {4103, DseDataWidth::Int16, DseDecodeKind::Bitmask, false, 1.0f, "scf_f7",
     "SCF Support Flags[7]", "", 0},
    {4104, DseDataWidth::Int16, DseDecodeKind::IdentityU16, false, 1.0f,
     "scf_key", "SCF Control Key (write via FC16)", "", 0},
    {4105, DseDataWidth::Int16, DseDecodeKind::IdentityU16, false, 1.0f,
     "scf_key_inv", "SCF Control Key Complement (write via FC16)", "", 0},

    // --- Digital input alarm pages ---
    {39169, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_di_0", "Digital Input Alarms DI1-4", "", 0},
    {39170, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_di_1", "Digital Input Alarms DI5-8", "", 0},
    {39171, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_di_2", "Digital Input Alarms DI9-12", "", 0},
    {39172, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_di_3", "Digital Input Alarms DI13-16", "", 0},

    // --- Standard alarms GenComm page 154 ---
    {39425, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_39425", "Alarms: E-Stop / Oil / Coolant", "", 0},
    {39426, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_39426", "Alarms: Speed / Frequency", "", 0},
    {39427, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_39427", "Alarms: Gen Voltage / Battery", "", 0},
    {39428, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_39428", "Alarms: Charge / Start-Stop / Gen Close", "", 0},
    {39429, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_39429", "Alarms: Mains Close / Oil Sender / MPU", "", 0},
    {39430, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_39430", "Alarms: Overcurrent / Fuel / CAN Warn", "", 0},
    {39431, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_39431", "Alarms: CAN Shutdown / Oil Level / Temp Sw", "", 0},
    {39432, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_39432", "Alarms: Fuel Sw / Watchdog / kW Overload", "", 0},
    {39433, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_39433", "Alarms: Earth / Phase Rot / Maint", "", 0},
    {39434, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_39434", "Alarms: Loading / Fuel Usage", "", 0},
    {39435, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_39435", "Alarms: Protections / Breaker Open Fail", "", 0},
    {39436, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_39436", "Alarms: Bus Breaker / Reverse Power / SC", "", 0},
    {39437, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_39437", "Alarms: Air Flap / Sync / Bus Live", "", 0},
    {39438, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_39438", "Alarms: Bus Phase / Priority / MSC", "", 0},
    {39439, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_39439", "Alarms: Bus Voltage / Frequency", "", 0},
    {39440, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_39440", "Alarms: MSC Failure / Sets / Version", "", 0},
    {39441, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_39441", "Alarms: Mains Export / Capacity / Sync", "", 0},
    {39442, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_39442", "Alarms: Aux Mains / Excitation / ROCOF", "", 0},
    {39443, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_39443", "Alarms: Mains Decoupling Freq/V", "", 0},
    {39444, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_39444", "Alarms: Decoupling Comb / Charge Air", "", 0},
    {39445, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_39445", "Alarms: Coolant Elec Trip / Sender OC", "", 0},

    // --- PLC / user alarms ---
    {39476, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_plc_0", "PLC / User Alarms 1-4", "", 0},
    {39477, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_plc_1", "PLC / User Alarms 5-8", "", 0},
    {39478, DseDataWidth::Int16, DseDecodeKind::AlarmPacked, false, 1.0f,
     "alm_plc_2", "PLC / User Alarms 9-11", "", 0},

    // --- Front-panel LED mirrors (S6 map) ---
    R16U(46648, 1.0f, "led_stop", "Stop LED", "", 521839),
    R16U(46649, 1.0f, "led_manual", "Manual LED", "", 521840),
    R16U(46650, 1.0f, "led_test", "Test LED", "", 521841),
    R16U(46651, 1.0f, "led_auto", "Auto LED", "", 521842),
    R16U(46652, 1.0f, "led_mains", "Mains LED", "", 521843),
    R16U(46653, 1.0f, "led_mains_brk", "Mains Breaker LED", "", 521844),
    R16U(46654, 1.0f, "led_gen_brk", "Gen Breaker LED", "", 521845),
    R16U(46655, 1.0f, "led_gen", "Gen LED", "", 521846),
};

constexpr size_t kMapCount = sizeof(kMap) / sizeof(kMap[0]);

// Slot order: nibble0 (bits0-3) .. nibble3 (bits12-15)
// Matches GenComm table columns right→left: Bits4-1 … Bits16-13
struct AlarmNames {
  uint16_t address;
  const char *slot[4];
};

const AlarmNames kAlarmNames[] = {
    {39425,
     {"Low Coolant Temperature", "High Coolant Temperature",
      "Low Oil Pressure", "Emergency Stop"}},
    {39426,
     {"Generator Over Hz", "Generator Under Hz", "Over Speed", "Under Speed"}},
    {39427,
     {"Battery High Voltage", "Battery Low Voltage", "Generator High Voltage",
      "Generator Low Voltage"}},
    {39428,
     {"Generator Fail to Close", "Fail to Stop", "Fail to Start",
      "Charge Alternator Failure"}},
    {39429,
     {"MPU Open Circuit", "Loss of MPU", "Oil Pressure Sender Fault",
      "Mains Fail to Close"}},
    {39430,
     {"CAN ECU Warning", "Low Fuel Level", "Controller Calibration Lost",
      "Generator Over Current"}},
    {39431,
     {"High Temperature Switch", "Low Oil Level Switch", "CAN ECU Data Fail",
      "CAN ECU Shutdown"}},
    {39432,
     {"(unused)", "kW Overload Alarm", "Expansion Unit Watchdog",
      "Low Fuel Level (switch)"}},
    {39433,
     {"Maintenance Alarm", "Auto Voltage Sense Fail", "Generator Phase Rotation",
      "Earth Fault Trip"}},
    {39434,
     {"Fuel Usage Stopped", "Fuel Usage Running", "Loading Voltage Alarm",
      "Loading Frequency Alarm"}},
    {39435,
     {"Mains Breaker Failed to Open", "Generator Breaker Failed to Open",
      "Protections Blocked", "Protections Disabled"}},
    {39436,
     {"Short Circuit Alarm", "Generator Reverse Power",
      "Bus Breaker Failed to Open", "Bus Breaker Failed to Close"}},
    {39437,
     {"Bus Not Live", "Bus Live", "Failure to Sync", "Air Flap Closed"}},
    {39438,
     {"MSC ID Error", "MSC Data Error", "Priority Selection Error",
      "Bus Phase Rotation"}},
    {39439,
     {"Bus High Frequency", "Bus Low Frequency", "Bus High Voltage",
      "Bus Low Voltage"}},
    {39440,
     {"MSC Old Version Units", "MSC Alarms Inhibited", "MSC Too Few Sets",
      "MSC Failure"}},
    {39441,
     {"Out of Sync", "Insufficient Capacity", "Minimum Sets Not Reached",
      "Mains Reverse Power/Export"}},
    {39442,
     {"Mains Vector Shift", "Mains ROCOF", "Loss of Excitation",
      "Alternative Aux Mains Fail"}},
    {39443,
     {"Mains Decoupling High Voltage", "Mains Decoupling Low Voltage",
      "Mains Decoupling High Freq", "Mains Decoupling Low Freq"}},
    {39444,
     {"AVR Max Trim Limit", "Mains Phase Rotation", "Charge Air Temperature",
      "Mains Decoupling Combined"}},
    {39445,
     {"Out of Sync Mains", "Out of Sync Bus", "Temperature Sender Open Circuit",
      "High Coolant Temp Elec Trip"}},
    {39169, {"DI1", "DI2", "DI3", "DI4"}},
    {39170, {"DI5", "DI6", "DI7", "DI8"}},
    {39171, {"DI9", "DI10", "DI11", "DI12"}},
    {39172, {"DI13", "DI14", "DI15", "DI16"}},
    {39476, {"PLC Alarm 1", "PLC Alarm 2", "PLC Alarm 3", "PLC Alarm 4"}},
    {39477, {"PLC Alarm 5", "PLC Alarm 6", "PLC Alarm 7", "PLC Alarm 8"}},
    {39478, {"PLC Alarm 9", "PLC Alarm 10", "PLC Alarm 11", "(unused)"}},
};

constexpr size_t kAlarmNameCount = sizeof(kAlarmNames) / sizeof(kAlarmNames[0]);

void printScaled(float value, float factor) {
  char buf[24];
  int decimals = 0;
  if (factor <= 0.011f) {
    decimals = 2;
  } else if (factor <= 0.11f) {
    decimals = 1;
  } else if (factor < 1.0f) {
    decimals = 3;
  }
  dtostrf(static_cast<double>(value), 0, decimals, buf);
  Serial.print(buf);
}

bool isInvalidScaled(const DseRegister &reg, int32_t rawS, uint32_t rawU) {
  if (reg.width == DseDataWidth::Int16) {
    if (reg.isSigned) {
      return rawS == 0x7FFF;
    }
    return rawU == 0xFFFFu;
  }
  if (reg.isSigned) {
    return rawS == static_cast<int32_t>(0x7FFFFFFF);
  }
  return rawU == 0xFFFFFFFFu;
}

const AlarmNames *findAlarmNames(uint16_t address) {
  for (size_t i = 0; i < kAlarmNameCount; i++) {
    if (kAlarmNames[i].address == address) {
      return &kAlarmNames[i];
    }
  }
  return nullptr;
}

void printAlarmPacked(uint16_t address, uint16_t raw, bool activeOnly) {
  const AlarmNames *names = findAlarmNames(address);
  Serial.print(F("ALARMS @"));
  Serial.print(address);
  Serial.print(F(" raw=0x"));
  if (raw < 0x1000) {
    Serial.print('0');
  }
  if (raw < 0x100) {
    Serial.print('0');
  }
  if (raw < 0x10) {
    Serial.print('0');
  }
  Serial.println(raw, HEX);

  uint8_t printed = 0;
  for (uint8_t slot = 0; slot < 4; slot++) {
    const uint8_t nibble = static_cast<uint8_t>((raw >> (slot * 4)) & 0xF);
    if (activeOnly && (nibble == 0 || nibble == 1 || nibble == 15)) {
      continue;
    }
    const char *label =
        names ? names->slot[slot] : DseRegisters::alarmSlotName(address, slot);
    Serial.print(F("  ["));
    Serial.print(slot);
    Serial.print(F("] "));
    Serial.print(label);
    Serial.print(F(": "));
    Serial.print(DseRegisters::alarmStateName(nibble));
    Serial.print(F(" ("));
    Serial.print(nibble);
    Serial.println(')');
    printed++;
  }
  if (activeOnly && printed == 0) {
    Serial.println(F("  (no active alarms)"));
  }
}

}  // namespace

namespace DseRegisters {

size_t count() { return kMapCount; }

const DseRegister &at(size_t index) { return kMap[index]; }

const DseRegister *findByKey(const char *key) {
  if (!key || !*key) {
    return nullptr;
  }
  for (size_t i = 0; i < kMapCount; i++) {
    if (strcasecmp(kMap[i].key, key) == 0) {
      return &kMap[i];
    }
  }
  return nullptr;
}

const DseRegister *findByAddress(uint16_t address) {
  for (size_t i = 0; i < kMapCount; i++) {
    if (kMap[i].address == address) {
      return &kMap[i];
    }
  }
  return nullptr;
}

const DseRegister *findByIndex1(size_t oneBased) {
  if (oneBased < 1 || oneBased > kMapCount) {
    return nullptr;
  }
  return &kMap[oneBased - 1];
}

const char *alarmStateName(uint8_t nibble) {
  switch (nibble) {
    case 0:
      return "Disabled";
    case 1:
      return "Healthy";
    case 2:
      return "Warning";
    case 3:
      return "Shutdown";
    case 4:
      return "ElectricalTrip";
    case 15:
      return "Unimplemented";
    default:
      return "Unknown";
  }
}

const char *alarmSlotName(uint16_t address, uint8_t slot0to3) {
  const AlarmNames *n = findAlarmNames(address);
  if (n && slot0to3 < 4) {
    return n->slot[slot0to3];
  }
  return "Alarm slot";
}

const char *opModeName(uint16_t code) {
  switch (code) {
    case 0:
      return "Stop";
    case 1:
      return "Auto";
    case 2:
      return "Manual";
    case 3:
      return "Test on Load";
    case 4:
      return "Auto+Manual Restore";
    case 5:
      return "User Configuration";
    case 6:
      return "Test off Load";
    case 7:
      return "Off";
    default:
      return "Unknown";
  }
}

const char *engStatusName(uint16_t code) {
  switch (code) {
    case 0:
      return "Stopped";
    case 1:
      return "Pre-Start/Preheat";
    case 2:
      return "Warming Up";
    case 3:
      return "Running";
    case 4:
      return "Cooling Down";
    case 5:
      return "Stopped";
    case 6:
      return "Post Run";
    case 15:
      return "N/A";
    default:
      return "Unknown";
  }
}

bool assembleRaw(const DseRegister &reg, const uint16_t *values,
                 uint8_t valueCount, int32_t *outRaw, uint32_t *outRawU) {
  if (!values || !outRaw) {
    return false;
  }
  const uint8_t need = static_cast<uint8_t>(reg.width);
  if (valueCount < need) {
    return false;
  }
  if (reg.width == DseDataWidth::Int16) {
    const uint16_t u = values[0];
    if (outRawU) {
      *outRawU = u;
    }
    *outRaw = reg.isSigned ? static_cast<int32_t>(static_cast<int16_t>(u))
                           : static_cast<int32_t>(u);
    return true;
  }
  const uint32_t u =
      (static_cast<uint32_t>(values[0]) << 16) | values[1];
  if (outRawU) {
    *outRawU = u;
  }
  *outRaw = reg.isSigned ? static_cast<int32_t>(u) : static_cast<int32_t>(u);
  return true;
}

DseDecodedValue decode(const DseRegister &reg, const uint16_t *values,
                       uint8_t valueCount) {
  DseDecodedValue v;
  v.reg = &reg;
  int32_t rawS = 0;
  uint32_t rawU = 0;
  if (!assembleRaw(reg, values, valueCount, &rawS, &rawU)) {
    return v;
  }
  v.ok = true;
  v.raw = rawS;
  v.rawU = rawU;

  switch (reg.kind) {
    case DseDecodeKind::Scaled:
      v.invalid = isInvalidScaled(reg, rawS, rawU);
      if (reg.isSigned) {
        v.engineering = static_cast<float>(rawS) * reg.factor;
      } else if (reg.width == DseDataWidth::Int32) {
        v.engineering = static_cast<float>(rawU) * reg.factor;
      } else {
        v.engineering = static_cast<float>(rawU) * reg.factor;
      }
      break;
    case DseDecodeKind::HoursFromSeconds:
      v.invalid = (rawU == 0xFFFFFFFFu);
      v.engineering = static_cast<float>(rawU) / 3600.0f;
      break;
    case DseDecodeKind::IdentityU16:
    case DseDecodeKind::Bitmask:
    case DseDecodeKind::EnumOpMode:
    case DseDecodeKind::EnumEngStatus:
    case DseDecodeKind::AlarmPacked:
      v.engineering = static_cast<float>(rawU);
      break;
    case DseDecodeKind::IdentityU32:
      v.invalid = (rawU == 0xFFFFFFFFu);
      v.engineering = static_cast<float>(rawU);
      break;
  }
  return v;
}

void printEntry(const DseRegister &reg) {
  Serial.print(reg.key);
  Serial.print(F("  @"));
  Serial.print(reg.address);
  Serial.print(F("  qty="));
  Serial.print(static_cast<uint8_t>(reg.width));
  Serial.print(F("  "));
  Serial.print(reg.name);
  if (reg.unit && reg.unit[0]) {
    Serial.print(F(" ["));
    Serial.print(reg.unit);
    Serial.print(']');
  }
  Serial.println();
}

void printList() {
  Serial.println(F("## DSE GenComm register map (FC03)"));
  Serial.print(F("Entries: "));
  Serial.println(kMapCount);
  for (size_t i = 0; i < kMapCount; i++) {
    Serial.print(i + 1);
    Serial.print(F(". "));
    printEntry(kMap[i]);
  }
}

void printDecoded(const DseDecodedValue &v) {
  if (!v.ok || !v.reg) {
    Serial.println(F("DECODE: failed (insufficient registers)"));
    return;
  }
  const DseRegister &r = *v.reg;

  if (r.kind == DseDecodeKind::AlarmPacked) {
    printAlarmPacked(r.address, static_cast<uint16_t>(v.rawU), false);
    return;
  }

  Serial.print(F("VALUE: "));
  Serial.print(r.name);
  Serial.print(F(" = "));

  if (v.invalid) {
    Serial.print(F("UNAVAILABLE"));
  } else {
    switch (r.kind) {
      case DseDecodeKind::EnumOpMode:
        Serial.print(opModeName(static_cast<uint16_t>(v.rawU)));
        Serial.print(F(" ("));
        Serial.print(v.rawU);
        Serial.print(')');
        break;
      case DseDecodeKind::EnumEngStatus:
        Serial.print(engStatusName(static_cast<uint16_t>(v.rawU)));
        Serial.print(F(" ("));
        Serial.print(v.rawU);
        Serial.print(')');
        break;
      case DseDecodeKind::Bitmask:
        Serial.print(F("0x"));
        Serial.print(v.rawU, HEX);
        break;
      case DseDecodeKind::IdentityU16:
        Serial.print(v.rawU);
        if (r.address == 768 && v.rawU == 1) {
          Serial.print(F(" (DSE)"));
        }
        break;
      case DseDecodeKind::IdentityU32:
        Serial.print(v.rawU);
        break;
      case DseDecodeKind::HoursFromSeconds: {
        char buf[24];
        dtostrf(static_cast<double>(v.engineering), 0, 2, buf);
        Serial.print(buf);
        Serial.print(F(" h"));
        Serial.print(F(" ("));
        Serial.print(v.rawU);
        Serial.print(F(" s)"));
        break;
      }
      case DseDecodeKind::Scaled:
      default:
        printScaled(v.engineering, r.factor);
        if (r.unit && r.unit[0]) {
          Serial.print(' ');
          Serial.print(r.unit);
        }
        break;
    }
  }

  Serial.print(F("  (raw="));
  if (r.width == DseDataWidth::Int32 && !r.isSigned) {
    Serial.print(v.rawU);
  } else {
    Serial.print(v.raw);
  }
  Serial.print(F(" @"));
  Serial.print(r.address);
  Serial.print(F(" key="));
  Serial.print(r.key);
  Serial.println(')');
}

void annotateRead(uint16_t address, uint16_t quantity, const uint16_t *values,
                  uint8_t valueCount) {
  const DseRegister *reg = findByAddress(address);
  if (!reg) {
    return;
  }
  const uint8_t need = static_cast<uint8_t>(reg->width);
  if (quantity < need || valueCount < need) {
    return;
  }
  printDecoded(decode(*reg, values, valueCount));
}

void printAlarmActive(uint16_t address, uint16_t raw) {
  printAlarmPacked(address, raw, true);
}

const char *friendlyLabel(const DseRegister &reg) {
  if (strcmp(reg.key, "gen_van") == 0) {
    return "Line 1 voltage";
  }
  if (strcmp(reg.key, "gen_vbn") == 0) {
    return "Line 2 voltage";
  }
  if (strcmp(reg.key, "gen_vcn") == 0) {
    return "Line 3 voltage";
  }
  if (strcmp(reg.key, "gen_vab") == 0) {
    return "Line 1-2 voltage";
  }
  if (strcmp(reg.key, "gen_vbc") == 0) {
    return "Line 2-3 voltage";
  }
  if (strcmp(reg.key, "gen_vca") == 0) {
    return "Line 3-1 voltage";
  }
  if (strcmp(reg.key, "gen_ia") == 0) {
    return "Line 1 current";
  }
  if (strcmp(reg.key, "gen_ib") == 0) {
    return "Line 2 current";
  }
  if (strcmp(reg.key, "gen_ic") == 0) {
    return "Line 3 current";
  }
  if (strcmp(reg.key, "gen_freq") == 0) {
    return "Frequency";
  }
  if (strcmp(reg.key, "battery_v") == 0) {
    return "Battery voltage";
  }
  if (strcmp(reg.key, "charge_alt_v") == 0) {
    return "Charge alternator voltage";
  }
  if (strcmp(reg.key, "eng_rpm") == 0) {
    return "Engine RPM";
  }
  if (strcmp(reg.key, "oil_press") == 0) {
    return "Oil pressure";
  }
  if (strcmp(reg.key, "coolant_temp") == 0) {
    return "Coolant temperature";
  }
  if (strcmp(reg.key, "oil_temp") == 0) {
    return "Oil temperature";
  }
  if (strcmp(reg.key, "fuel_level") == 0) {
    return "Fuel level";
  }
  if (strcmp(reg.key, "gen_pa") == 0) {
    return "Line 1 power";
  }
  if (strcmp(reg.key, "gen_pb") == 0) {
    return "Line 2 power";
  }
  if (strcmp(reg.key, "gen_pc") == 0) {
    return "Line 3 power";
  }
  if (strcmp(reg.key, "gen_p_tot") == 0) {
    return "Total real power";
  }
  if (strcmp(reg.key, "gen_q_tot") == 0) {
    return "Total reactive power";
  }
  if (strcmp(reg.key, "gen_s_tot") == 0) {
    return "Total apparent power";
  }
  if (strcmp(reg.key, "gen_pf") == 0) {
    return "Power factor";
  }
  if (strcmp(reg.key, "gen_pct_kw") == 0) {
    return "Load percent";
  }
  if (strcmp(reg.key, "mains_freq") == 0) {
    return "Mains frequency";
  }
  if (strcmp(reg.key, "mains_van") == 0) {
    return "Mains L1 voltage";
  }
  if (strcmp(reg.key, "mains_vbn") == 0) {
    return "Mains L2 voltage";
  }
  if (strcmp(reg.key, "mains_vcn") == 0) {
    return "Mains L3 voltage";
  }
  if (strcmp(reg.key, "eng_hours") == 0) {
    return "Engine hours";
  }
  if (strcmp(reg.key, "eng_starts") == 0) {
    return "Engine starts";
  }
  if (strcmp(reg.key, "gen_kwh_exp") == 0) {
    return "Energy export";
  }
  if (strcmp(reg.key, "op_mode") == 0) {
    return "Operating mode";
  }
  if (strcmp(reg.key, "eng_status") == 0) {
    return "Engine status";
  }
  if (strcmp(reg.key, "oil_level") == 0) {
    return "Oil level";
  }
  if (strcmp(reg.key, "coolant_level") == 0) {
    return "Coolant level";
  }
  return reg.name;
}

void printBatchReadable(uint16_t batchStart, const uint16_t *values,
                        uint8_t valueCount) {
  if (!values || valueCount == 0) {
    return;
  }
  const uint16_t batchEnd =
      static_cast<uint16_t>(batchStart + valueCount);  // exclusive
  for (size_t i = 0; i < kMapCount; i++) {
    const DseRegister &r = kMap[i];
    // Skip alarm/SCF noise in iterate output
    if (r.kind == DseDecodeKind::AlarmPacked ||
        r.kind == DseDecodeKind::Bitmask || strncmp(r.key, "scf_", 4) == 0 ||
        strncmp(r.key, "led_", 4) == 0 || strncmp(r.key, "alm_", 4) == 0) {
      continue;
    }
    const uint8_t need = static_cast<uint8_t>(r.width);
    if (r.address < batchStart ||
        static_cast<uint16_t>(r.address + need) > batchEnd) {
      continue;
    }
    const uint8_t off = static_cast<uint8_t>(r.address - batchStart);
    const DseDecodedValue v = decode(r, values + off, need);
    if (!v.ok) {
      continue;
    }

    Serial.print(friendlyLabel(r));
    Serial.print(F(": "));
    if (v.invalid) {
      Serial.println(F("unavailable"));
      continue;
    }
    switch (r.kind) {
      case DseDecodeKind::EnumOpMode:
        Serial.println(opModeName(static_cast<uint16_t>(v.rawU)));
        break;
      case DseDecodeKind::EnumEngStatus:
        Serial.println(engStatusName(static_cast<uint16_t>(v.rawU)));
        break;
      case DseDecodeKind::HoursFromSeconds: {
        char buf[24];
        dtostrf(static_cast<double>(v.engineering), 0, 2, buf);
        Serial.print(buf);
        Serial.println(F(" h"));
        break;
      }
      case DseDecodeKind::IdentityU16:
      case DseDecodeKind::IdentityU32:
        Serial.println(v.rawU);
        break;
      case DseDecodeKind::Scaled:
      default:
        printScaled(v.engineering, r.factor);
        if (r.unit && r.unit[0]) {
          Serial.print(' ');
          Serial.print(r.unit);
        }
        Serial.println();
        break;
    }
  }
}

}  // namespace DseRegisters
