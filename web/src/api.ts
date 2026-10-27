import { mockStatus } from "./mockStatus";
import type { GatewayStatus } from "./types";

const useMock = import.meta.env.VITE_USE_MOCK === "1";

export async function fetchStatus(): Promise<GatewayStatus> {
  if (useMock) {
    await new Promise((r) => setTimeout(r, 120));
    return {
      ...mockStatus,
      ageMs: Math.floor(Math.random() * 800),
      uptimeMs: mockStatus.uptimeMs + Date.now() % 100000,
    };
  }
  const res = await fetch("/api/status");
  if (!res.ok) {
    throw new Error(`HTTP ${res.status}`);
  }
  return (await res.json()) as GatewayStatus;
}
