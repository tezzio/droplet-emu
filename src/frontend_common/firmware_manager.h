// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/common_types.h"
#include "core/core.h"
#include <array>

#include "core/hle/result.h"
#include "core/hle/service/set/settings_types.h"
#include "core/hle/service/set/system_settings_server.h"

namespace FirmwareManager {

// Droplet: no key or firmware install, no firmware checks. Only the version lookup is left; the
// version comes from the built-in system version archive when no firmware is present.

/**
 * @brief Get the currently installed firmware version.
 * @param system The system to check firmware on.
 * @return A pair of the firmware version format and result code.
 */
inline std::pair<Service::Set::FirmwareVersionFormat, Result> GetFirmwareVersion(Core::System &system)
{
    Service::Set::FirmwareVersionFormat firmware_data{};
    const auto result
        = Service::Set::GetFirmwareVersionImpl(firmware_data,
                                               system,
                                               Service::Set::GetFirmwareVersionType::Version2);

    return {firmware_data, result};
}

// TODO(crueter): GET AS STRING
}
