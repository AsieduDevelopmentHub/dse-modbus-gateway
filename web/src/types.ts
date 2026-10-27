/** Mirrors ESP32 GET /api/status */

export type NamedCode = {
  code: number;
  name: string;
};

export type AlarmItem = {
  addr: number;
  slot: number;
  nibble: number;
  name: string;
  state: string;
};

export type GatewayStatus = {
  ok: boolean;
  ageMs: number;
  slaveId: number;
  rssi: number;
  wifiMode: string;
  wifiIp: string;
  uptimeMs: number;
  lastError: string;
  link: "ok" | "degraded" | "fail" | "unknown" | string;
  headline: string;
  opMode: NamedCode | null;
  engStatus: NamedCode | null;
  engRpm: number | null;
  batteryV: number | null;
  genFreq: number | null;
  genVan: number | null;
  genVbn: number | null;
  genVcn: number | null;
  genIa: number | null;
  genIb: number | null;
  genIc: number | null;
  genPTotW: number | null;
  genPTotKw: number | null;
  genPctKw: number | null;
  alarms: AlarmItem[];
};
