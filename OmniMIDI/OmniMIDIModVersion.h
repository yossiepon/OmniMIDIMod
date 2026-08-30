#pragma once

// Mod version — single source of truth for OmniMIDI.h and Resource.rc
// Build script (build-omnimidi.sh) auto-updates MOD_VER_DATE_STR and MOD_COPYRIGHT_YEARS.
#define MOD_VER_MAJOR     14
#define MOD_VER_MINOR     8
#define MOD_VER_PATCH     5
#define MOD_VER_REV       65535

// Auto-derived from numeric macros (no manual sync required)
#define _MOD_STRINGIFY(x) #x
#define _MOD_TOSTRING(x) _MOD_STRINGIFY(x)
#define MOD_VER_STRING    _MOD_TOSTRING(MOD_VER_MAJOR) "." _MOD_TOSTRING(MOD_VER_MINOR) "." _MOD_TOSTRING(MOD_VER_PATCH) "." _MOD_TOSTRING(MOD_VER_REV)

#define MOD_VER_DATE_STR  "2026-08-30"

#define MOD_COPYRIGHT_YEAR_START  2026
#define MOD_COPYRIGHT_YEARS  "2026"
