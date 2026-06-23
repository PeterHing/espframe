  function makeScreenBrightnessCard() {
    // Screen Brightness
    var dnDetails = el("div");

    var fDayBrt = field("Daytime Brightness");
    var rwDay = el("div", "range-wrap");
    var daySlider = document.createElement("input");
    daySlider.type = "range";
    daySlider.min = productNumberMin("brightness_day", 10);
    daySlider.max = productNumberMax("brightness_day", 100);
    daySlider.step = productNumberStep("brightness_day", 5);
    daySlider.value = S.brightness_day;
    var dayVal = el("span", "range-val");
    dayVal.textContent = Math.round(S.brightness_day) + "%";
    daySlider.oninput = function () {
      dayVal.textContent = daySlider.value + "%";
    };
    daySlider.onchange = function () {
      saveSetting("brightness_day", daySlider.value);
    };
    rwDay.appendChild(daySlider);
    rwDay.appendChild(dayVal);
    fDayBrt.appendChild(rwDay);
    dnDetails.appendChild(fDayBrt);

    var fNightBrt = field("Nighttime Brightness");
    var rwNight = el("div", "range-wrap");
    var nightSlider = document.createElement("input");
    nightSlider.type = "range";
    nightSlider.min = productNumberMin("brightness_night", 10);
    nightSlider.max = productNumberMax("brightness_night", 100);
    nightSlider.step = productNumberStep("brightness_night", 5);
    nightSlider.value = S.brightness_night;
    var nightVal = el("span", "range-val");
    nightVal.textContent = Math.round(S.brightness_night) + "%";
    nightSlider.oninput = function () {
      nightVal.textContent = nightSlider.value + "%";
    };
    nightSlider.onchange = function () {
      saveSetting("brightness_night", nightSlider.value);
    };
    rwNight.appendChild(nightSlider);
    rwNight.appendChild(nightVal);
    fNightBrt.appendChild(rwNight);
    dnDetails.appendChild(fNightBrt);

    var fSunInfo = el("div", "field sun-info");
    fSunInfo.id = "sun-info";
    function updateSunInfo() {
      updateSunInfoElement(fSunInfo);
    }
    updateSunInfo();
    dnDetails.appendChild(fSunInfo);

    return makeCollapsibleCard("Screen Brightness", dnDetails, true);

  }

  function makeScreenToneCard() {
    // Screen Tone
    var toneBadge = makeBadge(S.base_tone_enabled || S.warm_tones_enabled);
    var warmBody = el("div");

    var fBaseToneToggle = field("");
    var baseTr = el("div", "toggle-row");
    baseTr.innerHTML = "<span>Screen Tone Adjustment</span>";
    var baseTog = el("div", S.base_tone_enabled ? "toggle on" : "toggle");
    var baseDetails = el("div");
    baseDetails.style.display = S.base_tone_enabled ? "" : "none";

    baseTog.onclick = function () {
      S.base_tone_enabled = !S.base_tone_enabled;
      baseTog.className = S.base_tone_enabled ? "toggle on" : "toggle";
      baseDetails.style.display = S.base_tone_enabled ? "" : "none";
      toneBadge.className = "on-badge" + ((S.base_tone_enabled || S.warm_tones_enabled) ? " active" : "");
      saveSetting("base_tone_enabled", S.base_tone_enabled);
    };
    baseTr.appendChild(baseTog);
    fBaseToneToggle.appendChild(baseTr);
    fBaseToneToggle.style.marginBottom = "8px";
    warmBody.appendChild(fBaseToneToggle);

    var fBaseTone = field("");
    var rwBase = el("div", "range-wrap");
    var baseLabelL = el("span", "range-label");
    baseLabelL.textContent = "Cooler";
    var baseSlider = document.createElement("input");
    baseSlider.type = "range";
    baseSlider.min = productNumberMin("base_tone", 0);
    baseSlider.max = productNumberMax("base_tone", 100);
    baseSlider.step = productNumberStep("base_tone", 5);
    baseSlider.value = S.base_tone;
    baseSlider.onchange = function () {
      saveSetting("base_tone", baseSlider.value);
    };
    var baseLabelR = el("span", "range-label");
    baseLabelR.textContent = "Warmer";
    rwBase.appendChild(baseLabelL);
    rwBase.appendChild(baseSlider);
    rwBase.appendChild(baseLabelR);
    fBaseTone.appendChild(rwBase);
    baseDetails.appendChild(fBaseTone);
    baseDetails.style.marginBottom = "28px";
    warmBody.appendChild(baseDetails);

    var fWarmToggle = field("");
    var warmTr = el("div", "toggle-row");
    warmTr.innerHTML = "<span>Night Tone Adjustment</span>";
    var warmTog = el("div", S.warm_tones_enabled ? "toggle on" : "toggle");
    var nightDetails = el("div");
    nightDetails.style.display = S.warm_tones_enabled ? "" : "none";

    warmTog.onclick = function () {
      S.warm_tones_enabled = !S.warm_tones_enabled;
      warmTog.className = S.warm_tones_enabled ? "toggle on" : "toggle";
      nightDetails.style.display = S.warm_tones_enabled ? "" : "none";
      toneBadge.className = "on-badge" + ((S.base_tone_enabled || S.warm_tones_enabled) ? " active" : "");
      saveSetting("warm_tones_enabled", S.warm_tones_enabled);
    };
    warmTr.appendChild(warmTog);
    fWarmToggle.appendChild(warmTr);
    fWarmToggle.style.marginBottom = "8px";
    warmBody.appendChild(fWarmToggle);

    var fWarmInt = field("");
    var rwWarm = el("div", "range-wrap");
    var warmLabelL = el("span", "range-label");
    warmLabelL.textContent = "Cooler";
    var warmSlider = document.createElement("input");
    warmSlider.type = "range";
    warmSlider.min = productNumberMin("warm_tone_intensity", 10);
    warmSlider.max = productNumberMax("warm_tone_intensity", 100);
    warmSlider.step = productNumberStep("warm_tone_intensity", 5);
    warmSlider.value = S.warm_tone_intensity;
    warmSlider.onchange = function () {
      saveSetting("warm_tone_intensity", warmSlider.value);
    };
    var warmLabelR = el("span", "range-label");
    warmLabelR.textContent = "Warmer";
    rwWarm.appendChild(warmLabelL);
    rwWarm.appendChild(warmSlider);
    rwWarm.appendChild(warmLabelR);
    fWarmInt.appendChild(rwWarm);
    nightDetails.appendChild(fWarmInt);

    var fOverride = field("");
    var overTr = el("div", "toggle-row");
    overTr.innerHTML = "<span>Turn on until sunrise</span>";
    var overTog = el("div", S.warm_tone_override ? "toggle on" : "toggle");
    overTog.onclick = function () {
      S.warm_tone_override = !S.warm_tone_override;
      overTog.className = S.warm_tone_override ? "toggle on" : "toggle";
      saveSetting("warm_tone_override", S.warm_tone_override);
    };
    overTr.appendChild(overTog);
    fOverride.appendChild(overTr);
    nightDetails.appendChild(fOverride);

    warmBody.appendChild(nightDetails);
    return makeCollapsibleCard("Screen Tone", warmBody, true, toneBadge);

  }

  function makeNightScheduleCard() {
    // Schedule
    var schedBadge = makeBadge(S.schedule_enabled);
    var schedBody = el("div");
    var fSchedToggle = field("");
    var schedTr = el("div", "toggle-row");
    schedTr.innerHTML = "<span>Schedule Screen Off</span>";
    var schedTog = el("div", S.schedule_enabled ? "toggle on" : "toggle");
    var schedDetails = el("div");
    schedDetails.style.display = S.schedule_enabled ? "" : "none";

    schedTog.onclick = function () {
      S.schedule_enabled = !S.schedule_enabled;
      schedTog.className = S.schedule_enabled ? "toggle on" : "toggle";
      schedDetails.style.display = S.schedule_enabled ? "" : "none";
      schedBadge.className = "on-badge" + (S.schedule_enabled ? " active" : "");
      saveSetting("schedule_enabled", S.schedule_enabled);
    };
    schedTr.appendChild(schedTog);
    fSchedToggle.appendChild(schedTr);
    schedBody.appendChild(fSchedToggle);

    var fOnTime = field("On Time");
    var onSel = document.createElement("select");
    onSel.className = "select";
    var scheduleOnMin = productNumberMin("schedule_on_hour", 0);
    var scheduleOnMax = productNumberMax("schedule_on_hour", 23);
    for (var h = scheduleOnMin; h <= scheduleOnMax; h++) {
      var o = document.createElement("option");
      o.value = h;
      o.textContent = formatHour(h);
      if (h === Math.round(S.schedule_on_hour)) o.selected = true;
      onSel.appendChild(o);
    }
    onSel.onchange = function () {
      saveSetting("schedule_on_hour", parseInt(onSel.value));
    };
    fOnTime.appendChild(onSel);
    schedDetails.appendChild(fOnTime);

    var fOffTime = field("Off Time");
    var offSel = document.createElement("select");
    offSel.className = "select";
    var scheduleOffMin = productNumberMin("schedule_off_hour", 0);
    var scheduleOffMax = productNumberMax("schedule_off_hour", 23);
    for (var h2 = scheduleOffMin; h2 <= scheduleOffMax; h2++) {
      var o2 = document.createElement("option");
      o2.value = h2;
      o2.textContent = formatHour(h2);
      if (h2 === Math.round(S.schedule_off_hour)) o2.selected = true;
      offSel.appendChild(o2);
    }
    offSel.onchange = function () {
      saveSetting("schedule_off_hour", parseInt(offSel.value));
    };
    fOffTime.appendChild(offSel);
    schedDetails.appendChild(fOffTime);

    var fWakeTimeout = field("When Woken, Idle Time To Screen Off");
    var scheduleWakeMin = productNumberMin("schedule_wake_timeout", 10);
    var scheduleWakeMax = productNumberMax("schedule_wake_timeout", 3600);
    var scheduleWakeOptions = [10, 30, 60, 120, 300, 600, 1800, 3600].filter(function (v) {
      return v >= scheduleWakeMin && v <= scheduleWakeMax;
    });
    var scheduleWakeCurrent = normalizeScheduleWakeTimeout(S.schedule_wake_timeout);
    if (scheduleWakeOptions.indexOf(scheduleWakeCurrent) === -1) {
      scheduleWakeOptions.push(scheduleWakeCurrent);
      scheduleWakeOptions.sort(function (a, b) { return a - b; });
    }
    fWakeTimeout.appendChild(
      selectFromOptions(scheduleWakeOptions, scheduleWakeCurrent, function (v) {
        saveSetting("schedule_wake_timeout", v);
      }, formatDurationSeconds)
    );
    schedDetails.appendChild(fWakeTimeout);

    schedBody.appendChild(schedDetails);
    return makeCollapsibleCard("Night Schedule", schedBody, true, schedBadge);

  }

  function makeRotationCard() {
    // Rotation
    var rotationBody = el("div");
    var fRotation = field("Rotation");
    var rotationOptions = screenRotationOptionsForUi();
    fRotation.appendChild(
      selectFromOptions(rotationOptions, effectiveScreenRotationForUi(), function (v) {
        saveSetting("screen_rotation", v);
        renderSettings();
      }, function (v) {
        return v + " degrees";
      })
    );
    rotationBody.appendChild(fRotation);
    return makeCollapsibleCard("Rotation", rotationBody, true);

  }

  function makeClockCard() {
    // Clock
    var clockBadge = makeBadge(S.show_clock);
    var clkBody = el("div");
    var f5 = field("");
    var tr = el("div", "toggle-row");
    tr.innerHTML = "<span>Show Clock</span>";
    var tog = el("div", S.show_clock ? "toggle on" : "toggle");
    tog.onclick = function () {
      S.show_clock = !S.show_clock;
      tog.className = S.show_clock ? "toggle on" : "toggle";
      clockBadge.className = "on-badge" + (S.show_clock ? " active" : "");
      saveSetting("show_clock", S.show_clock);
    };
    tr.appendChild(tog);
    f5.appendChild(tr);
    clkBody.appendChild(f5);

    var f6 = field("Format");
    f6.appendChild(
      selectFromOptions(productSettingOptions("clock_format"), S.clock_format, function (v) {
        saveSetting("clock_format", v);
      })
    );
    clkBody.appendChild(f6);

    var f7 = field("Timezone");
    f7.appendChild(
      timezoneSelect(S.tz_options, S.timezone, function (v) {
        saveSetting("timezone", v);
      })
    );
    clkBody.appendChild(f7);
    clkBody.appendChild(ntpServersField());
    return makeCollapsibleCard("Clock", clkBody, true, clockBadge);

  }


  function makeDeviceWifiCard() {
    // WiFi reconfiguration. Lets the user change networks from the running UI
    // instead of having to forget and re-enter the captive-portal AP.
    var body = el("div");

    var current = el("div", "field");
    var currentLabel = document.createElement("label");
    currentLabel.textContent = "Current Network";
    current.appendChild(currentLabel);
    var currentValue = document.createElement("div");
    currentValue.className = "key-mask";
    currentValue.style.textAlign = "left";
    currentValue.textContent = S.wifi_current_ssid || "(disconnected)";
    current.appendChild(currentValue);
    body.appendChild(current);

    var ssidField = field("New SSID");
    var ssidInput = input("text", "", S.wifi_current_ssid || "My Home WiFi", 32);
    ssidInput.setAttribute("aria-label", "New WiFi SSID");
    ssidInput.autocomplete = "off";
    ssidInput.spellcheck = false;
    ssidField.appendChild(ssidInput);
    body.appendChild(ssidField);

    var pwField = field("Password");
    var pwGroup = el("div", "input-group");
    var pwInput = input("password", "", "WiFi password (blank for open networks)", 63);
    pwInput.setAttribute("aria-label", "New WiFi password");
    pwInput.autocomplete = "new-password";
    pwGroup.appendChild(pwInput);
    var showBtn = el("button", "btn btn-secondary");
    showBtn.textContent = "Show";
    showBtn.type = "button";
    showBtn.onclick = function () {
      var isPass = pwInput.type === "password";
      pwInput.type = isPass ? "text" : "password";
      showBtn.textContent = isPass ? "Hide" : "Show";
    };
    pwGroup.appendChild(showBtn);
    pwField.appendChild(pwGroup);
    body.appendChild(pwField);

    // Scan networks --------------------------------------------------------
    var scanField = field("Nearby Networks");
    var scanRow = el("div", "input-group");
    var scanBtn = el("button", "btn btn-secondary");
    scanBtn.type = "button";
    scanBtn.textContent = "Scan";
    var scanStatus = el("div");
    scanStatus.className = "key-mask";
    scanStatus.style.textAlign = "left";
    scanStatus.textContent = S.wifi_scan_results || "Tap Scan to list nearby networks.";
    scanRow.appendChild(scanBtn);
    scanField.appendChild(scanRow);
    scanField.appendChild(scanStatus);
    body.appendChild(scanField);

    function renderScanResults(raw) {
      scanStatus.innerHTML = "";
      var text = String(raw || "").trim();
      if (!text || text === "(no networks found)") {
        scanStatus.textContent = text || "(no networks found)";
        return;
      }
      if (text === "Scanning...") {
        scanStatus.textContent = "Scanning...";
        return;
      }
      var lines = text.split("\n");
      lines.forEach(function (line) {
        var row = document.createElement("div");
        row.style.cursor = "pointer";
        row.style.padding = "4px 0";
        row.textContent = line;
        // Strip trailing " (-NN dBm)" to extract just the SSID for click-to-fill.
        var match = line.match(/^(.*) \(-?\d+ dBm\)$/);
        var ssid = match ? match[1] : line;
        row.onclick = function () {
          ssidInput.value = ssid;
          pwInput.focus();
        };
        scanStatus.appendChild(row);
      });
    }

    scanBtn.onclick = function () {
      if (!endpoints.wifi_scan_networks) return;
      scanBtn.disabled = true;
      var originalText = scanBtn.textContent;
      scanBtn.textContent = "Scanning...";
      scanStatus.textContent = "Scanning...";
      post(endpoints.wifi_scan_networks + "/press").then(function () {
        // Firmware scan takes ~5s; poll the results sensor for up to 12s.
        var attempts = 0;
        function poll() {
          attempts++;
          safeGet(endpoints.wifi_scan_results).then(function (resp) {
            var value = resp && (resp.value || resp.state);
            if (value && value !== "Scanning...") {
              renderScanResults(value);
              scanBtn.disabled = false;
              scanBtn.textContent = originalText;
              return;
            }
            if (attempts >= 12) {
              scanBtn.disabled = false;
              scanBtn.textContent = originalText;
              scanStatus.textContent = "Scan timed out. Try again.";
              return;
            }
            setTimeout(poll, 1000);
          });
        }
        setTimeout(poll, 1500);
      });
    };

    // Save & reconnect -----------------------------------------------------
    var saveRow = el("div", "backup-row");
    var saveBtn = el("button", "btn btn-primary");
    saveBtn.type = "button";
    saveBtn.textContent = "Save and Reconnect";

    // Forget current WiFi --------------------------------------------------
    var forgetBtn = el("button", "btn btn-secondary");
    forgetBtn.type = "button";
    forgetBtn.textContent = "Forget WiFi";
    forgetBtn.title = "Clears saved WiFi credentials and reboots into the captive-portal setup AP.";

    saveBtn.onclick = function () {
      var ssidValue = (ssidInput.value || "").trim();
      if (!ssidValue) {
        showBanner("Enter a WiFi SSID before saving.", "error");
        ssidInput.focus();
        return;
      }
      if (!endpoints.wifi_new_ssid || !endpoints.wifi_new_password || !endpoints.wifi_save_and_reconnect) {
        showBanner("WiFi reconfig endpoints are not available on this firmware.", "error");
        return;
      }
      saveBtn.disabled = true;
      forgetBtn.disabled = true;
      showBanner("Saving WiFi credentials. Device will reboot \u2014 reconnect this browser to the same WiFi (or the setup AP if connection fails) and reload in ~30s.", "success");
      Promise.resolve()
        .then(function () { return postTextValueSet(endpoints.wifi_new_ssid + "/set", ssidValue); })
        .then(function () { return postTextValueSet(endpoints.wifi_new_password + "/set", pwInput.value || ""); })
        .then(function () { return delayMs(600); })
        .then(function () { return post(endpoints.wifi_save_and_reconnect + "/press"); })
        .catch(function () {
          saveBtn.disabled = false;
          forgetBtn.disabled = false;
          showBanner("Failed to save WiFi credentials. Try again.", "error");
        });
    };

    forgetBtn.onclick = function () {
      if (!endpoints.wifi_forget_and_restart_setup) return;
      var confirmed = window.confirm(
        "Forget the current WiFi network?\n\nThe device will reboot into the captive-portal setup AP. " +
          "You will need to connect to the device's WiFi hotspot to reconfigure it."
      );
      if (!confirmed) return;
      saveBtn.disabled = true;
      forgetBtn.disabled = true;
      showBanner("Forgetting WiFi. Device will reboot into the setup AP.", "success");
      post(endpoints.wifi_forget_and_restart_setup + "/press").catch(function () {
        saveBtn.disabled = false;
        forgetBtn.disabled = false;
        showBanner("Failed to forget WiFi. Try again.", "error");
      });
    };

    saveRow.appendChild(saveBtn);
    saveRow.appendChild(forgetBtn);
    body.appendChild(saveRow);

    return makeCollapsibleCard("WiFi Network", body, true);
  }
