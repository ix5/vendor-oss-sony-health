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
#include <batteryservice/BatteryService.h>
/* no need to include healthd */
/* #include <health2/Health.h> */
/* #include <hidl/HidlTransportSupport.h> */

#include "CycleCountBackupRestore.h"
#include "LearnedCapacityBackupRestore.h"

/* #define HEALTH_NB_BUCKETS 8 */
/* Since cycle_count only reports an int, buckets are 1 for tone */
#define HEALTH_NB_BUCKETS 1

namespace {
    using ::device::sony::health::CycleCountBackupRestore;
    using ::device::sony::health::LearnedCapacityBackupRestore;
    static CycleCountBackupRestore ccBackupRestore(HEALTH_NB_BUCKETS);
    static LearnedCapacityBackupRestore lcBackupRestore;
}  // anonymous namespace


/* the pointer behind healthd_config has meaning here! do not put it in front of */
/* health_config! */
void healthd_board_init(struct healthd_config*)
{
    /* LOG(INFO) << "Restoring cycle count"; */
    ccBackupRestore.Restore();
    /* LOG(INFO) << "Restoring learned capacity"; */
    lcBackupRestore.Restore();
}

/* int healthd_board_battery_update() { */
int healthd_board_battery_update(struct android::BatteryProperties *props)
{
    /* LOG(INFO) << "Saving cycle count"; */
    ccBackupRestore.Backup(props->batteryLevel);
    /* LOG(INFO) << "Saving learned capacity"; */
    lcBackupRestore.Backup();
    // return 0 to log periodic polled battery status to kernel log
    return 0;
}

int main()
{
    return health_service_main("sony");
}
