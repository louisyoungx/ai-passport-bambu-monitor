#pragma once

// Credentials are optional: a clean checkout boots into web provisioning.
// Keep accepting an ignored secrets.h for existing installations.
#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.h.example"
#endif
