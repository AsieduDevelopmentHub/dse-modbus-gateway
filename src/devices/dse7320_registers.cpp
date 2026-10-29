#include "devices/dse7320_registers.h"

#if !defined(GATEWAY_LITE)

#include <string.h>

#include <Arduino.h>
#if defined(__AVR__)
#include <avr/pgmspace.h>
#else
#include <pgmspace.h>
#endif

#include "util/str_util.h"

namespace {

// Flash-resident map row (string pointers are PSTR / PROGMEM on AVR).
struct DseRegisterFlash {
  uint16_t address;
  uint8_t width;
  uint8_t kind;
  uint8_t isSigned;
  float factor;
  PGM_P key;
  PGM_P name;
  PGM_P unit;
  uint32_t spn;
};

#define R16S(addr, fac, key, name, unit, spn)                                  \
  {addr, 1, static_cast<uint8_t>(DseDecodeKind::Scaled), 1, fac, key, name,    \
   unit, spn}
#define R16U(addr, fac, key, name, unit, spn)                                  \
  {addr, 1, static_cast<uint8_t>(DseDecodeKind::Scaled), 0, fac, key, name,    \
   unit, spn}
#define R32S(addr, fac, key, name, unit, spn)                                  \
  {addr, 2, static_cast<uint8_t>(DseDecodeKind::Scaled), 1, fac, key, name,    \
   unit, spn}
#define R32U(addr, fac, key, name, unit, spn)                                  \
  {addr, 2, static_cast<uint8_t>(DseDecodeKind::Scaled), 0, fac, key, name,    \
   unit, spn}

// Named PROGMEM strings (Arduino PSTR() cannot appear in static initializers)
static const char dse_ps_7e167c1d10[] PROGMEM = "mfr_code";
static const char dse_ps_3bcbfd7e64[] PROGMEM = "Manufacturer Code";
static const char dse_ps_d41d8cd98f[] PROGMEM = "";
static const char dse_ps_20f35e630d[] PROGMEM = "model";
static const char dse_ps_dd20850f12[] PROGMEM = "Model Number";
static const char dse_ps_74136b2421[] PROGMEM = "serial";
static const char dse_ps_78cf8ce506[] PROGMEM = "Serial Number";
static const char dse_ps_1c7cc88fe1[] PROGMEM = "op_mode";
static const char dse_ps_fed6c84461[] PROGMEM = "Operating Mode / Switch State";
static const char dse_ps_d416e37937[] PROGMEM = "ctrl_status";
static const char dse_ps_594ccab04e[] PROGMEM = "Control Unit Status";
static const char dse_ps_1cc021520e[] PROGMEM = "chg_stages";
static const char dse_ps_eed8dc9c33[] PROGMEM = "Battery Charger Stages";
static const char dse_ps_0274e235a6[] PROGMEM = "chg_status";
static const char dse_ps_1df23e2e92[] PROGMEM = "Battery Charger Status";
static const char dse_ps_5c314c6089[] PROGMEM = "eng_status";
static const char dse_ps_e101d50d52[] PROGMEM = "Engine Operating State";
static const char dse_ps_729dae3919[] PROGMEM = "eng_status_alt";
static const char dse_ps_8f1ea1332a[] PROGMEM = "Engine State (alt)";
static const char dse_ps_35538962db[] PROGMEM = "password_status";
static const char dse_ps_ade3d33eda[] PROGMEM = "Password Status";
static const char dse_ps_20ada7717d[] PROGMEM = "gen_status";
static const char dse_ps_d50d8afcd9[] PROGMEM = "Generator Status (packed)";
static const char dse_ps_6fc05f75b9[] PROGMEM = "device_ident";
static const char dse_ps_9170cd0393[] PROGMEM = "Device Identity";
static const char dse_ps_878265f8a5[] PROGMEM = "eng_hours";
static const char dse_ps_ff63ed2bf6[] PROGMEM = "Engine Operating Hours";
static const char dse_ps_2510c39011[] PROGMEM = "h";
static const char dse_ps_30a3b500a8[] PROGMEM = "alm_count_old";
static const char dse_ps_e222d40499[] PROGMEM = "Number of Named Alarms (old page 8)";
static const char dse_ps_bde6bc6a53[] PROGMEM = "alm_unnamed";
static const char dse_ps_e314e89d45[] PROGMEM = "Unnamed Alarm Conditions (page 153)";
static const char dse_ps_f5c045c3fc[] PROGMEM = "alm_named_count";
static const char dse_ps_afb0925676[] PROGMEM = "Named Alarm Conditions count (page 154)";
static const char dse_ps_05b8c96b83[] PROGMEM = "scf_f0";
static const char dse_ps_7c82abc3a7[] PROGMEM = "SCF Support Flags[0]";
static const char dse_ps_8c3fe0361a[] PROGMEM = "scf_f1";
static const char dse_ps_e019d3752b[] PROGMEM = "SCF Support Flags[1]";
static const char dse_ps_ad081cd0f2[] PROGMEM = "scf_f2";
static const char dse_ps_46632d858a[] PROGMEM = "SCF Support Flags[2]";
static const char dse_ps_4d34f14f45[] PROGMEM = "scf_f3";
static const char dse_ps_fd15ff7def[] PROGMEM = "SCF Support Flags[3]";
static const char dse_ps_f02d77658c[] PROGMEM = "scf_f4";
static const char dse_ps_7ec28d3738[] PROGMEM = "SCF Support Flags[4]";
static const char dse_ps_58913a2528[] PROGMEM = "scf_f5";
static const char dse_ps_da7a485b8d[] PROGMEM = "SCF Support Flags[5]";
static const char dse_ps_ca951e6e8c[] PROGMEM = "scf_f6";
static const char dse_ps_00adba35d6[] PROGMEM = "SCF Support Flags[6]";
static const char dse_ps_f6241a4d41[] PROGMEM = "scf_f7";
static const char dse_ps_d0e62e59ae[] PROGMEM = "SCF Support Flags[7]";
static const char dse_ps_4effb8a17e[] PROGMEM = "scf_key";
static const char dse_ps_2680d6fe93[] PROGMEM = "SCF Control Key (write via FC16)";
static const char dse_ps_91542b952c[] PROGMEM = "scf_key_inv";
static const char dse_ps_eef0fd92f6[] PROGMEM = "SCF Control Key Complement (write via FC16)";
static const char dse_ps_6138c1e393[] PROGMEM = "alm_di_0";
static const char dse_ps_d3a0bd95ae[] PROGMEM = "Digital Input Alarms DI1-4";
static const char dse_ps_97a6b39cb5[] PROGMEM = "alm_di_1";
static const char dse_ps_176be0bea8[] PROGMEM = "Digital Input Alarms DI5-8";
static const char dse_ps_127c1ff8d0[] PROGMEM = "alm_di_2";
static const char dse_ps_d9a414e033[] PROGMEM = "Digital Input Alarms DI9-12";
static const char dse_ps_59a976fe30[] PROGMEM = "alm_di_3";
static const char dse_ps_d4d7edd525[] PROGMEM = "Digital Input Alarms DI13-16";
static const char dse_ps_10982cb7dc[] PROGMEM = "alm_39425";
static const char dse_ps_535599d73d[] PROGMEM = "Alarms: E-Stop / Oil / Coolant";
static const char dse_ps_4d7c78bc5a[] PROGMEM = "alm_39426";
static const char dse_ps_133c933471[] PROGMEM = "Alarms: Speed / Frequency";
static const char dse_ps_2fc4f9a395[] PROGMEM = "alm_39427";
static const char dse_ps_282c5f5069[] PROGMEM = "Alarms: Gen Voltage / Battery";
static const char dse_ps_a1a81f9129[] PROGMEM = "alm_39428";
static const char dse_ps_b05fb791ad[] PROGMEM = "Alarms: Charge / Start-Stop / Gen Close";
static const char dse_ps_81d6890536[] PROGMEM = "alm_39429";
static const char dse_ps_da7184c3d4[] PROGMEM = "Alarms: Mains Close / Oil Sender / MPU";
static const char dse_ps_8919637575[] PROGMEM = "alm_39430";
static const char dse_ps_d739b94954[] PROGMEM = "Alarms: Overcurrent / Fuel / CAN Warn";
static const char dse_ps_3f8a514833[] PROGMEM = "alm_39431";
static const char dse_ps_dcfa7cabf4[] PROGMEM = "Alarms: CAN Shutdown / Oil Level / Temp Sw";
static const char dse_ps_19d58cd91f[] PROGMEM = "alm_39432";
static const char dse_ps_7ad825e835[] PROGMEM = "Alarms: Fuel Sw / Watchdog / kW Overload";
static const char dse_ps_9315eb0456[] PROGMEM = "alm_39433";
static const char dse_ps_71a6df89c7[] PROGMEM = "Alarms: Earth / Phase Rot / Maint";
static const char dse_ps_453d02fb09[] PROGMEM = "alm_39434";
static const char dse_ps_aea7b9fba9[] PROGMEM = "Alarms: Loading / Fuel Usage";
static const char dse_ps_d9db2df72b[] PROGMEM = "alm_39435";
static const char dse_ps_19dbb6e61b[] PROGMEM = "Alarms: Protections / Breaker Open Fail";
static const char dse_ps_fcedaea47e[] PROGMEM = "alm_39436";
static const char dse_ps_9697d36bd0[] PROGMEM = "Alarms: Bus Breaker / Reverse Power / SC";
static const char dse_ps_66ca78663a[] PROGMEM = "alm_39437";
static const char dse_ps_e5275ce396[] PROGMEM = "Alarms: Air Flap / Sync / Bus Live";
static const char dse_ps_3f893f6495[] PROGMEM = "alm_39438";
static const char dse_ps_635aac0f6f[] PROGMEM = "Alarms: Bus Phase / Priority / MSC";
static const char dse_ps_c29f4858c4[] PROGMEM = "alm_39439";
static const char dse_ps_ddac102d91[] PROGMEM = "Alarms: Bus Voltage / Frequency";
static const char dse_ps_90a2246b60[] PROGMEM = "alm_39440";
static const char dse_ps_5a40e31a50[] PROGMEM = "Alarms: MSC Failure / Sets / Version";
static const char dse_ps_ca8df72474[] PROGMEM = "alm_39441";
static const char dse_ps_f74b8d407c[] PROGMEM = "Alarms: Mains Export / Capacity / Sync";
static const char dse_ps_02b76db363[] PROGMEM = "alm_39442";
static const char dse_ps_afb3860fb3[] PROGMEM = "Alarms: Aux Mains / Excitation / ROCOF";
static const char dse_ps_a60a1528ea[] PROGMEM = "alm_39443";
static const char dse_ps_b78ebae358[] PROGMEM = "Alarms: Mains Decoupling Freq/V";
static const char dse_ps_84f8c330e4[] PROGMEM = "alm_39444";
static const char dse_ps_69142ba7fc[] PROGMEM = "Alarms: Decoupling Comb / Charge Air";
static const char dse_ps_e9aee87419[] PROGMEM = "alm_39445";
static const char dse_ps_27a3e160e4[] PROGMEM = "Alarms: Coolant Elec Trip / Sender OC";
static const char dse_ps_b5327b9d58[] PROGMEM = "alm_plc_0";
static const char dse_ps_7bb7586adb[] PROGMEM = "PLC / User Alarms 1-4";
static const char dse_ps_651c282f35[] PROGMEM = "alm_plc_1";
static const char dse_ps_9558935667[] PROGMEM = "PLC / User Alarms 5-8";
static const char dse_ps_bc52747cb9[] PROGMEM = "alm_plc_2";
static const char dse_ps_91554a8c69[] PROGMEM = "PLC / User Alarms 9-11";
static const char dse_ps_2f4571e67f[] PROGMEM = "Low Coolant Temperature";
static const char dse_ps_95143a97af[] PROGMEM = "High Coolant Temperature";
static const char dse_ps_b76eb8a4d8[] PROGMEM = "Low Oil Pressure";
static const char dse_ps_5873a7a8a7[] PROGMEM = "Emergency Stop";
static const char dse_ps_e68935ed68[] PROGMEM = "Generator Over Hz";
static const char dse_ps_568953dc21[] PROGMEM = "Generator Under Hz";
static const char dse_ps_ff83a35cfb[] PROGMEM = "Over Speed";
static const char dse_ps_0e77d4ff67[] PROGMEM = "Under Speed";
static const char dse_ps_bf010ec1af[] PROGMEM = "Battery High Voltage";
static const char dse_ps_1f62a3ca04[] PROGMEM = "Battery Low Voltage";
static const char dse_ps_e999c3d907[] PROGMEM = "Generator High Voltage";
static const char dse_ps_f55c09161d[] PROGMEM = "Generator Low Voltage";
static const char dse_ps_1c83a08082[] PROGMEM = "Generator Fail to Close";
static const char dse_ps_89dc13b27c[] PROGMEM = "Fail to Stop";
static const char dse_ps_3d1e1e3d88[] PROGMEM = "Fail to Start";
static const char dse_ps_bdda68fb33[] PROGMEM = "Charge Alternator Failure";
static const char dse_ps_3f10cb6289[] PROGMEM = "MPU Open Circuit";
static const char dse_ps_19e3245059[] PROGMEM = "Loss of MPU";
static const char dse_ps_905724fa35[] PROGMEM = "Oil Pressure Sender Fault";
static const char dse_ps_a0b8a0d86f[] PROGMEM = "Mains Fail to Close";
static const char dse_ps_58f1e13833[] PROGMEM = "CAN ECU Warning";
static const char dse_ps_114e36360c[] PROGMEM = "Low Fuel Level";
static const char dse_ps_854ffef6a5[] PROGMEM = "Controller Calibration Lost";
static const char dse_ps_01d9266dcc[] PROGMEM = "Generator Over Current";
static const char dse_ps_193fc5f5b9[] PROGMEM = "High Temperature Switch";
static const char dse_ps_ef010f1d6a[] PROGMEM = "Low Oil Level Switch";
static const char dse_ps_370d896287[] PROGMEM = "CAN ECU Data Fail";
static const char dse_ps_162406935c[] PROGMEM = "CAN ECU Shutdown";
static const char dse_ps_593adf9a72[] PROGMEM = "(unused)";
static const char dse_ps_d9308e074d[] PROGMEM = "kW Overload Alarm";
static const char dse_ps_6bcd5c0fc4[] PROGMEM = "Expansion Unit Watchdog";
static const char dse_ps_ab1c7e33ae[] PROGMEM = "Low Fuel Level (switch)";
static const char dse_ps_6451a38fba[] PROGMEM = "Maintenance Alarm";
static const char dse_ps_5a8324d612[] PROGMEM = "Auto Voltage Sense Fail";
static const char dse_ps_c91820e43b[] PROGMEM = "Generator Phase Rotation";
static const char dse_ps_ed1f62b070[] PROGMEM = "Earth Fault Trip";
static const char dse_ps_bc0c24f899[] PROGMEM = "Fuel Usage Stopped";
static const char dse_ps_407739b4a1[] PROGMEM = "Fuel Usage Running";
static const char dse_ps_166f95288f[] PROGMEM = "Loading Voltage Alarm";
static const char dse_ps_d67ee80f1b[] PROGMEM = "Loading Frequency Alarm";
static const char dse_ps_c311d66214[] PROGMEM = "Mains Breaker Failed to Open";
static const char dse_ps_fce9aae0d8[] PROGMEM = "Generator Breaker Failed to Open";
static const char dse_ps_e83f52263c[] PROGMEM = "Protections Blocked";
static const char dse_ps_56754b5158[] PROGMEM = "Protections Disabled";
static const char dse_ps_ea049685a8[] PROGMEM = "Short Circuit Alarm";
static const char dse_ps_bf1f6a21ad[] PROGMEM = "Generator Reverse Power";
static const char dse_ps_5bb90da3c4[] PROGMEM = "Bus Breaker Failed to Open";
static const char dse_ps_95029306d8[] PROGMEM = "Bus Breaker Failed to Close";
static const char dse_ps_6235197a28[] PROGMEM = "Bus Not Live";
static const char dse_ps_dc45c845d4[] PROGMEM = "Bus Live";
static const char dse_ps_1df6b3b27d[] PROGMEM = "Failure to Sync";
static const char dse_ps_ccb77aab70[] PROGMEM = "Air Flap Closed";
static const char dse_ps_9d2d1b40f1[] PROGMEM = "MSC ID Error";
static const char dse_ps_be5503f06e[] PROGMEM = "MSC Data Error";
static const char dse_ps_7419abd307[] PROGMEM = "Priority Selection Error";
static const char dse_ps_85dce448b2[] PROGMEM = "Bus Phase Rotation";
static const char dse_ps_4f73f55cd9[] PROGMEM = "Bus High Frequency";
static const char dse_ps_67e068ad51[] PROGMEM = "Bus Low Frequency";
static const char dse_ps_14befac4c7[] PROGMEM = "Bus High Voltage";
static const char dse_ps_db76bb5c4a[] PROGMEM = "Bus Low Voltage";
static const char dse_ps_d19a98444c[] PROGMEM = "MSC Old Version Units";
static const char dse_ps_7df2373690[] PROGMEM = "MSC Alarms Inhibited";
static const char dse_ps_d1c32ebbc2[] PROGMEM = "MSC Too Few Sets";
static const char dse_ps_c483f09df4[] PROGMEM = "MSC Failure";
static const char dse_ps_6945d551a1[] PROGMEM = "Out of Sync";
static const char dse_ps_2769af4f41[] PROGMEM = "Insufficient Capacity";
static const char dse_ps_64534d88cf[] PROGMEM = "Minimum Sets Not Reached";
static const char dse_ps_1cfd4bc545[] PROGMEM = "Mains Reverse Power/Export";
static const char dse_ps_dc6e00916a[] PROGMEM = "Mains Vector Shift";
static const char dse_ps_3e9327e63c[] PROGMEM = "Mains ROCOF";
static const char dse_ps_df62a6b362[] PROGMEM = "Loss of Excitation";
static const char dse_ps_381c8c1bea[] PROGMEM = "Alternative Aux Mains Fail";
static const char dse_ps_0d8f4474ea[] PROGMEM = "Mains Decoupling High Voltage";
static const char dse_ps_0048ae9d16[] PROGMEM = "Mains Decoupling Low Voltage";
static const char dse_ps_ca3c9c2746[] PROGMEM = "Mains Decoupling High Freq";
static const char dse_ps_efc0c93976[] PROGMEM = "Mains Decoupling Low Freq";
static const char dse_ps_d90c055ad0[] PROGMEM = "AVR Max Trim Limit";
static const char dse_ps_ad99d9677a[] PROGMEM = "Mains Phase Rotation";
static const char dse_ps_42ddee8523[] PROGMEM = "Charge Air Temperature";
static const char dse_ps_a16e4b5e33[] PROGMEM = "Mains Decoupling Combined";
static const char dse_ps_0bb882c198[] PROGMEM = "Out of Sync Mains";
static const char dse_ps_2d9e181236[] PROGMEM = "Out of Sync Bus";
static const char dse_ps_5cf195b029[] PROGMEM = "Temperature Sender Open Circuit";
static const char dse_ps_de19d59c28[] PROGMEM = "High Coolant Temp Elec Trip";
static const char dse_ps_78ce164c32[] PROGMEM = "DI1";
static const char dse_ps_8aaf392095[] PROGMEM = "DI2";
static const char dse_ps_fbea5a1989[] PROGMEM = "DI3";
static const char dse_ps_496096c68d[] PROGMEM = "DI4";
static const char dse_ps_ccf8834d92[] PROGMEM = "DI5";
static const char dse_ps_254c8c1769[] PROGMEM = "DI6";
static const char dse_ps_3997f020f8[] PROGMEM = "DI7";
static const char dse_ps_38e1c64319[] PROGMEM = "DI8";
static const char dse_ps_d8497aaa93[] PROGMEM = "DI9";
static const char dse_ps_1073950751[] PROGMEM = "DI10";
static const char dse_ps_9182b4c3bc[] PROGMEM = "DI11";
static const char dse_ps_e3f8b46b37[] PROGMEM = "DI12";
static const char dse_ps_af7e9ab2da[] PROGMEM = "DI13";
static const char dse_ps_1e34ff8651[] PROGMEM = "DI14";
static const char dse_ps_ad08ccddac[] PROGMEM = "DI15";
static const char dse_ps_9b8423266a[] PROGMEM = "DI16";
static const char dse_ps_0962a00a7a[] PROGMEM = "PLC Alarm 1";
static const char dse_ps_3e41b22dd0[] PROGMEM = "PLC Alarm 2";
static const char dse_ps_b039df3b88[] PROGMEM = "PLC Alarm 3";
static const char dse_ps_b6d0ee980e[] PROGMEM = "PLC Alarm 4";
static const char dse_ps_577fb09115[] PROGMEM = "PLC Alarm 5";
static const char dse_ps_5276791421[] PROGMEM = "PLC Alarm 6";
static const char dse_ps_b95b1f27a4[] PROGMEM = "PLC Alarm 7";
static const char dse_ps_5ddc03e96f[] PROGMEM = "PLC Alarm 8";
static const char dse_ps_23e155f98f[] PROGMEM = "PLC Alarm 9";
static const char dse_ps_944917c378[] PROGMEM = "PLC Alarm 10";
static const char dse_ps_15ada885b5[] PROGMEM = "PLC Alarm 11";
static const char dse_ps_f27b039bf5[] PROGMEM = "oil_press";
static const char dse_ps_ff57ab08eb[] PROGMEM = "Oil Pressure";
static const char dse_ps_97d41f8341[] PROGMEM = "kPa";
static const char dse_ps_2bd01ee0af[] PROGMEM = "coolant_temp";
static const char dse_ps_c39c590cfc[] PROGMEM = "Coolant Temperature";
static const char dse_ps_0d61f8370c[] PROGMEM = "C";
static const char dse_ps_6a3bfcfa0d[] PROGMEM = "oil_temp";
static const char dse_ps_7ea431cdca[] PROGMEM = "Oil Temperature";
static const char dse_ps_f1b51323c3[] PROGMEM = "fuel_level";
static const char dse_ps_80a55f5983[] PROGMEM = "Fuel / Tank Level";
static const char dse_ps_0bcef9c45b[] PROGMEM = "%";
static const char dse_ps_1c2eff267e[] PROGMEM = "charge_alt_v";
static const char dse_ps_78a74de0e0[] PROGMEM = "Charger Alternator Voltage";
static const char dse_ps_5206560a30[] PROGMEM = "V";
static const char dse_ps_983590dc35[] PROGMEM = "battery_v";
static const char dse_ps_88f7e267b8[] PROGMEM = "Battery / Starter Voltage";
static const char dse_ps_f405dddd07[] PROGMEM = "eng_rpm";
static const char dse_ps_8fadf4417c[] PROGMEM = "Engine Speed";
static const char dse_ps_455385f5d8[] PROGMEM = "rpm";
static const char dse_ps_d378782718[] PROGMEM = "oil_level";
static const char dse_ps_a6d699fb29[] PROGMEM = "Oil Level";
static const char dse_ps_7afa802402[] PROGMEM = "coolant_level";
static const char dse_ps_3fafe493c2[] PROGMEM = "Coolant Level";
static const char dse_ps_75793476a4[] PROGMEM = "chg_i_out";
static const char dse_ps_8d672bad48[] PROGMEM = "Battery Charger Output Current";
static const char dse_ps_7fc56270e7[] PROGMEM = "A";
static const char dse_ps_29518834a1[] PROGMEM = "chg_v_out";
static const char dse_ps_afed100c18[] PROGMEM = "Battery Charger Output Voltage";
static const char dse_ps_9574a34829[] PROGMEM = "gen_freq";
static const char dse_ps_8987176615[] PROGMEM = "Generator Average AC Frequency";
static const char dse_ps_d1e2634cb7[] PROGMEM = "Hz";
static const char dse_ps_1cbee4df4a[] PROGMEM = "gen_van";
static const char dse_ps_3416ba27d1[] PROGMEM = "Generator Phase A Line-Neutral AC RMS Voltage";
static const char dse_ps_928a324b0c[] PROGMEM = "gen_vbn";
static const char dse_ps_e1d0102ee4[] PROGMEM = "Generator Phase B Line-Neutral AC RMS Voltage";
static const char dse_ps_4b586e72d4[] PROGMEM = "gen_vcn";
static const char dse_ps_d6bfc2a7a1[] PROGMEM = "Generator Phase C Line-Neutral AC RMS Voltage";
static const char dse_ps_9c1433d84c[] PROGMEM = "gen_vab";
static const char dse_ps_4f19013d02[] PROGMEM = "Generator Phase AB Line-Line AC RMS Voltage";
static const char dse_ps_9932d7bee8[] PROGMEM = "gen_vbc";
static const char dse_ps_a94e4e36a3[] PROGMEM = "Generator Phase BC Line-Line AC RMS Voltage";
static const char dse_ps_988f51d613[] PROGMEM = "gen_vca";
static const char dse_ps_42675f76db[] PROGMEM = "Generator Phase CA Line-Line AC RMS Voltage";
static const char dse_ps_9460430a03[] PROGMEM = "gen_ia";
static const char dse_ps_d9f203c8c7[] PROGMEM = "Generator Phase A AC RMS Current";
static const char dse_ps_51d4e81560[] PROGMEM = "gen_ib";
static const char dse_ps_c9661af9b8[] PROGMEM = "Generator Phase B AC RMS Current";
static const char dse_ps_82ac98907f[] PROGMEM = "gen_ic";
static const char dse_ps_e7ff17aa7c[] PROGMEM = "Generator Phase C AC RMS Current";
static const char dse_ps_6a94d0a41c[] PROGMEM = "gen_pa";
static const char dse_ps_84f8ee3eaa[] PROGMEM = "Generator Phase A Real Power";
static const char dse_ps_61e9c06ea9[] PROGMEM = "W";
static const char dse_ps_f913e84376[] PROGMEM = "gen_pb";
static const char dse_ps_e5b4786031[] PROGMEM = "Generator Phase B Real Power";
static const char dse_ps_3f1968ccbb[] PROGMEM = "gen_pc";
static const char dse_ps_606b895ff0[] PROGMEM = "Generator Phase C Real Power";
static const char dse_ps_2141f99a1a[] PROGMEM = "mains_freq";
static const char dse_ps_b7c1c2c6c4[] PROGMEM = "Utility / Mains Frequency";
static const char dse_ps_069e48d517[] PROGMEM = "mains_van";
static const char dse_ps_74338ba66d[] PROGMEM = "Utility Voltage L1";
static const char dse_ps_38a41c4c12[] PROGMEM = "mains_vbn";
static const char dse_ps_298bb12c5d[] PROGMEM = "Utility Voltage L2";
static const char dse_ps_c2db023abe[] PROGMEM = "mains_vcn";
static const char dse_ps_0d627646f5[] PROGMEM = "Utility Voltage L3";
static const char dse_k_mains_vab[] PROGMEM = "mains_vab";
static const char dse_n_mains_vab[] PROGMEM = "Utility Voltage L1-L2";
static const char dse_k_mains_vbc[] PROGMEM = "mains_vbc";
static const char dse_n_mains_vbc[] PROGMEM = "Utility Voltage L2-L3";
static const char dse_k_mains_vca[] PROGMEM = "mains_vca";
static const char dse_n_mains_vca[] PROGMEM = "Utility Voltage L3-L1";
static const char dse_ps_abeb9c1634[] PROGMEM = "gen_i_avg";
static const char dse_ps_e3c4824de1[] PROGMEM = "Generator Average AC RMS Current";
static const char dse_ps_78171ae0b4[] PROGMEM = "gen_p_tot";
static const char dse_ps_aac7229b24[] PROGMEM = "Generator Total Real Power";
static const char dse_ps_0fc899be8d[] PROGMEM = "gen_sa";
static const char dse_ps_580b9be508[] PROGMEM = "Generator Phase A Apparent Power";
static const char dse_ps_3a11129c34[] PROGMEM = "VA";
static const char dse_ps_5123f15a67[] PROGMEM = "gen_sb";
static const char dse_ps_e33eafd0e8[] PROGMEM = "Generator Phase B Apparent Power";
static const char dse_ps_2cc3a931a8[] PROGMEM = "gen_sc";
static const char dse_ps_276d78b634[] PROGMEM = "Generator Phase C Apparent Power";
static const char dse_ps_1a6bbc6fee[] PROGMEM = "gen_s_tot";
static const char dse_ps_332a614a14[] PROGMEM = "Generator Total Apparent Power";
static const char dse_ps_4fcec61049[] PROGMEM = "gen_qa";
static const char dse_ps_e1983552c7[] PROGMEM = "Generator Phase A Reactive Power";
static const char dse_ps_f54ce9273e[] PROGMEM = "VAr";
static const char dse_ps_2b4abef024[] PROGMEM = "gen_qb";
static const char dse_ps_af44f49361[] PROGMEM = "Generator Phase B Reactive Power";
static const char dse_ps_b8106ed467[] PROGMEM = "gen_qc";
static const char dse_ps_2a4e7e2bd0[] PROGMEM = "Generator Phase C Reactive Power";
static const char dse_ps_46d03e71ae[] PROGMEM = "gen_q_tot";
static const char dse_ps_dedf71f37b[] PROGMEM = "Generator Total Reactive Power";
static const char dse_ps_137fe31cea[] PROGMEM = "gen_pfa";
static const char dse_ps_2a6fa3c22d[] PROGMEM = "Generator Phase A Power Factor";
static const char dse_ps_3c1d3117da[] PROGMEM = "gen_pfb";
static const char dse_ps_4a0e550ed6[] PROGMEM = "Generator Phase B Power Factor";
static const char dse_ps_37750eb852[] PROGMEM = "gen_pfc";
static const char dse_ps_aad8316a87[] PROGMEM = "Generator Phase C Power Factor";
static const char dse_ps_03ceab18c2[] PROGMEM = "gen_pf";
static const char dse_ps_1fbf04bc09[] PROGMEM = "Generator Overall Power Factor";
static const char dse_ps_b51d5ce898[] PROGMEM = "gen_pct_kw";
static const char dse_ps_4fb277f4b9[] PROGMEM = "Engine / Generator Load Percent";
static const char dse_ps_3ad3c9db10[] PROGMEM = "gen_kwh_exp";
static const char dse_ps_6a1de93a4d[] PROGMEM = "AC Energy Forward / kWh Export";
static const char dse_ps_83994382f0[] PROGMEM = "kWh";
static const char dse_ps_763c70ca34[] PROGMEM = "eng_starts";
static const char dse_ps_9c87c30347[] PROGMEM = "Engine Start Count";
static const char dse_ps_e4f4e9c555[] PROGMEM = "led_stop";
static const char dse_ps_d86b85dda0[] PROGMEM = "Stop LED";
static const char dse_ps_63d5c19f7b[] PROGMEM = "led_manual";
static const char dse_ps_9231c572d5[] PROGMEM = "Manual LED";
static const char dse_ps_36eb4810fe[] PROGMEM = "led_test";
static const char dse_ps_98baa7b263[] PROGMEM = "Test LED";
static const char dse_ps_77bad85138[] PROGMEM = "led_auto";
static const char dse_ps_db4bbf5697[] PROGMEM = "Auto LED";
static const char dse_ps_02805e1935[] PROGMEM = "led_mains";
static const char dse_ps_61e47fb554[] PROGMEM = "Mains LED";
static const char dse_ps_01b6f0de77[] PROGMEM = "led_mains_brk";
static const char dse_ps_bcffb111d7[] PROGMEM = "Mains Breaker LED";
static const char dse_ps_b7e95f9ded[] PROGMEM = "led_gen_brk";
static const char dse_ps_6a13847102[] PROGMEM = "Gen Breaker LED";
static const char dse_ps_9ae004cb0c[] PROGMEM = "led_gen";
static const char dse_ps_8896f98994[] PROGMEM = "Gen LED";

const DseRegisterFlash kMap[] PROGMEM = {
    // --- Identity (DSE page 3) ---
    {768, 1, static_cast<uint8_t>(DseDecodeKind::IdentityU16), 0, 1.0f, dse_ps_7e167c1d10, dse_ps_3bcbfd7e64, dse_ps_d41d8cd98f, 0},
    {769, 1, static_cast<uint8_t>(DseDecodeKind::IdentityU16), 0, 1.0f, dse_ps_20f35e630d, dse_ps_dd20850f12, dse_ps_d41d8cd98f, 0},
    {770, 2, static_cast<uint8_t>(DseDecodeKind::IdentityU32), 0, 1.0f, dse_ps_74136b2421, dse_ps_78cf8ce506, dse_ps_d41d8cd98f, 0},

    // --- Generator / controller state ---
    {772, 1, static_cast<uint8_t>(DseDecodeKind::EnumOpMode), 0, 1.0f, dse_ps_1c7cc88fe1, dse_ps_fed6c84461, dse_ps_d41d8cd98f, 0},
    {774, 1, static_cast<uint8_t>(DseDecodeKind::IdentityU16), 0, 1.0f, dse_ps_d416e37937, dse_ps_594ccab04e, dse_ps_d41d8cd98f, 0},
    {785, 1, static_cast<uint8_t>(DseDecodeKind::IdentityU16), 0, 1.0f, dse_ps_1cc021520e, dse_ps_eed8dc9c33, dse_ps_d41d8cd98f, 0},
    {795, 1, static_cast<uint8_t>(DseDecodeKind::IdentityU16), 0, 1.0f, dse_ps_0274e235a6, dse_ps_1df23e2e92, dse_ps_d41d8cd98f, 0},
    {1408, 1, static_cast<uint8_t>(DseDecodeKind::EnumEngStatus), 0, 1.0f, dse_ps_5c314c6089, dse_ps_e101d50d52, dse_ps_d41d8cd98f, 0},
    {1418, 1, static_cast<uint8_t>(DseDecodeKind::EnumEngStatus), 0, 1.0f, dse_ps_729dae3919, dse_ps_8f1ea1332a, dse_ps_d41d8cd98f, 0},

    // --- Comm / misc status (7320MKII clone JSON) ---
    {6, 1, static_cast<uint8_t>(DseDecodeKind::IdentityU16), 0, 1.0f, dse_ps_35538962db, dse_ps_ade3d33eda, dse_ps_d41d8cd98f, 0},
    {128, 2, static_cast<uint8_t>(DseDecodeKind::Bitmask), 0, 1.0f, dse_ps_20ada7717d, dse_ps_d50d8afcd9, dse_ps_d41d8cd98f, 0},
    {258, 1, static_cast<uint8_t>(DseDecodeKind::IdentityU16), 0, 1.0f, dse_ps_6fc05f75b9, dse_ps_9170cd0393, dse_ps_d41d8cd98f, 0},

    // --- Engine / mechanical ---
    R16U(1024, 1.0f, dse_ps_f27b039bf5, dse_ps_ff57ab08eb, dse_ps_97d41f8341, 0),
    R16S(1025, 1.0f, dse_ps_2bd01ee0af, dse_ps_c39c590cfc, dse_ps_0d61f8370c, 0),
    R16S(1026, 1.0f, dse_ps_6a3bfcfa0d, dse_ps_7ea431cdca, dse_ps_0d61f8370c, 0),
    R16U(1027, 1.0f, dse_ps_f1b51323c3, dse_ps_80a55f5983, dse_ps_0bcef9c45b, 0),
    R16U(1028, 0.1f, dse_ps_1c2eff267e, dse_ps_78a74de0e0, dse_ps_5206560a30, 0),
    R16U(1029, 0.1f, dse_ps_983590dc35, dse_ps_88f7e267b8, dse_ps_5206560a30, 167),
    R16U(1030, 1.0f, dse_ps_f405dddd07, dse_ps_8fadf4417c, dse_ps_455385f5d8, 589),
    R16U(1357, 1.0f, dse_ps_d378782718, dse_ps_a6d699fb29, dse_ps_0bcef9c45b, 0),
    R16U(1359, 1.0f, dse_ps_7afa802402, dse_ps_3fafe493c2, dse_ps_0bcef9c45b, 0),
    R32S(1256, 0.001f, dse_ps_75793476a4, dse_ps_8d672bad48, dse_ps_7fc56270e7, 0),
    R32S(1258, 0.001f, dse_ps_29518834a1, dse_ps_afed100c18, dse_ps_5206560a30, 0),

    // --- AC electrical (S6 + DSE family) ---
    R16U(1031, 0.1f, dse_ps_9574a34829, dse_ps_8987176615, dse_ps_d1e2634cb7, 2436),
    R32U(1032, 0.1f, dse_ps_1cbee4df4a, dse_ps_3416ba27d1, dse_ps_5206560a30, 2445),
    R32U(1034, 0.1f, dse_ps_928a324b0c, dse_ps_e1d0102ee4, dse_ps_5206560a30, 2446),
    R32U(1036, 0.1f, dse_ps_4b586e72d4, dse_ps_d6bfc2a7a1, dse_ps_5206560a30, 2447),
    R32U(1038, 0.1f, dse_ps_9c1433d84c, dse_ps_4f19013d02, dse_ps_5206560a30, 2441),
    R32U(1040, 0.1f, dse_ps_9932d7bee8, dse_ps_a94e4e36a3, dse_ps_5206560a30, 2442),
    R32U(1042, 0.1f, dse_ps_988f51d613, dse_ps_42675f76db, dse_ps_5206560a30, 2443),
    R32U(1044, 0.1f, dse_ps_9460430a03, dse_ps_d9f203c8c7, dse_ps_7fc56270e7, 2449),
    R32U(1046, 0.1f, dse_ps_51d4e81560, dse_ps_c9661af9b8, dse_ps_7fc56270e7, 2450),
    R32U(1048, 0.1f, dse_ps_82ac98907f, dse_ps_e7ff17aa7c, dse_ps_7fc56270e7, 2451),
    R32S(1052, 1.0f, dse_ps_6a94d0a41c, dse_ps_84f8ee3eaa, dse_ps_61e9c06ea9, 2453),
    R32S(1054, 1.0f, dse_ps_f913e84376, dse_ps_e5b4786031, dse_ps_61e9c06ea9, 2454),
    R32S(1056, 1.0f, dse_ps_3f1968ccbb, dse_ps_606b895ff0, dse_ps_61e9c06ea9, 2455),
    // Utility / mains (page 4) — from 7320MKII clone JSON
    R16U(1059, 0.1f, dse_ps_2141f99a1a, dse_ps_b7c1c2c6c4, dse_ps_d1e2634cb7, 0),
    // GenComm 1060 is uint32, high word first. The high word is 0 at mains
    // voltage, so a 16-bit read of 1060 alone shows 0 V.
    R32U(1060, 0.1f, dse_ps_069e48d517, dse_ps_74338ba66d, dse_ps_5206560a30, 0),
    R32U(1062, 0.1f, dse_ps_38a41c4c12, dse_ps_298bb12c5d, dse_ps_5206560a30, 0),
    R32U(1064, 0.1f, dse_ps_c2db023abe, dse_ps_0d627646f5, dse_ps_5206560a30, 0),
    R32U(1066, 0.1f, dse_k_mains_vab, dse_n_mains_vab, dse_ps_5206560a30, 0),
    R32U(1068, 0.1f, dse_k_mains_vbc, dse_n_mains_vbc, dse_ps_5206560a30, 0),
    R32U(1070, 0.1f, dse_k_mains_vca, dse_n_mains_vca, dse_ps_5206560a30, 0),
    R32U(1666, 0.1f, dse_ps_abeb9c1634, dse_ps_e3c4824de1, dse_ps_7fc56270e7, 2448),
    R32S(1536, 1.0f, dse_ps_78171ae0b4, dse_ps_aac7229b24, dse_ps_61e9c06ea9, 2452),
    R32U(1538, 1.0f, dse_ps_0fc899be8d, dse_ps_580b9be508, dse_ps_3a11129c34, 2461),
    R32U(1540, 1.0f, dse_ps_5123f15a67, dse_ps_e33eafd0e8, dse_ps_3a11129c34, 2462),
    R32U(1542, 1.0f, dse_ps_2cc3a931a8, dse_ps_276d78b634, dse_ps_3a11129c34, 2463),
    R32S(1544, 1.0f, dse_ps_1a6bbc6fee, dse_ps_332a614a14, dse_ps_3a11129c34, 2460),
    R32S(1546, 1.0f, dse_ps_4fcec61049, dse_ps_e1983552c7, dse_ps_f54ce9273e, 2457),
    R32S(1548, 1.0f, dse_ps_2b4abef024, dse_ps_af44f49361, dse_ps_f54ce9273e, 2458),
    R32S(1550, 1.0f, dse_ps_b8106ed467, dse_ps_2a4e7e2bd0, dse_ps_f54ce9273e, 2459),
    R32S(1552, 1.0f, dse_ps_46d03e71ae, dse_ps_dedf71f37b, dse_ps_f54ce9273e, 2456),
    R16S(1554, 0.01f, dse_ps_137fe31cea, dse_ps_2a6fa3c22d, dse_ps_d41d8cd98f, 2465),
    R16S(1555, 0.01f, dse_ps_3c1d3117da, dse_ps_4a0e550ed6, dse_ps_d41d8cd98f, 2466),
    R16S(1556, 0.01f, dse_ps_37750eb852, dse_ps_aad8316a87, dse_ps_d41d8cd98f, 2467),
    R16S(1557, 0.01f, dse_ps_03ceab18c2, dse_ps_1fbf04bc09, dse_ps_d41d8cd98f, 2464),
    R16S(1558, 0.1f, dse_ps_b51d5ce898, dse_ps_4fb277f4b9, dse_ps_0bcef9c45b, 3590),

    // --- Accumulators ---
    {1798, 2, static_cast<uint8_t>(DseDecodeKind::HoursFromSeconds), 0, 1.0f, dse_ps_878265f8a5, dse_ps_ff63ed2bf6, dse_ps_2510c39011, 0},
    R32U(1800, 0.1f, dse_ps_3ad3c9db10, dse_ps_6a1de93a4d, dse_ps_83994382f0, 2468),
    R32U(1808, 1.0f, dse_ps_763c70ca34, dse_ps_9c87c30347, dse_ps_d41d8cd98f, 0),

    // --- Alarm page headers (7320MKII clone JSON) ---
    {2048, 1, static_cast<uint8_t>(DseDecodeKind::IdentityU16), 0, 1.0f, dse_ps_30a3b500a8, dse_ps_e222d40499, dse_ps_d41d8cd98f, 0},
    {39168, 1, static_cast<uint8_t>(DseDecodeKind::Bitmask), 0, 1.0f, dse_ps_bde6bc6a53, dse_ps_e314e89d45, dse_ps_d41d8cd98f, 0},
    {39424, 1, static_cast<uint8_t>(DseDecodeKind::IdentityU16), 0, 1.0f, dse_ps_f5c045c3fc, dse_ps_afb0925676, dse_ps_d41d8cd98f, 0},

    // --- SCF support flags (read); control writes are FC16 @4104/4105 ---
    {4096, 1, static_cast<uint8_t>(DseDecodeKind::Bitmask), 0, 1.0f, dse_ps_05b8c96b83, dse_ps_7c82abc3a7, dse_ps_d41d8cd98f, 0},
    {4097, 1, static_cast<uint8_t>(DseDecodeKind::Bitmask), 0, 1.0f, dse_ps_8c3fe0361a, dse_ps_e019d3752b, dse_ps_d41d8cd98f, 0},
    {4098, 1, static_cast<uint8_t>(DseDecodeKind::Bitmask), 0, 1.0f, dse_ps_ad081cd0f2, dse_ps_46632d858a, dse_ps_d41d8cd98f, 0},
    {4099, 1, static_cast<uint8_t>(DseDecodeKind::Bitmask), 0, 1.0f, dse_ps_4d34f14f45, dse_ps_fd15ff7def, dse_ps_d41d8cd98f, 0},
    {4100, 1, static_cast<uint8_t>(DseDecodeKind::Bitmask), 0, 1.0f, dse_ps_f02d77658c, dse_ps_7ec28d3738, dse_ps_d41d8cd98f, 0},
    {4101, 1, static_cast<uint8_t>(DseDecodeKind::Bitmask), 0, 1.0f, dse_ps_58913a2528, dse_ps_da7a485b8d, dse_ps_d41d8cd98f, 0},
    {4102, 1, static_cast<uint8_t>(DseDecodeKind::Bitmask), 0, 1.0f, dse_ps_ca951e6e8c, dse_ps_00adba35d6, dse_ps_d41d8cd98f, 0},
    {4103, 1, static_cast<uint8_t>(DseDecodeKind::Bitmask), 0, 1.0f, dse_ps_f6241a4d41, dse_ps_d0e62e59ae, dse_ps_d41d8cd98f, 0},
    {4104, 1, static_cast<uint8_t>(DseDecodeKind::IdentityU16), 0, 1.0f, dse_ps_4effb8a17e, dse_ps_2680d6fe93, dse_ps_d41d8cd98f, 0},
    {4105, 1, static_cast<uint8_t>(DseDecodeKind::IdentityU16), 0, 1.0f, dse_ps_91542b952c, dse_ps_eef0fd92f6, dse_ps_d41d8cd98f, 0},

    // --- Digital input alarm pages ---
    {39169, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_6138c1e393, dse_ps_d3a0bd95ae, dse_ps_d41d8cd98f, 0},
    {39170, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_97a6b39cb5, dse_ps_176be0bea8, dse_ps_d41d8cd98f, 0},
    {39171, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_127c1ff8d0, dse_ps_d9a414e033, dse_ps_d41d8cd98f, 0},
    {39172, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_59a976fe30, dse_ps_d4d7edd525, dse_ps_d41d8cd98f, 0},

    // --- Standard alarms DSE page 154 ---
    {39425, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_10982cb7dc, dse_ps_535599d73d, dse_ps_d41d8cd98f, 0},
    {39426, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_4d7c78bc5a, dse_ps_133c933471, dse_ps_d41d8cd98f, 0},
    {39427, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_2fc4f9a395, dse_ps_282c5f5069, dse_ps_d41d8cd98f, 0},
    {39428, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_a1a81f9129, dse_ps_b05fb791ad, dse_ps_d41d8cd98f, 0},
    {39429, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_81d6890536, dse_ps_da7184c3d4, dse_ps_d41d8cd98f, 0},
    {39430, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_8919637575, dse_ps_d739b94954, dse_ps_d41d8cd98f, 0},
    {39431, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_3f8a514833, dse_ps_dcfa7cabf4, dse_ps_d41d8cd98f, 0},
    {39432, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_19d58cd91f, dse_ps_7ad825e835, dse_ps_d41d8cd98f, 0},
    {39433, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_9315eb0456, dse_ps_71a6df89c7, dse_ps_d41d8cd98f, 0},
    {39434, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_453d02fb09, dse_ps_aea7b9fba9, dse_ps_d41d8cd98f, 0},
    {39435, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_d9db2df72b, dse_ps_19dbb6e61b, dse_ps_d41d8cd98f, 0},
    {39436, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_fcedaea47e, dse_ps_9697d36bd0, dse_ps_d41d8cd98f, 0},
    {39437, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_66ca78663a, dse_ps_e5275ce396, dse_ps_d41d8cd98f, 0},
    {39438, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_3f893f6495, dse_ps_635aac0f6f, dse_ps_d41d8cd98f, 0},
    {39439, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_c29f4858c4, dse_ps_ddac102d91, dse_ps_d41d8cd98f, 0},
    {39440, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_90a2246b60, dse_ps_5a40e31a50, dse_ps_d41d8cd98f, 0},
    {39441, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_ca8df72474, dse_ps_f74b8d407c, dse_ps_d41d8cd98f, 0},
    {39442, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_02b76db363, dse_ps_afb3860fb3, dse_ps_d41d8cd98f, 0},
    {39443, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_a60a1528ea, dse_ps_b78ebae358, dse_ps_d41d8cd98f, 0},
    {39444, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_84f8c330e4, dse_ps_69142ba7fc, dse_ps_d41d8cd98f, 0},
    {39445, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_e9aee87419, dse_ps_27a3e160e4, dse_ps_d41d8cd98f, 0},

    // --- PLC / user alarms ---
    {39476, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_b5327b9d58, dse_ps_7bb7586adb, dse_ps_d41d8cd98f, 0},
    {39477, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_651c282f35, dse_ps_9558935667, dse_ps_d41d8cd98f, 0},
    {39478, 1, static_cast<uint8_t>(DseDecodeKind::AlarmPacked), 0, 1.0f, dse_ps_bc52747cb9, dse_ps_91554a8c69, dse_ps_d41d8cd98f, 0},

    // --- Front-panel LED mirrors (S6 map) ---
    R16U(46648, 1.0f, dse_ps_e4f4e9c555, dse_ps_d86b85dda0, dse_ps_d41d8cd98f, 521839),
    R16U(46649, 1.0f, dse_ps_63d5c19f7b, dse_ps_9231c572d5, dse_ps_d41d8cd98f, 521840),
    R16U(46650, 1.0f, dse_ps_36eb4810fe, dse_ps_98baa7b263, dse_ps_d41d8cd98f, 521841),
    R16U(46651, 1.0f, dse_ps_77bad85138, dse_ps_db4bbf5697, dse_ps_d41d8cd98f, 521842),
    R16U(46652, 1.0f, dse_ps_02805e1935, dse_ps_61e47fb554, dse_ps_d41d8cd98f, 521843),
    R16U(46653, 1.0f, dse_ps_01b6f0de77, dse_ps_bcffb111d7, dse_ps_d41d8cd98f, 521844),
    R16U(46654, 1.0f, dse_ps_b7e95f9ded, dse_ps_6a13847102, dse_ps_d41d8cd98f, 521845),
    R16U(46655, 1.0f, dse_ps_9ae004cb0c, dse_ps_8896f98994, dse_ps_d41d8cd98f, 521846),
};

constexpr size_t kMapCount = sizeof(kMap) / sizeof(kMap[0]);

// Slot order: nibble0 (bits0-3) .. nibble3 (bits12-15)
// Matches DSE alarm table columns right→left: Bits4-1 … Bits16-13
struct AlarmNamesFlash {
  uint16_t address;
  PGM_P slot[4];
};

const AlarmNamesFlash kAlarmNames[] PROGMEM = {
    {39425,
     {dse_ps_2f4571e67f, dse_ps_95143a97af,
      dse_ps_b76eb8a4d8, dse_ps_5873a7a8a7}},
    {39426,
     {dse_ps_e68935ed68, dse_ps_568953dc21,
      dse_ps_ff83a35cfb, dse_ps_0e77d4ff67}},
    {39427,
     {dse_ps_bf010ec1af, dse_ps_1f62a3ca04,
      dse_ps_e999c3d907, dse_ps_f55c09161d}},
    {39428,
     {dse_ps_1c83a08082, dse_ps_89dc13b27c,
      dse_ps_3d1e1e3d88, dse_ps_bdda68fb33}},
    {39429,
     {dse_ps_3f10cb6289, dse_ps_19e3245059,
      dse_ps_905724fa35, dse_ps_a0b8a0d86f}},
    {39430,
     {dse_ps_58f1e13833, dse_ps_114e36360c,
      dse_ps_854ffef6a5, dse_ps_01d9266dcc}},
    {39431,
     {dse_ps_193fc5f5b9, dse_ps_ef010f1d6a,
      dse_ps_370d896287, dse_ps_162406935c}},
    {39432,
     {dse_ps_593adf9a72, dse_ps_d9308e074d,
      dse_ps_6bcd5c0fc4, dse_ps_ab1c7e33ae}},
    {39433,
     {dse_ps_6451a38fba, dse_ps_5a8324d612,
      dse_ps_c91820e43b, dse_ps_ed1f62b070}},
    {39434,
     {dse_ps_bc0c24f899, dse_ps_407739b4a1,
      dse_ps_166f95288f, dse_ps_d67ee80f1b}},
    {39435,
     {dse_ps_c311d66214, dse_ps_fce9aae0d8,
      dse_ps_e83f52263c, dse_ps_56754b5158}},
    {39436,
     {dse_ps_ea049685a8, dse_ps_bf1f6a21ad,
      dse_ps_5bb90da3c4, dse_ps_95029306d8}},
    {39437,
     {dse_ps_6235197a28, dse_ps_dc45c845d4,
      dse_ps_1df6b3b27d, dse_ps_ccb77aab70}},
    {39438,
     {dse_ps_9d2d1b40f1, dse_ps_be5503f06e,
      dse_ps_7419abd307, dse_ps_85dce448b2}},
    {39439,
     {dse_ps_4f73f55cd9, dse_ps_67e068ad51,
      dse_ps_14befac4c7, dse_ps_db76bb5c4a}},
    {39440,
     {dse_ps_d19a98444c, dse_ps_7df2373690,
      dse_ps_d1c32ebbc2, dse_ps_c483f09df4}},
    {39441,
     {dse_ps_6945d551a1, dse_ps_2769af4f41,
      dse_ps_64534d88cf, dse_ps_1cfd4bc545}},
    {39442,
     {dse_ps_dc6e00916a, dse_ps_3e9327e63c,
      dse_ps_df62a6b362, dse_ps_381c8c1bea}},
    {39443,
     {dse_ps_0d8f4474ea, dse_ps_0048ae9d16,
      dse_ps_ca3c9c2746, dse_ps_efc0c93976}},
    {39444,
     {dse_ps_d90c055ad0, dse_ps_ad99d9677a,
      dse_ps_42ddee8523, dse_ps_a16e4b5e33}},
    {39445,
     {dse_ps_0bb882c198, dse_ps_2d9e181236,
      dse_ps_5cf195b029, dse_ps_de19d59c28}},
    {39169,
     {dse_ps_78ce164c32, dse_ps_8aaf392095, dse_ps_fbea5a1989, dse_ps_496096c68d}},
    {39170,
     {dse_ps_ccf8834d92, dse_ps_254c8c1769, dse_ps_3997f020f8, dse_ps_38e1c64319}},
    {39171,
     {dse_ps_d8497aaa93, dse_ps_1073950751, dse_ps_9182b4c3bc, dse_ps_e3f8b46b37}},
    {39172,
     {dse_ps_af7e9ab2da, dse_ps_1e34ff8651, dse_ps_ad08ccddac, dse_ps_9b8423266a}},
    {39476,
     {dse_ps_0962a00a7a, dse_ps_3e41b22dd0, dse_ps_b039df3b88,
      dse_ps_b6d0ee980e}},
    {39477,
     {dse_ps_577fb09115, dse_ps_5276791421, dse_ps_b95b1f27a4,
      dse_ps_5ddc03e96f}},
    {39478,
     {dse_ps_23e155f98f, dse_ps_944917c378, dse_ps_15ada885b5,
      dse_ps_593adf9a72}},
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

bool inSentinelBand(const DseRegister &reg, uint32_t rawU) {
  // GenComm instrumentation sentinels, eight codes ending at unimplemented.
  // Signed: 0x7FF8..0x7FFF. Unsigned: 0xFFF8..0xFFFF (and the 32-bit forms).
  if (reg.width == DseDataWidth::Int16) {
    if (reg.isSigned) {
      return rawU >= 0x7FF8u && rawU <= 0x7FFFu;
    }
    return rawU >= 0xFFF8u && rawU <= 0xFFFFu;
  }
  if (reg.isSigned) {
    return rawU >= 0x7FFFFFF8u && rawU <= 0x7FFFFFFFu;
  }
  return rawU >= 0xFFFFFFF8u;
}

const char *sentinelName(const DseRegister &reg, uint32_t rawU) {
  uint32_t base = 0;
  if (reg.width == DseDataWidth::Int16) {
    base = reg.isSigned ? 0x7FF8u : 0xFFF8u;
  } else {
    base = reg.isSigned ? 0x7FFFFFF8u : 0xFFFFFFF8u;
  }
  if (rawU < base || rawU > base + 7u) {
    return "unavailable";
  }
  switch (rawU - base) {
    case 0:
      return "reserved";
    case 1:
      return "low input";
    case 2:
      return "high input";
    case 3:
      return "bad data";
    case 4:
      return "transducer fault";
    case 5:
      return "under range";
    case 6:
      return "over range";
    default:
      return "unimplemented";
  }
}

bool isInvalidScaled(const DseRegister &reg, int32_t rawS, uint32_t rawU) {
  (void)rawS;
  return inSentinelBand(reg, rawU);
}

void loadRegister(size_t index, DseRegister *out) {
  DseRegisterFlash f;
  memcpy_P(&f, &kMap[index], sizeof(f));
  out->address = f.address;
  out->width = static_cast<DseDataWidth>(f.width);
  out->kind = static_cast<DseDecodeKind>(f.kind);
  out->isSigned = f.isSigned != 0;
  out->factor = f.factor;
  out->spn = f.spn;
  strncpy_P(out->key, f.key, sizeof(out->key) - 1);
  out->key[sizeof(out->key) - 1] = '\0';
  strncpy_P(out->name, f.name, sizeof(out->name) - 1);
  out->name[sizeof(out->name) - 1] = '\0';
  strncpy_P(out->unit, f.unit, sizeof(out->unit) - 1);
  out->unit[sizeof(out->unit) - 1] = '\0';
}

bool loadAlarmNames(uint16_t address, AlarmNamesFlash *out) {
  for (size_t i = 0; i < kAlarmNameCount; i++) {
    AlarmNamesFlash row;
    memcpy_P(&row, &kAlarmNames[i], sizeof(row));
    if (row.address == address) {
      *out = row;
      return true;
    }
  }
  return false;
}

void copyAlarmSlot(const AlarmNamesFlash &names, uint8_t slot, char *buf,
                   size_t buflen) {
  if (!buf || buflen == 0) {
    return;
  }
  if (slot < 4) {
    strncpy_P(buf, names.slot[slot], buflen - 1);
  } else {
    buf[0] = '\0';
    return;
  }
  buf[buflen - 1] = '\0';
}

void printAlarmPacked(uint16_t address, uint16_t raw, bool activeOnly) {
  AlarmNamesFlash names{};
  const bool haveNames = loadAlarmNames(address, &names);
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
  char labelBuf[40];
  for (uint8_t slot = 0; slot < 4; slot++) {
    const uint8_t nibble = static_cast<uint8_t>((raw >> (slot * 4)) & 0xF);
    if (activeOnly && (nibble == 0 || nibble == 1 || nibble == 15)) {
      continue;
    }
    if (haveNames) {
      copyAlarmSlot(names, slot, labelBuf, sizeof(labelBuf));
    } else {
      strncpy(labelBuf, DseRegisters::alarmSlotName(address, slot),
              sizeof(labelBuf) - 1);
      labelBuf[sizeof(labelBuf) - 1] = '\0';
    }
    Serial.print(F("  ["));
    Serial.print(slot);
    Serial.print(F("] "));
    Serial.print(labelBuf);
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

const DseRegister &at(size_t index) {
  static DseRegister buf;
  if (index >= kMapCount) {
    index = 0;
  }
  loadRegister(index, &buf);
  return buf;
}

const DseRegister *findByKey(const char *key) {
  static DseRegister scratch;
  if (!key || !*key) {
    return nullptr;
  }
  for (size_t i = 0; i < kMapCount; i++) {
    loadRegister(i, &scratch);
    if (strEqualsIgnoreCase(scratch.key, key) == 0) {
      return &scratch;
    }
  }
  return nullptr;
}

const DseRegister *findByAddress(uint16_t address) {
  static DseRegister scratch;
  for (size_t i = 0; i < kMapCount; i++) {
    const uint16_t addr = pgm_read_word(&kMap[i].address);
    if (addr == address) {
      loadRegister(i, &scratch);
      return &scratch;
    }
  }
  return nullptr;
}

const DseRegister *findByIndex1(size_t oneBased) {
  static DseRegister scratch;
  if (oneBased < 1 || oneBased > kMapCount) {
    return nullptr;
  }
  loadRegister(oneBased - 1, &scratch);
  return &scratch;
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
  static char buf[40];
  AlarmNamesFlash names{};
  if (loadAlarmNames(address, &names) && slot0to3 < 4) {
    copyAlarmSlot(names, slot0to3, buf, sizeof(buf));
    return buf;
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
    case DseDecodeKind::EnumOpMode:
    case DseDecodeKind::EnumEngStatus:
      v.invalid = (rawU == 0xFFFFu);
      v.engineering = static_cast<float>(rawU);
      break;
    case DseDecodeKind::Bitmask:
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
  if (reg.unit[0]) {
    Serial.print(F(" ["));
    Serial.print(reg.unit);
    Serial.print(']');
  }
  Serial.println();
}

void printList() {
  Serial.println(F("## DSE register map (FC03)"));
  Serial.print(F("Entries: "));
  Serial.println(kMapCount);
  for (size_t i = 0; i < kMapCount; i++) {
    DseRegister reg;
    loadRegister(i, &reg);
    Serial.print(i + 1);
    Serial.print(F(". "));
    printEntry(reg);
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
    Serial.print(sentinelName(r, v.rawU));
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
        if (r.unit[0]) {
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
  if (strcmp(reg.key, "mains_vab") == 0) {
    return "Mains L1-L2 voltage";
  }
  if (strcmp(reg.key, "mains_vbc") == 0) {
    return "Mains L2-L3 voltage";
  }
  if (strcmp(reg.key, "mains_vca") == 0) {
    return "Mains L3-L1 voltage";
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
    DseRegister r;
    loadRegister(i, &r);
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
      Serial.println(sentinelName(r, v.rawU));
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
        if (r.unit[0]) {
          Serial.print(' ');
          Serial.print(r.unit);
        }
        Serial.println();
        break;
    }
  }
}

}  // namespace DseRegisters

#endif  // !GATEWAY_LITE
