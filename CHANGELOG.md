# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.1] - 2026-11-21

### Changed
- **Platform & Framework Upgrade**: Upgraded to `espressif32@^8.0.0` (Arduino-ESP32 3.x / ESP-IDF 5.x) and `chl33/og3@^0.7.1`, updated `lib_ldf_mode` to `chain`, and aligned sensor/display library dependencies.
- **Svelte Asset Caching & CI**: Updated `build-svelte.sh` to use `--etag=always`, pinned `svelteesp32@3.2.5` in `svelte/package.json`, switched CI to Node 22 with `npm ci`, and excluded generated `svelteesp32async.h` from formatting and license checks.

## [1.0.0] - 2026-03-30

### Added
- **Dynamic Svelte Web Interface**: A completely new, responsive web interface that adapts to the board's hardware configuration.
- **JSON API**: Full API support for status, configuration, WiFi, and MQTT settings using the `camelCase` standard.

### Changed
- **Standardized Naming**: All API keys and internal variables migrated to `camelCase` for consistency across the Room133/Plant133/Garden133 family.
- **Legacy Interface Migration**: The old HTML-only interface has been moved to `/old` and `/old_config`.
- **OTA Reliability**: Switched to the `min_spiffs.csv` partition layout to restore dual-app-partition support for safe wireless updates.
