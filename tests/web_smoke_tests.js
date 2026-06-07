const assert = require("assert/strict");
const fs = require("fs");
const path = require("path");

const root = path.resolve(__dirname, "..");
const app = fs.readFileSync(path.join(root, "docs/public/webserver/app.js"), "utf8");

const requiredFlowSnippets = [
  "function renderWizard()",
  "connect your photo frame",
  "saveConnectionValue(endpoints.immich_url",
  "saveConnectionValue(endpoints.api_key",
  "function renderSettings()",
  "Immich Server URL",
  "Photo Source",
  "schedulePhotoSourceApply",
  "Add an album",
  "Add a person",
  "function exportConfig()",
  "function importConfig()",
  "JSON.stringify(data, null, 2)",
  "Settings imported successfully",
  "function buildLogsPage(parent)",
  "addEventListener(\"log\"",
];

for (const snippet of requiredFlowSnippets) {
  assert.ok(app.includes(snippet), `generated web app is missing ${snippet}`);
}

assert.equal(/__ESPFRAME_[A-Z0-9_]+__/.test(app), false, "generated web app must not contain placeholders");

console.log("web smoke tests passed");
