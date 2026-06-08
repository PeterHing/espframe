  // --- Import / Export ---

  function backupExportFieldValue(entry) {
    if (!entry || !Array.isArray(entry.state_keys) || !entry.state_keys.length) return "";
    if (entry.group === "screen" && entry.field === "schedule_wake_timeout") {
      return normalizeScheduleWakeTimeout(S.schedule_wake_timeout);
    }
    if (entry.state_keys.length > 1) {
      return entry.state_keys.map(function (key) {
        return S[key];
      });
    }
    return S[entry.state_keys[0]];
  }

  function buildBackupExportData() {
    var data = {
      version: 1,
      exported_at: new Date().toISOString()
    };
    BACKUP_SCHEMA.forEach(function (entry) {
      if (!entry || !entry.group || !entry.field) return;
      if (!data[entry.group]) data[entry.group] = {};
      data[entry.group][entry.field] = backupExportFieldValue(entry);
    });
    return data;
  }

  function exportConfig() {
    var data = buildBackupExportData();
    var json = JSON.stringify(data, null, 2);
    var blob = new Blob([json], { type: "application/json" });
    var url = URL.createObjectURL(blob);
    var now = new Date();
    var name = "espframe-config-" +
      now.getFullYear() + "-" +
      String(now.getMonth() + 1).padStart(2, "0") + "-" +
      String(now.getDate()).padStart(2, "0") + ".json";
    var a = document.createElement("a");
    a.href = url;
    a.download = name;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  }

  function backupEntryKey(entry) {
    return entry.group + "." + entry.field;
  }

  function backupImportFieldPresent(data, entry) {
    var groupData = data[entry.group] || {};
    return groupData[entry.field] !== undefined;
  }

  function backupImportFieldValue(data, entry) {
    return (data[entry.group] || {})[entry.field];
  }

  function backupImportStateKey(entry) {
    return entry && Array.isArray(entry.state_keys) && entry.state_keys.length ? entry.state_keys[0] : "";
  }

  function backupImportEntityDomain(stateKey) {
    var parts = productSettingEntityParts(stateKey);
    if (!parts && STATIC_ENTITIES && STATIC_ENTITIES[stateKey]) {
      parts = entityStringParts(STATIC_ENTITIES[stateKey].entity);
    }
    if (!parts && MANUAL_ENTITIES && MANUAL_ENTITIES[stateKey]) {
      parts = entityStringParts(MANUAL_ENTITIES[stateKey].entity);
    }
    return parts && parts.domain ? parts.domain : "";
  }

  function applyGenericBackupImportField(entry, value) {
    var stateKey = backupImportStateKey(entry);
    if (!stateKey || !endpoints[stateKey]) return;
    var domain = backupImportEntityDomain(stateKey);
    S[stateKey] = value;
    if (domain === "switch") {
      post(endpoints[stateKey] + (value ? "/turn_on" : "/turn_off"));
    } else if (domain === "select") {
      post(endpoints[stateKey] + "/set", { option: value });
    } else if (domain === "number" || domain === "text") {
      post(endpoints[stateKey] + "/set", { value: value });
    }
  }

  function applyBackupImportField(entry, value) {
    switch (backupEntryKey(entry)) {
      case "connection.immich_url":
        S.immich_url = normalizeImmichUrl(value);
        postTextValueSet(endpoints.immich_url + "/set", S.immich_url, true);
        return;
      case "connection.api_key":
        S.api_key = value;
        postTextValueSet(endpoints.api_key + "/set", value, true);
        return;
      case "photos.album_ids":
        var importAlbum = String(value).trim();
        if (photoIdFieldTooLong(importAlbum)) {
          showBanner("Album IDs exceed 255 characters - not imported", "error");
        } else if (!isValidUuidList(importAlbum)) {
          showBanner("Import skipped invalid album IDs", "error");
        } else {
          S.album_ids = importAlbum;
          postTextValueSet(endpoints.album_ids + "/set", importAlbum);
        }
        return;
      case "photos.album_labels":
        var importAlbumLabels = String(value).trim();
        if (photoLabelFieldTooLong(importAlbumLabels)) {
          showBanner("Album labels exceed 255 characters - not imported", "error");
        } else {
          S.album_labels = importAlbumLabels;
          postTextValueSet(endpoints.album_labels + "/set", importAlbumLabels);
        }
        return;
      case "photos.person_ids":
        var importPerson = String(value).trim();
        if (photoIdFieldTooLong(importPerson)) {
          showBanner("Person IDs exceed 255 characters - not imported", "error");
        } else if (!isValidUuidList(importPerson)) {
          showBanner("Import skipped invalid person IDs", "error");
        } else {
          S.person_ids = importPerson;
          postTextValueSet(endpoints.person_ids + "/set", importPerson);
        }
        return;
      case "photos.person_labels":
        var importPersonLabels = String(value).trim();
        if (photoLabelFieldTooLong(importPersonLabels)) {
          showBanner("Person labels exceed 255 characters - not imported", "error");
        } else {
          S.person_labels = importPersonLabels;
          postTextValueSet(endpoints.person_labels + "/set", importPersonLabels);
        }
        return;
      case "firmware_updates.manifest_url":
        var importManifestUrl = normalizeFirmwareManifestUrl(value);
        if (importManifestUrl && !isValidHttpUrl(importManifestUrl)) {
          showBanner("Stable firmware URL was invalid - not imported", "error");
        } else {
          S.firmware_manifest_url = importManifestUrl;
          postTextValueSet(endpoints.firmware_manifest_url + "/set", importManifestUrl);
        }
        return;
      case "firmware_updates.beta_manifest_url":
        var importBetaManifestUrl = normalizeFirmwareManifestUrl(value);
        if (importBetaManifestUrl && !isValidHttpUrl(importBetaManifestUrl)) {
          showBanner("Beta firmware URL was invalid - not imported", "error");
        } else {
          S.firmware_beta_manifest_url = importBetaManifestUrl;
          postTextValueSet(endpoints.firmware_beta_manifest_url + "/set", importBetaManifestUrl);
        }
        return;
      case "clock.timezone":
        S.timezone = normalizeTimezoneOption(value);
        post(endpoints.timezone + "/set", { option: S.timezone });
        return;
      case "clock.ntp_servers":
        if (Array.isArray(value)) {
          ["ntp_server_1", "ntp_server_2", "ntp_server_3"].forEach(function (key, idx) {
            if (value[idx] === undefined) return;
            saveNtpServer(key, value[idx]);
          });
        }
        return;
      case "screen.schedule_wake_timeout":
        S.schedule_wake_timeout = normalizeScheduleWakeTimeout(value);
        postScheduleWakeTimeout(S.schedule_wake_timeout);
        return;
      case "screen.rotation":
        var importedRotation = String(value);
        if (screenRotationOptionsForUi().indexOf(importedRotation) !== -1) {
          S.screen_rotation = importedRotation;
          post(endpoints.screen_rotation + "/set", { option: S.screen_rotation });
        }
        return;
      default:
        applyGenericBackupImportField(entry, value);
    }
  }

  function importConfig() {
    var fileInput = document.createElement("input");
    fileInput.type = "file";
    fileInput.accept = ".json";
    fileInput.style.display = "none";

    fileInput.addEventListener("change", function () {
      if (!fileInput.files || !fileInput.files[0]) return;
      var reader = new FileReader();
      reader.onload = function () {
        var data;
        try { data = JSON.parse(reader.result); } catch (_) {
          showBanner("Invalid file \u2014 could not parse JSON", "error");
          return;
        }

        if (!data.version) {
          showBanner("Invalid config file \u2014 missing version", "error");
          return;
        }

        BACKUP_SCHEMA.forEach(function (entry) {
          if (!backupImportFieldPresent(data, entry)) return;
          applyBackupImportField(entry, backupImportFieldValue(data, entry));
        });

        showBanner("Settings imported successfully", "success");
        renderSettings();
      };
      reader.readAsText(fileInput.files[0]);
    });

    document.body.appendChild(fileInput);
    fileInput.click();
    document.body.removeChild(fileInput);
  }
