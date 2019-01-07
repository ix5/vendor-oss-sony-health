/*
 * Copyright (C) 2018 The Android Open Source Project
 * Copyright (C) 2019 Felix Elsner
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "android.hardware.health@2.0-service.sony"

#include <android-base/logging.h>
#include <health2/service.h>
/* frameworks/native/services/batteryservice/include/batteryservice/BatteryService.h */
/* no need to include healthd.h or health2/Health.h */
#include <batteryservice/BatteryService.h>

#include "CycleCountBackupRestore.h"
#include "LearnedCapacityBackupRestore.h"

namespace {
using ::device::sony::health::CycleCountBackupRestore;
using ::device::sony::health::LearnedCapacityBackupRestore;
static CycleCountBackupRestore ccBackupRestore;
static LearnedCapacityBackupRestore lcBackupRestore;
}  // anonymous namespace

/* The pointer behind healthd_config has meaning here! Do not put it in front of */
/* health_config! */
void healthd_board_init(struct healthd_config *) {
    /* TODO: Isn't this already implemented by kernel drivers/power/supply/qpnp-fg.c
     *       via cycle_counter stuff? */
    ccBackupRestore.Restore();
    /* TODO: Same for learned capacity, seems it is already handled fine by them bms */
    lcBackupRestore.Restore();
}

/* int healthd_board_battery_update() { */
int healthd_board_battery_update(struct android::BatteryProperties *props) {
    ccBackupRestore.Backup(props->batteryLevel);
    lcBackupRestore.Backup();
    // return 0 to log periodic polled battery status to kernel log
    return 0;
}

int main() {
    return health_service_main();
    // Hosting our own interface(i.e. not "default") will result in boot failure
    // since Android wants android.hardware.health@2.0::IHealth/default
    // We could however start a second interface, but there's no reason to at the moment
    // since we only want to implement the default one. In case we decide to
    // launch a second interface at some point in the future, note that the
    // manifest needs to be amended as well, and a second interface needs to be
    // added to the .rc file too.
    //return health_service_main("sony");
}
