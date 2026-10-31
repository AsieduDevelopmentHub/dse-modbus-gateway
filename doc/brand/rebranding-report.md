# iCanOva Rebranding Report

**Date:** 2026-10-31  
**Classification:** PARTIALLY COMPLETED — local brand migration done; GitHub rename and some env rotations need operator authorization.

## 1. Repository migration

| Item | Previous | New | Status |
| ---- | -------- | --- | ------ |
| Gateway GitHub | `AsieduDevelopmentHub/dse-modbus-gateway` | `icanova-edge-gateway` | Done on GitHub |
| Web GitHub | `AsieduDevelopmentHub/dse-gateway-web` | `icanova-monitoring-web` | Done on GitHub |
| Local directories | `dse-modbus-gateway`, `dse-gateway-web` | `icanova-edge-gateway`, `icanova-monitoring-web` | Script ready — Cursor locks folders while open; run `D:\dev\rename-icanova-folders.ps1` after closing this workspace |
| Git remotes | old URLs | new canonical URLs | Done — `git remote set-url` + fetch OK |
| package.json (web) | `dse-gateway-web` | `icanova-monitoring-web` | Done |

## 2. Branding implementation

| Component | Status |
| --------- | ------ |
| Brand identity documentation (`doc/brand/`) | Done |
| Application title / metadata | Done |
| Navigation branding | Done |
| Colour tokens | Done (web CSS + embedded overview) |
| Typography | Done (Manrope / Inter / IBM Plex Mono on web) |
| Gateway documentation / README | Done |
| Web documentation / README | Done |
| Hardware identity references | Documented; PCB/electrical unchanged |
| Temporary wordmark | Done (text `iCanOva`) |
| Soft-AP SSID prefix `DSE-GW` | **Deliberately unchanged** (field compatibility) |
| Auth JWT audience defaults | **Deliberately unchanged** (Auth0 / security) |
| DSE equipment / register docs | **Preserved** |

## 3. Technical verification

| Test | Result |
| ---- | ------ |
| Gateway build (`pio run -e esp32-c3-supermini`) | PASSED |
| Web build (`pnpm build`) | PASSED |
| Frontend unit tests (`pnpm test`) | PASSED — 155 / 155 |
| Authentication verification | Not re-run against staging (no secret rotation) |
| Organization isolation | Untouched |
| Device communication | Untouched (Modbus / GenComm / SCF preserved) |
| Deployment verification | Remotes not renamed |

## 4. Outstanding work

1. Authorized GitHub rename of both repositories; then `git remote set-url origin …`.
2. Optional local directory rename after remotes move (update multi-root workspace).
3. Operator decision on rotating Auth0 audience from any remaining `dse-gateway` values to `icanova` in all environments.
4. Optional soft-AP SSID migration plan if field units can be reconfigured together.
5. Final logo artwork when approved.
6. Full UI/UX redesign (explicitly out of scope).

## 5. Final assessment

**PARTIALLY COMPLETED** — product identity, tokens, documentation, and safe firmware/web strings are on iCanOva; repository hosting names and credential/audience migration remain operator actions.
