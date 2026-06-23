# espframe fork — issue triage

Fork: https://github.com/PeterHing/espframe (private)
Upstream: https://github.com/jtenniswood/espframe
Local clone: `/home/user/workspace/espframe` (origin = fork, upstream = jtenniswood)
Upstream latest release: **v1.10.7** (10 Jun 2026) — nothing unreleased on `main`.

## Priority lanes

### P0 — Bugs that break the product on current firmware
| # | Issue | State | Likely location | Notes |
|---|-------|-------|-----------------|-------|
| [#100](https://github.com/jtenniswood/espframe/issues/100) | People source only shows newest photo | v1.10.6 & v1.10.7 — **Peter affected** | `common/addon/immich_api.yaml` + `components/espframe/immich_helpers.h` | Logs show same UUID looping in slots 1 & 2; both report `NO COMPANION`. Likely a pagination / `assetId` dedup bug in the People fetch path. |
| [#94](https://github.com/jtenniswood/espframe/issues/94) | Memories >64KB → random fallback | Buffer raised to 192KB in v1.10.7 but `rhatguy` reports 3 remaining bugs | `common/addon/immich_api.yaml` ~line 49 | `json::parse_json(body)` triggers heap OOM on large responses; needs filtered `deserializeJson` with field allow-list, plus two follow-up bugs they detailed in the comments. |
| [#87](https://github.com/jtenniswood/espframe/issues/87) | Adding large album (~848 photos) reboots device | Reproduced by `tam481`, still open | `immich_helpers.h` album fetch + JSON parse | Boot loop with no logs past ROM banner = almost certainly heap exhaustion in the same JSON parse pattern as #94. Likely same fix family. |
| [#103](https://github.com/jtenniswood/espframe/issues/103) | OTA stopped working | No detail, no comments | `common/addon/firmware_update.yaml`, update URL handling | Need to repro on Peter's device, then capture logs. |

### P1 — Bugs likely already fixed, need on-device verification
| # | Issue | Suspected fix | Verify by |
|---|-------|---------------|-----------|
| [#99](https://github.com/jtenniswood/espframe/issues/99) | Screen left on after auto-update | `758878c` (in v1.10.7) | Re-test nightly schedule across one OTA cycle. |

### P2 — Feature requests with clear path
| # | Title | Notes |
|---|-------|-------|
| [#89](https://github.com/jtenniswood/espframe/issues/89) | Support for tags | Already partly landed: commit `7e90eb3` adds Immich tag source. Confirm UI exposure + close upstream. |
| [#85](https://github.com/jtenniswood/espframe/issues/85) | Album / photo order | Needs UX decision (chronological / shuffle / album order). |
| [#68](https://github.com/jtenniswood/espframe/issues/68) | Change display modes from screen | Labelled "ready for development". |
| [#66](https://github.com/jtenniswood/espframe/issues/66) | Portrait rotation support | Labelled "ready for development". |

### P3 — Larger / lower priority
| # | Title | Notes |
|---|-------|-------|
| [#76](https://github.com/jtenniswood/espframe/issues/76) | Wireguard VPN support | Big surface area; defer. |
| [#4](https://github.com/jtenniswood/espframe/issues/4) | Renovate dependency dashboard | Housekeeping; keep enabled on fork or disable to avoid noise. |

## Fork-specific tasks (rebrand so OTA points at us, not upstream)
- [ ] Update `product/espframe.json` slug + OTA / version-check URLs to point at `PeterHing/espframe` release artefacts.
- [ ] Decide whether to keep publishing the web installer at `peterhing.github.io/espframe` or self-host on the home lab (Nginx Proxy Manager + GH Pages mirror both work).
- [ ] Add a `FORK_NOTES.md` retaining PolyForm `Required Notice:` lines and stating this is a personal, non-commercial fork.
- [ ] Decide CI strategy: keep `compile.yml` + `release.yml` running on the fork, or trim down to compile-only until we're cutting releases.

## Suggested first PR (against fork's `main`)
**Fix #100 — People source only newest photo.**
Peter is hit by it on his own frames, smallest blast radius, lives in one file (`immich_api.yaml` / `immich_helpers.h`), and the user-visible win is immediate.
