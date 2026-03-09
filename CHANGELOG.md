# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.9.92] - 2026-03-08

### Changed
- **Modernized Framework**: Upgraded to **og3 v0.5.0** and its extension libraries (**og3x-oled**, **og3x-shtc3**).
- **Consolidated Configuration**: Refactored the build system to use a single `local.ini` file with multiple environments (e.g., `livingroom`, `workshop`) instead of switching between separate secret files.
- **Improved Dependency Management**: Switched to environment-scoped hardware options and ensured all transitive dependencies are correctly resolved using `lib_ldf_mode = deep+`.
- **API Modernization**: Updated all web handlers to use the new `(NetRequest*, NetResponse*)` signature.
