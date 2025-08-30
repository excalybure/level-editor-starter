#pragma once

#include <catch2/catch_test_macros.hpp>
import platform.dx12;

// Utility to attempt headless initialization and emit a WARN then early-return from a test section
// Usage:
//   dx12::Device device;
//   if(!requireHeadlessDevice(device)) return; // inside a lambda passed to REQUIRE_NOTHROW or directly in TEST_CASE
inline bool requireHeadlessDevice(dx12::Device &device, const char *context = nullptr)
{
    if (device.initializeHeadless())
        return true;
    if (context)
        WARN("Skipping " << context << ": headless device initialization failed");
    else
        WARN("Skipping test: headless device initialization failed");
    return false;
}
