# DSE Gateway — React overview

Consumes the same JSON as the ESP32 local page (`GET /api/status`).

Package manager: **pnpm**.

## Run

```bash
cd web
pnpm install

# Mock data (no hardware)
pnpm dev:mock

# Proxy to gateway AP (default http://192.168.4.1)
pnpm dev

# Custom gateway IP
# Windows PowerShell:
$env:VITE_GW_URL="http://192.168.1.50"; pnpm dev
```

Open http://localhost:5173/

## Build

```bash
pnpm build
```

Static output in `dist/` (host separately; ESP32 continues to serve its thin embedded page).
