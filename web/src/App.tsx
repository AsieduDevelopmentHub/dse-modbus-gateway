import { useEffect, useState } from "react";
import { fetchStatus } from "./api";
import type { GatewayStatus } from "./types";

function fmt(v: number | null | undefined, digits: number): string {
  if (v === null || v === undefined || Number.isNaN(v)) {
    return "—";
  }
  return v.toFixed(digits);
}

function linkPill(link: string) {
  if (link === "ok") {
    return <span className="pill ok">link ok</span>;
  }
  if (link === "degraded") {
    return <span className="pill warn">degraded</span>;
  }
  if (link === "fail") {
    return <span className="pill bad">link fail</span>;
  }
  return <span className="pill">link ?</span>;
}

function Metric({
  label,
  value,
  unit,
}: {
  label: string;
  value: string;
  unit?: string;
}) {
  return (
    <div className="metric">
      <div className="label">{label}</div>
      <div className="value">
        {value}
        {unit ? <span className="unit">{unit}</span> : null}
      </div>
    </div>
  );
}

export function App() {
  const [status, setStatus] = useState<GatewayStatus | null>(null);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    let cancelled = false;
    const tick = async () => {
      try {
        const next = await fetchStatus();
        if (!cancelled) {
          setStatus(next);
          setError(null);
        }
      } catch (e) {
        if (!cancelled) {
          setError(e instanceof Error ? e.message : "fetch failed");
        }
      }
    };
    tick();
    const id = window.setInterval(tick, 2000);
    return () => {
      cancelled = true;
      window.clearInterval(id);
    };
  }, []);

  const stale = (status?.ageMs ?? 0) > 8000;
  const mode = status?.opMode?.name ?? "—";
  const eng = status?.engStatus?.name ?? "—";

  return (
    <div className={`app${stale ? " stale" : ""}`}>
      <header className="hero">
        <h1 className="brand">DSE Gateway</h1>
        <p className="support">
          {mode} · {eng}
        </p>
        <p className="headline">{status?.headline ?? (error ? "Offline" : "…")}</p>
        <div className="pills">
          {status ? linkPill(status.link) : null}
          {stale ? <span className="pill warn">stale</span> : null}
          {import.meta.env.VITE_USE_MOCK === "1" ? (
            <span className="pill">mock</span>
          ) : null}
        </div>
        <p className="meta">
          {status
            ? `slave ${status.slaveId} · ${status.wifiMode} ${status.wifiIp}` +
              (status.rssi ? ` · ${status.rssi} dBm` : "") +
              ` · age ${status.ageMs} ms` +
              (status.lastError ? ` · ${status.lastError}` : "")
            : "Waiting for /api/status…"}
        </p>
        {error ? <p className="err">{error}</p> : null}
      </header>

      <div className="metrics">
        <Metric label="Battery" value={fmt(status?.batteryV, 1)} unit="V" />
        <Metric label="Engine RPM" value={fmt(status?.engRpm, 0)} />
        <Metric label="Frequency" value={fmt(status?.genFreq, 1)} unit="Hz" />
        <Metric label="L1 voltage" value={fmt(status?.genVan, 1)} unit="V" />
        <Metric label="L2 voltage" value={fmt(status?.genVbn, 1)} unit="V" />
        <Metric label="L3 voltage" value={fmt(status?.genVcn, 1)} unit="V" />
        <Metric label="L1 current" value={fmt(status?.genIa, 1)} unit="A" />
        <Metric label="L2 current" value={fmt(status?.genIb, 1)} unit="A" />
        <Metric label="L3 current" value={fmt(status?.genIc, 1)} unit="A" />
        <Metric label="Total power" value={fmt(status?.genPTotKw, 2)} unit="kW" />
        <Metric label="Load" value={fmt(status?.genPctKw, 1)} unit="%" />
      </div>

      <section className="section">
        <h2>Active alarms</h2>
        {!status?.alarms?.length ? (
          <p className="empty">No active alarms</p>
        ) : (
          <ul className="alarms">
            {status.alarms.map((a) => (
              <li key={`${a.addr}-${a.slot}`}>
                <span className="state">{a.state}</span> — {a.name}{" "}
                <span className="meta">
                  @{a.addr}[{a.slot}]
                </span>
              </li>
            ))}
          </ul>
        )}
      </section>

      <footer>
        Polls /api/status every 2s · same contract as ESP32 local page
      </footer>
    </div>
  );
}
