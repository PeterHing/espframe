#pragma once
#include "date_utils.h"
#include "esp_random.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

static constexpr uint16_t ZOOM_IDENTITY = 256;
static constexpr uint16_t IMMICH_ALBUM_PAGE_SIZE = 16;
static constexpr uint16_t IMMICH_METADATA_PAGE_SIZE = 5;
// Sample size for the single-shot random-pick metadata search.
// Sized so the request body fits in 64 kB of HTTP response buffer for typical
// assets (~25 * ~1.5 kB after Filter-based deserialization).
static constexpr uint16_t IMMICH_METADATA_RANDOM_SAMPLE_SIZE = 25;

struct ImmichAssetMeta {
  // Normalized subset of the Immich asset response used by the slideshow UI.
  // Keeping a compact struct avoids spreading JSON field names through YAML
  // lambdas.
  std::string asset_id, image_url, date, location, person;
  std::string datetime;  // localDateTime from asset, for slot display
  int year = 0, month = 0, day = 0;
  bool is_portrait = false;
  bool orientation_known = false;
  uint16_t zoom = ZOOM_IDENTITY;
};

// ============================================================================
// Immich search body builder
// ============================================================================
// Builds the JSON POST body for /api/search/random with optional filters
// for favorites, albums, people, and tags. The `extra` parameter allows injecting
// additional JSON fields (e.g. takenAfter/takenBefore for companion search).

struct ImmichDateRange {
  std::string from;
  std::string to;
  bool relative_skipped_for_invalid_time = false;
};

struct ImmichTimelineBucketChoice {
  std::string time_bucket;
  uint32_t count = 0;
  uint32_t page = 1;
};

struct ImmichTimelineBucketInfo {
  std::string time_bucket;
  uint32_t count = 0;
};

struct ImmichTimelineAssetCandidate {
  std::string asset_id;
  bool is_image = true;
  bool has_ratio = false;
  float ratio = 0.0f;
};

inline int immich_days_in_month(int year, int month) {
  static const int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (month == 2) {
    bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    return leap ? 29 : 28;
  }
  if (month < 1 || month > 12) return 31;
  return days[month - 1];
}

inline std::string immich_format_iso_date(int year, int month, int day) {
  char buf[11];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d", year, month, day);
  return std::string(buf);
}

inline std::string immich_format_iso_date_offset(int year, int month, int day, int offset_days) {
  int shifted_year = 0;
  int shifted_month = 0;
  int shifted_day = 0;
  civil_from_days(days_from_civil(year, month, day) + offset_days,
                  shifted_year, shifted_month, shifted_day);
  return immich_format_iso_date(shifted_year, shifted_month, shifted_day);
}

inline void append_csv_value(std::string &csv, const std::string &value) {
  if (value.empty()) return;
  if (!csv.empty()) csv += ",";
  csv += value;
}

inline std::string csv_value_at(const std::string &csv, int index) {
  if (index < 0) return "";
  size_t start = 0;
  for (int i = 0; i < index; i++) {
    start = csv.find(',', start);
    if (start == std::string::npos) return "";
    start++;
  }
  size_t end = csv.find(',', start);
  if (end == std::string::npos) end = csv.size();
  return csv.substr(start, end - start);
}

inline ImmichDateRange resolve_immich_date_filter(bool enabled,
                                                  const std::string &mode,
                                                  int amount,
                                                  const std::string &unit,
                                                  bool now_valid,
                                                  int now_year,
                                                  int now_month,
                                                  int now_day,
                                                  const std::string &fixed_from,
                                                  const std::string &fixed_to) {
  ImmichDateRange range;
  if (!enabled) return range;

  if (mode == "Relative Range") {
    if (!now_valid) {
      range.relative_skipped_for_invalid_time = true;
      return range;
    }
    if (amount < 1) amount = 1;
    int year = now_year;
    int month = now_month;
    int day = now_day;
    if (unit == "Years") {
      year -= amount;
    } else {
      int total_months = year * 12 + (month - 1) - amount;
      year = total_months / 12;
      month = (total_months % 12) + 1;
    }
    int max_day = immich_days_in_month(year, month);
    if (day > max_day) day = max_day;
    range.from = immich_format_iso_date(year, month, day);
    range.to = immich_format_iso_date(now_year, now_month, now_day);
    return range;
  }

  range.from = fixed_from;
  range.to = fixed_to;
  return range;
}

inline void append_immich_taken_range(std::string &extra,
                                      const std::string &from,
                                      const std::string &to) {
  if (!from.empty()) {
    extra += "\"takenAfter\":\"" + from + "T00:00:00.000Z\"";
  }
  if (!to.empty()) {
    if (!extra.empty()) extra += ",";
    extra += "\"takenBefore\":\"" + to + "T23:59:59.999Z\"";
  }
}

inline std::string build_immich_date_filter_extra(const ImmichDateRange &range) {
  std::string extra;
  append_immich_taken_range(extra, range.from, range.to);
  return extra;
}

inline std::string build_immich_companion_date_filter_extra(const std::string &day,
                                                           const ImmichDateRange &range) {
  std::string after = day + "T00:00:00.000Z";
  std::string before = day + "T23:59:59.999Z";
  if (!range.from.empty() && range.from > day) after = range.from + "T00:00:00.000Z";
  if (!range.to.empty() && range.to < day) before = range.to + "T23:59:59.999Z";
  return "\"takenAfter\":\"" + after + "\",\"takenBefore\":\"" + before + "\"";
}

inline std::vector<std::string> split_uuid_csv(const std::string &csv) {
  // Home Assistant text fields store source IDs as comma-separated text;
  // normalize that into individual UUID strings before building API requests.
  std::vector<std::string> out;
  size_t start = 0;
  while (start < csv.size()) {
    size_t end = csv.find(',', start);
    if (end == std::string::npos)
      end = csv.size();
    size_t s = start, e = end;
    while (s < e && csv[s] == ' ')
      s++;
    while (e > s && csv[e - 1] == ' ')
      e--;
    if (s < e)
      out.emplace_back(csv.substr(s, e - s));
    start = end + 1;
  }
  return out;
}

// Immich treats multiple personIds as AND (asset must include every person).
// For Person source we send one UUID per request so results are any-of over time.
inline std::string pick_one_person_id_for_random_search(const std::string &csv) {
  std::vector<std::string> ids = split_uuid_csv(csv);
  if (ids.empty())
    return "";
  if (ids.size() == 1)
    return ids[0];
  return ids[esp_random() % ids.size()];
}

inline std::string pick_one_uuid_from_csv(const std::string &csv) {
  std::vector<std::string> ids = split_uuid_csv(csv);
  if (ids.empty())
    return "";
  if (ids.size() == 1)
    return ids[0];
  return ids[esp_random() % ids.size()];
}

inline std::string build_uuid_json_array(const std::string &csv) {
  std::vector<std::string> ids = split_uuid_csv(csv);
  std::string result = "[";
  for (size_t i = 0; i < ids.size(); i++) {
    if (i)
      result += ",";
    result += "\"" + ids[i] + "\"";
  }
  result += "]";
  return result;
}

inline std::string build_immich_search_body(int size, bool with_people,
                                             const std::string &photo_source,
                                             const std::string &album_ids,
                                             const std::string &person_ids,
                                             const std::string &tag_ids,
                                             const std::string &extra = "") {
  // Construct the small JSON request body by hand to keep this header usable
  // from ESPHome lambdas without bringing in another JSON writer.
  std::string body = "{\"size\":" + std::to_string(size) +
                      ",\"type\":\"IMAGE\",\"withExif\":true";
  if (with_people) body += ",\"withPeople\":true";
  if (!extra.empty()) body += "," + extra;
  if (photo_source == "Favorites") {
    body += ",\"isFavorite\":true";
  } else if (photo_source == "Album" && !album_ids.empty()) {
    body += ",\"albumIds\":" + build_uuid_json_array(album_ids);
  } else if (photo_source == "Person" && !person_ids.empty()) {
    std::string one = pick_one_person_id_for_random_search(person_ids);
    if (!one.empty())
      body += ",\"personIds\":" + build_uuid_json_array(one);
  } else if (photo_source == "Tag" && !tag_ids.empty()) {
    body += ",\"tagIds\":" + build_uuid_json_array(tag_ids);
  }
  body += "}";
  return body;
}

inline uint32_t immich_metadata_page_for_total(uint32_t total,
                                               uint16_t page_size = IMMICH_METADATA_PAGE_SIZE) {
  if (page_size == 0) page_size = IMMICH_METADATA_PAGE_SIZE;
  if (total == 0) return 1;
  uint32_t pages = (total + page_size - 1) / page_size;
  if (pages == 0) pages = 1;
  return (esp_random() % pages) + 1;
}

inline bool immich_source_uses_metadata_search(const std::string &photo_source) {
  return photo_source == "Album" || photo_source == "Person" || photo_source == "Tag";
}

inline std::string build_immich_metadata_search_body(uint32_t page,
                                                     uint16_t size,
                                                     bool with_people,
                                                     const std::string &photo_source,
                                                     const std::string &album_id,
                                                     const std::string &person_id,
                                                     const std::string &tag_ids,
                                                     const std::string &extra = "") {
  if (page == 0) page = 1;
  if (size == 0) size = 1;
  std::string body = "{\"page\":" + std::to_string(page) +
                     ",\"size\":" + std::to_string(size) +
                     ",\"type\":\"IMAGE\",\"visibility\":\"timeline\",\"withExif\":true";
  if (with_people) body += ",\"withPeople\":true";
  if (!extra.empty()) body += "," + extra;
  if (photo_source == "Favorites") {
    body += ",\"isFavorite\":true";
  } else if (photo_source == "Album" && !album_id.empty()) {
    body += ",\"albumIds\":[\"" + album_id + "\"]";
  } else if (photo_source == "Person" && !person_id.empty()) {
    body += ",\"personIds\":[\"" + person_id + "\"]";
  } else if (photo_source == "Tag" && !tag_ids.empty()) {
    body += ",\"tagIds\":" + build_uuid_json_array(tag_ids);
  }
  body += "}";
  return body;
}

inline bool photo_orientation_matches(const ImmichAssetMeta &meta, const std::string &filter) {
  if (filter == "Any" || filter.empty()) return true;
  if (!meta.orientation_known) return false;
  if (filter == "Portrait Only") return meta.is_portrait;
  if (filter == "Landscape Only") return !meta.is_portrait;
  return true;
}

inline uint32_t immich_album_page_for_count(uint32_t count,
                                            uint16_t page_size = IMMICH_ALBUM_PAGE_SIZE) {
  if (page_size == 0) page_size = IMMICH_ALBUM_PAGE_SIZE;
  if (count == 0) count = 1;
  uint32_t pages = (count + page_size - 1) / page_size;
  if (pages == 0) pages = 1;
  return (esp_random() % pages) + 1;
}

inline ImmichTimelineBucketChoice pick_immich_timeline_bucket_from_choices(
    const std::vector<ImmichTimelineBucketInfo> &buckets,
    uint16_t page_size = IMMICH_ALBUM_PAGE_SIZE) {
  std::vector<ImmichTimelineBucketInfo> choices;
  uint32_t total = 0;

  for (const auto &bucket : buckets) {
    if (bucket.time_bucket.empty()) continue;
    uint32_t count = bucket.count == 0 ? 1 : bucket.count;
    choices.push_back({bucket.time_bucket, count});
    total += count;
  }

  if (choices.empty() || total == 0) return {};

  uint32_t pick = esp_random() % total;
  uint32_t seen = 0;
  for (const auto &choice : choices) {
    seen += choice.count;
    if (pick < seen) {
      return {choice.time_bucket, choice.count,
              immich_album_page_for_count(choice.count, page_size)};
    }
  }

  const auto &choice = choices.back();
  return {choice.time_bucket, choice.count,
          immich_album_page_for_count(choice.count, page_size)};
}

inline std::string pick_immich_timeline_asset_id_from_candidates(
    const std::vector<ImmichTimelineAssetCandidate> &assets,
    const std::string &orientation_filter = "Any") {
  std::vector<std::string> candidates;

  for (const auto &asset : assets) {
    if (asset.asset_id.empty() || !asset.is_image) continue;

    if (orientation_filter == "Portrait Only" || orientation_filter == "Landscape Only") {
      if (!asset.has_ratio || asset.ratio <= 0.0f) continue;
      bool portrait = asset.ratio < 1.0f;
      if (orientation_filter == "Portrait Only" && !portrait) continue;
      if (orientation_filter == "Landscape Only" && portrait) continue;
    }

    candidates.push_back(asset.asset_id);
  }

  if (candidates.empty()) return "";
  return candidates[esp_random() % candidates.size()];
}

// ============================================================================
// Immich asset parser — parse JSON asset and fill meta
// ============================================================================
// body: JSON string (single asset object or array with one object).
// base_url: Immich server base URL (no trailing slash).
// out_meta: filled with asset_id, image_url, date, location, person, year,
//           month, day, is_portrait, zoom. Returns the image URL on success,
//           empty string on parse failure.

#ifdef USE_JSON
#include "esphome/components/json/json_util.h"

inline std::string parse_immich_asset_object(JsonObject asset,
                                             const std::string &base_url,
                                             ImmichAssetMeta *out_meta) {
  if (out_meta == nullptr) return "";
  if (asset.isNull() || !asset["id"].is<const char *>())
    return "";

  std::string asset_id = asset["id"].as<std::string>();
  std::string photo_date, photo_location, photo_person;
  int photo_year = 0, photo_month = 0, photo_day = 0;
  bool is_portrait = false;
  bool orientation_known = false;
  auto read_date = [](const std::string &raw, int &year, int &month, int &day) {
    if (raw.size() < 10) return false;
    year = atoi(raw.substr(0, 4).c_str());
    month = atoi(raw.substr(5, 2).c_str());
    day = atoi(raw.substr(8, 2).c_str());
    return year > 0 && month >= 1 && month <= 12 && day >= 1 && day <= 31;
  };

  std::string local_datetime;
  if (asset["localDateTime"].is<const char *>()) {
    std::string raw = asset["localDateTime"].as<std::string>();
    local_datetime = raw;
    if (read_date(raw, photo_year, photo_month, photo_day))
      photo_date = format_photo_date_full(photo_year, photo_month, photo_day);
  }

  JsonObject exif = asset["exifInfo"].as<JsonObject>();
  if (!exif.isNull()) {
    // Prefer location and date from EXIF when available; Immich's localDateTime
    // remains the first choice because it already reflects the library's time
    // zone handling.
    std::string city, country;
    if (exif["city"].is<const char *>()) city = exif["city"].as<std::string>();
    if (exif["country"].is<const char *>()) country = exif["country"].as<std::string>();
    if (!city.empty() && !country.empty()) photo_location = city + ", " + country;
    else if (!city.empty()) photo_location = city;
    else if (!country.empty()) photo_location = country;

    if (photo_date.empty() && exif["dateTimeOriginal"].is<const char *>()) {
      std::string raw = exif["dateTimeOriginal"].as<std::string>();
      if (read_date(raw, photo_year, photo_month, photo_day))
        photo_date = format_photo_date_full(photo_year, photo_month, photo_day);
    }

    int exif_w = 0, exif_h = 0;
    if (exif["exifImageWidth"].is<int>()) exif_w = exif["exifImageWidth"].as<int>();
    if (exif["exifImageHeight"].is<int>()) exif_h = exif["exifImageHeight"].as<int>();
    std::string orientation;
    if (exif["orientation"].is<const char *>()) orientation = exif["orientation"].as<std::string>();
    // EXIF orientations 5-8 mean the stored dimensions are rotated relative to
    // the displayed photo, so swap before deciding whether it is portrait.
    if (orientation == "5" || orientation == "6" || orientation == "7" || orientation == "8")
      std::swap(exif_w, exif_h);
    if (exif_w > 0 && exif_h > 0) {
      is_portrait = (exif_h > exif_w);
      orientation_known = true;
    }
  }

  if (asset["people"].is<JsonArray>()) {
    JsonArray people = asset["people"].as<JsonArray>();
    if (people.size() > 0) {
      JsonObject person = people[0].as<JsonObject>();
      if (person["name"].is<const char *>())
        photo_person = person["name"].as<std::string>();
    }
  }

  std::string img_url = base_url + "/api/assets/" + asset_id + "/thumbnail?size=preview";
  out_meta->asset_id = asset_id;
  out_meta->image_url = img_url;
  out_meta->date = photo_date;
  out_meta->location = photo_location;
  out_meta->year = photo_year;
  out_meta->month = photo_month;
  out_meta->day = photo_day;
  out_meta->person = photo_person;
  out_meta->datetime = local_datetime;
  out_meta->is_portrait = is_portrait;
  out_meta->orientation_known = orientation_known;
  out_meta->zoom = ZOOM_IDENTITY;
  return img_url;
}

// ArduinoJson Filter that limits parsed heap to the fields the slot UI actually
// reads. Used for both `/api/search/random` (top-level array) and single-asset
// detail (top-level object) responses. Without this, parsing a single asset
// with extensive EXIF + people metadata can balloon to 30-50 kB of heap and
// abort on a fragmented ESP32-P4 SRAM (root cause of #87 and the remaining
// symptoms in #94).
inline JsonDocument immich_asset_field_filter() {
  JsonDocument filter;
  // Allow either array-of-assets or single-asset response shapes.
  filter[0]["id"] = true;
  filter[0]["localDateTime"] = true;
  filter[0]["exifInfo"]["city"] = true;
  filter[0]["exifInfo"]["country"] = true;
  filter[0]["exifInfo"]["dateTimeOriginal"] = true;
  filter[0]["exifInfo"]["exifImageWidth"] = true;
  filter[0]["exifInfo"]["exifImageHeight"] = true;
  filter[0]["exifInfo"]["orientation"] = true;
  filter[0]["people"][0]["name"] = true;
  filter["id"] = true;
  filter["localDateTime"] = true;
  filter["exifInfo"]["city"] = true;
  filter["exifInfo"]["country"] = true;
  filter["exifInfo"]["dateTimeOriginal"] = true;
  filter["exifInfo"]["exifImageWidth"] = true;
  filter["exifInfo"]["exifImageHeight"] = true;
  filter["exifInfo"]["orientation"] = true;
  filter["people"][0]["name"] = true;
  return filter;
}

inline std::string parse_immich_asset(const std::string &body,
                                      const std::string &base_url,
                                      ImmichAssetMeta *out_meta,
                                      const std::string &orientation_filter = "Any") {
  if (out_meta == nullptr) return "";

  JsonDocument filter = immich_asset_field_filter();
  JsonDocument doc;
  DeserializationError err = deserializeJson(
      doc, body.c_str(), DeserializationOption::Filter(filter));
  if (err) return "";

  if (doc.is<JsonArray>()) {
    JsonArray arr = doc.as<JsonArray>();
    for (size_t i = 0; i < arr.size(); i++) {
      ImmichAssetMeta candidate;
      std::string img_url = parse_immich_asset_object(arr[i].as<JsonObject>(), base_url, &candidate);
      if (img_url.empty()) continue;
      if (!photo_orientation_matches(candidate, orientation_filter)) continue;
      *out_meta = candidate;
      return img_url;
    }
    return "";
  }

  if (doc.is<JsonObject>()) {
    ImmichAssetMeta candidate;
    std::string img_url = parse_immich_asset_object(doc.as<JsonObject>(), base_url, &candidate);
    if (img_url.empty() || !photo_orientation_matches(candidate, orientation_filter)) return "";
    *out_meta = candidate;
    return img_url;
  }

  return "";
}

// DEPRECATED: Immich's /api/search/metadata response does not expose the true
// total of matching assets in `assets.total` or `assets.count`. Both fields are
// populated with the items count of the current page, so this helper always
// returned 1 for size=1 probes. Kept in-tree only for backward compatibility
// with the old two-step probe in immich_api.yaml; new code uses the single
// random-sample fetch via pick_random_immich_metadata_asset. See #100.
inline uint32_t parse_immich_metadata_total(const std::string &body) {
  auto doc = esphome::json::parse_json(body);
  if (doc.isNull() || !doc.is<JsonObject>()) return 0;

  JsonObject root = doc.as<JsonObject>();
  JsonObject assets = root["assets"].as<JsonObject>();
  if (assets.isNull()) return 0;

  int total = 0;
  if (assets["total"].is<int>()) total = assets["total"].as<int>();
  if (total <= 0 && assets["count"].is<int>()) total = assets["count"].as<int>();
  if (total <= 0 && assets["items"].is<JsonArray>()) {
    total = assets["items"].as<JsonArray>().size();
  }
  return total > 0 ? static_cast<uint32_t>(total) : 0;
}

// Filter for /api/search/metadata responses. Mirrors immich_asset_field_filter
// but rooted inside `assets.items[*]` since that response wraps assets in an
// envelope.
inline JsonDocument immich_metadata_asset_field_filter() {
  JsonDocument filter;
  filter["assets"]["items"][0]["id"] = true;
  filter["assets"]["items"][0]["localDateTime"] = true;
  filter["assets"]["items"][0]["exifInfo"]["city"] = true;
  filter["assets"]["items"][0]["exifInfo"]["country"] = true;
  filter["assets"]["items"][0]["exifInfo"]["dateTimeOriginal"] = true;
  filter["assets"]["items"][0]["exifInfo"]["exifImageWidth"] = true;
  filter["assets"]["items"][0]["exifInfo"]["exifImageHeight"] = true;
  filter["assets"]["items"][0]["exifInfo"]["orientation"] = true;
  filter["assets"]["items"][0]["people"][0]["name"] = true;
  return filter;
}

// Pick one asset at random from a `/api/search/metadata` response, honoring the
// orientation filter. Returns the image URL on success, empty string on no
// match. Uses ArduinoJson's Filter option so heap stays bounded regardless of
// response size — required to safely run with size=IMMICH_METADATA_RANDOM_SAMPLE_SIZE.
// Fixes #100 (only-newest), #87 (large-album reboot), and the asset-detail leg of #94.
inline std::string pick_random_immich_metadata_asset(
    const std::string &body, const std::string &base_url,
    ImmichAssetMeta *out_meta, const std::string &orientation_filter = "Any") {
  if (out_meta == nullptr) return "";

  JsonDocument filter = immich_metadata_asset_field_filter();
  JsonDocument doc;
  DeserializationError err = deserializeJson(
      doc, body.c_str(), DeserializationOption::Filter(filter));
  if (err || !doc.is<JsonObject>()) return "";

  JsonObject assets = doc["assets"].as<JsonObject>();
  if (assets.isNull() || !assets["items"].is<JsonArray>()) return "";
  JsonArray items = assets["items"].as<JsonArray>();
  if (items.size() == 0) return "";

  // Two-pass: first index every item that satisfies the orientation filter,
  // then pick one of those at random. This is uniform over matches regardless
  // of where they appear in the (already-randomized) server response.
  std::vector<uint16_t> matches;
  matches.reserve(items.size());
  for (size_t i = 0; i < items.size(); i++) {
    ImmichAssetMeta probe;
    std::string img_url = parse_immich_asset_object(items[i].as<JsonObject>(), base_url, &probe);
    if (img_url.empty()) continue;
    if (!photo_orientation_matches(probe, orientation_filter)) continue;
    matches.push_back(static_cast<uint16_t>(i));
  }
  if (matches.empty()) return "";

  uint16_t chosen_index = matches[esp_random() % matches.size()];
  ImmichAssetMeta chosen;
  std::string img_url = parse_immich_asset_object(
      items[chosen_index].as<JsonObject>(), base_url, &chosen);
  if (img_url.empty()) return "";
  *out_meta = chosen;
  return img_url;
}

// Routes to pick_random_immich_metadata_asset — the legacy YAML still calls
// this name but it now performs a single random-pick parse. Bridging here
// keeps the YAML diff small. See #100.
inline std::string parse_immich_metadata_asset(const std::string &body,
                                               const std::string &base_url,
                                               ImmichAssetMeta *out_meta,
                                               const std::string &orientation_filter = "Any") {
  return pick_random_immich_metadata_asset(body, base_url, out_meta, orientation_filter);
}

inline ImmichTimelineBucketChoice pick_immich_timeline_bucket(
    const std::string &body,
    uint16_t page_size = IMMICH_ALBUM_PAGE_SIZE) {
  auto doc = esphome::json::parse_json(body);
  if (doc.isNull() || !doc.is<JsonArray>()) return {};

  JsonArray buckets = doc.as<JsonArray>();
  std::vector<ImmichTimelineBucketInfo> choices;

  for (size_t i = 0; i < buckets.size(); i++) {
    JsonObject bucket = buckets[i].as<JsonObject>();
    if (bucket.isNull() || !bucket["timeBucket"].is<const char *>()) continue;
    int count = bucket["count"].is<int>() ? bucket["count"].as<int>() : 1;
    if (count <= 0) count = 1;
    choices.push_back({bucket["timeBucket"].as<std::string>(),
                       static_cast<uint32_t>(count)});
  }

  return pick_immich_timeline_bucket_from_choices(choices, page_size);
}

inline std::string pick_immich_time_bucket(const std::string &body) {
  return pick_immich_timeline_bucket(body).time_bucket;
}

inline std::string pick_immich_timeline_asset_id(const std::string &body,
                                                 const std::string &orientation_filter = "Any") {
  auto doc = esphome::json::parse_json(body);
  if (doc.isNull() || !doc.is<JsonObject>()) return "";

  JsonObject bucket = doc.as<JsonObject>();
  JsonArray ids = bucket["id"].as<JsonArray>();
  if (ids.isNull() || ids.size() == 0) return "";

  JsonArray is_images = bucket["isImage"].as<JsonArray>();
  JsonArray ratios = bucket["ratio"].as<JsonArray>();
  std::vector<ImmichTimelineAssetCandidate> candidates;

  for (size_t i = 0; i < ids.size(); i++) {
    if (!ids[i].is<const char *>()) continue;

    ImmichTimelineAssetCandidate candidate;
    candidate.asset_id = ids[i].as<std::string>();
    if (!is_images.isNull() && is_images[i].is<bool>()) {
      candidate.is_image = is_images[i].as<bool>();
    }
    if (!ratios.isNull() &&
        (ratios[i].is<float>() || ratios[i].is<double>() || ratios[i].is<int>())) {
      candidate.has_ratio = true;
      candidate.ratio = ratios[i].as<float>();
    }
    candidates.push_back(candidate);
  }

  return pick_immich_timeline_asset_id_from_candidates(candidates, orientation_filter);
}

inline std::string find_immich_portrait_companion_url(const std::string &body,
                                                      const std::string &base_url,
                                                      const std::string &primary_asset_id) {
  // Filtered parse — companion search can return up to 5 candidates with full
  // EXIF+people; keep heap bounded. See #94/#87.
  JsonDocument filter;
  filter[0]["id"] = true;
  filter[0]["exifInfo"]["exifImageWidth"] = true;
  filter[0]["exifInfo"]["exifImageHeight"] = true;
  filter[0]["exifInfo"]["orientation"] = true;

  JsonDocument doc;
  DeserializationError err = deserializeJson(
      doc, body.c_str(), DeserializationOption::Filter(filter));
  if (err || !doc.is<JsonArray>()) return "";

  JsonArray arr = doc.as<JsonArray>();
  for (size_t i = 0; i < arr.size(); i++) {
    JsonObject asset = arr[i].as<JsonObject>();
    if (asset.isNull() || !asset["id"].is<const char *>()) continue;

    std::string asset_id = asset["id"].as<std::string>();
    if (asset_id == primary_asset_id) continue;

    JsonObject exif = asset["exifInfo"].as<JsonObject>();
    if (exif.isNull()) continue;

    int width = 0;
    int height = 0;
    if (exif["exifImageWidth"].is<int>()) width = exif["exifImageWidth"].as<int>();
    if (exif["exifImageHeight"].is<int>()) height = exif["exifImageHeight"].as<int>();

    std::string orientation;
    if (exif["orientation"].is<const char *>()) orientation = exif["orientation"].as<std::string>();
    if (orientation == "5" || orientation == "6" || orientation == "7" || orientation == "8")
      std::swap(width, height);

    if (width <= 0 || height <= 0 || height <= width) continue;
    return base_url + "/api/assets/" + asset_id + "/thumbnail?size=preview";
  }

  return "";
}

#endif  // USE_JSON
