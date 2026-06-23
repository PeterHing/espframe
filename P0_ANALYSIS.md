# P0 bug analysis — #100, #94, #87

All three live in the Immich fetch path. They share two root causes which can be fixed in one patch family.

## #100 — Person source only shows the newest photo

**Path:** `common/addon/immich_api.yaml` → `immich_fetch_metadata_into_slot` (lines 277-377) → `immich_fetch_metadata_page` (lines 379-487).

**Mechanism the code expected:**
1. Probe request: `POST /api/search/metadata` with `{page:1, size:1, randomize:true, ...}`.
2. Read `total` from response via `parse_immich_metadata_total()`.
3. Pick random page in `[1, ceil(total/page_size)]`.
4. Refetch at that page → random photo.

**Where it actually breaks:** Immich's `/api/search/metadata` does **not** return the total count of matching assets in `assets.total` or `assets.count`. Both fields are populated with **the number of items returned in the current page**. The only signal that more pages exist is `assets.nextPage` (a string page number or `null`).

So with `size:1, page:1`:
- Response: `{assets: {items: [newest_asset], total: 1, count: 1, nextPage: "2"}}`
- `parse_immich_metadata_total(body)` returns **1** (reads `assets.total` = 1, or fallback `items.size()` = 1).
- `immich_metadata_page_for_total(1, page_size)` → `(esp_random() % 1) + 1` = **1**.
- Second request: `{page:1, size:1, randomize:true, ...}`. `randomize:true` would normally shuffle, but Immich's metadata-search randomization seeds per request and with `size=1` you reliably get the highest-date asset (or near it).
- Result: same UUID returned every cycle.

Matches the user logs in [#100](https://github.com/jtenniswood/espframe/issues/100): slot 1 and slot 2 both showing `8bc3870e-... | 5 June, 2026 | PERSON NAME` repeatedly. Affects **Album** and **Tag** sources too (same code path) but is most visible for Person because users typically have many photos of one person across years.

**Why the helper test passes:** `tests/espframe_helper_tests.cpp:188-190` only checks `immich_metadata_page_for_total(848)` — i.e. it tests the math given a correct total. It doesn't exercise the response-parse step. Real-world Immich responses never return a true total, so the math is fed `1` and returns `1`.

## #94 — Memories >64KB

**Already mostly fixed in v1.10.7** (commit `4e5d5c6`, `immich_api.yaml` lines 128-138). The Memories window fetch now uses ArduinoJson 7's `DeserializationOption::Filter` to only allocate heap for `id` and `type` fields, regardless of body size. Buffer raised 64kB → 192kB.

**Outstanding gaps from [rhatguy's comment](https://github.com/jtenniswood/espframe/issues/94):**
1. `immich_fetch_memory_asset` (step 2, line 199-268) still uses `parse_immich_asset` → `esphome::json::parse_json(body)` — a full unfiltered parse. For an asset with extensive EXIF and many people tags, this is the same OOM pattern.
2. Same unfiltered parse appears in:
   - `parse_immich_metadata_asset` (used by Album/Person/Tag)
   - `find_immich_portrait_companion_url` (companion search response)
   - `pick_immich_timeline_bucket` / `pick_immich_timeline_asset_id` (used elsewhere)

## #87 — Adding a large album reboots the device

**Symptom:** [#87 reporter](https://github.com/jtenniswood/espframe/issues/87) has a single album with 848 photos. Adding the album UUID reboots the device within seconds; reflashing doesn't help; manually importing also reboots. Boot log shows only the ESP-ROM banner — no app logs.

**Hypothesis (high confidence):** Same OOM family as #94. The path:
1. User adds album UUID → `flush_slots_and_refetch` runs → `immich_fetch_into_slot` → `immich_fetch_metadata_into_slot`.
2. First fetch is `size:1, withExif:true, withPeople:true`. Response contains one asset with full EXIF + people array.
3. `parse_immich_metadata_total` runs full `esphome::json::parse_json(body)`. With a heap that's already fragmented from a long uptime, ArduinoJson allocates ~3× the body size as `JsonDocument`. For a 10-15KB response that's 30-45KB of contiguous heap; on a fragmented ESP32-P4 internal SRAM that abort-reboots.

ROM-banner-only log = the on_response lambda allocator aborted; the task watchdog rebooted before any app logs could flush.

Album size correlates because: many-photo albums = more varied EXIF/people fan-out across requests = higher chance of hitting an asset whose serialized representation crosses the safe threshold.

## Unified fix plan

### Patch A — Single-step metadata fetch (fixes #100)
Replace the two-step `probe → random page` flow with a single request that asks for a moderate sample (`size=25`) with `randomize:true`, then picks one matching asset client-side.

Why this works:
- Immich's `randomize:true` re-shuffles per request, so a 25-asset random sample over many cycles fairly samples even an 848-photo album.
- One round trip instead of two = faster slideshow and fewer chances for partial-state bugs.
- Removes the broken `parse_immich_metadata_total` dependency entirely.

**Files:** `common/addon/immich_api.yaml`, `components/espframe/immich_helpers.h`.

### Patch B — Filtered JSON parsing for all asset responses (fixes #87, completes #94)
Replace every `esphome::json::parse_json(body)` followed by field reads with `deserializeJson(doc, body, Filter(...))` using an allow-list filter:

```cpp
JsonDocument filter;
filter[0]["id"] = true;
filter[0]["localDateTime"] = true;
filter[0]["exifInfo"]["city"] = true;
filter[0]["exifInfo"]["country"] = true;
filter[0]["exifInfo"]["dateTimeOriginal"] = true;
filter[0]["exifInfo"]["exifImageWidth"] = true;
filter[0]["exifInfo"]["exifImageHeight"] = true;
filter[0]["exifInfo"]["orientation"] = true;
filter[0]["people"][0]["name"] = true;
```

This keeps the parsed document under ~3KB per asset regardless of how much EXIF / people metadata is in the response. Eliminates OOM at the source.

**Files:** `components/espframe/immich_helpers.h` (parse functions), `common/addon/immich_api.yaml` (any inline lambdas).

### Patch C — Tests
- Helper-level test for new `pick_immich_random_metadata_asset_*` selecting different items across many calls when given the same buffer.
- Helper-level test confirming orientation filter is honored on the random pick.
- Web smoke test confirming the YAML still validates with the new script structure.

### Branch & PR
- Branch: `fix/p0-immich-metadata` off `main` on the fork.
- Single PR titled "Fix Person/Album/Tag stuck on newest and harden large-response parsing", referencing #100, #94, #87.
- Don't merge to main until the fix is verified on Peter's actual device.
