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

#define LOG_TAG "android.hardware.health@2.0-service.sony:LearnedCapacacity"

#include "LearnedCapacityBackupRestore.h"

/* TODO: Improve log messages, e.g. "SRAM" is a lil non-friendly */
/* Better: "Saved learned maximum capacity of <x> mAh to persist storage" */

namespace device {
namespace sony {
namespace health {

static constexpr int kBuffSize = 256;

LearnedCapacityBackupRestore::LearnedCapacityBackupRestore() {}

void LearnedCapacityBackupRestore::Restore() {
    ReadFromStorage();
    ReadFromSRAM();
    UpdateAndSave();
}

void LearnedCapacityBackupRestore::Backup() {
    ReadFromSRAM();
    UpdateAndSave();
}

void LearnedCapacityBackupRestore::ReadFromStorage() {
    std::string buffer;

    if (!android::base::ReadFileToString(std::string(kPersistChargeFullFile), &buffer)) {
        LOG(ERROR) << "Cannot read the persist storage file";
        return;
    }

    if (sscanf(buffer.c_str(), "%d", &sw_cap_) < 1) {
        LOG(ERROR) << "Data format is wrong in the persist storage file: " << buffer;
    } else {
        /* TODO: Is it really mAh? */
        LOG(INFO) << " Read persist storage data: " << buffer << " max mAh";
    }
}

void LearnedCapacityBackupRestore::SaveToStorage() {
    char strData[kBuffSize];

    snprintf(strData, kBuffSize, "%d", sw_cap_);

    /* TODO: Is it really mAh? */
    LOG(INFO) << "Save to persist storage: " << strData << " max mAh";

    if (!android::base::WriteStringToFile(strData, std::string(kPersistChargeFullFile)))
        LOG(ERROR) << "Write persist file error: " << strerror(errno);
}

void LearnedCapacityBackupRestore::ReadFromSRAM() {
    std::string buffer;

    if (!android::base::ReadFileToString(std::string(kSysChargeFullFile), &buffer)) {
        LOG(ERROR) << "Read from SRAM error: " << strerror(errno);
        return;
    }

    buffer = android::base::Trim(buffer);

    if (sscanf(buffer.c_str(), "%d", &hw_cap_) < 1) {
        LOG(ERROR) << "Failed to parse SRAM bins: " << buffer;
    } else {
        /* TODO: Is it really mAh? */
        LOG(INFO) << "Read from SRAM: " << buffer << " mAh";
    }
}

void LearnedCapacityBackupRestore::SaveToSRAM() {
    char strData[kBuffSize];

    snprintf(strData, kBuffSize, "%d", hw_cap_);

    /* TODO: Is it really mAh? */
    LOG(INFO) << "Save to SRAM: " << strData << " max mAh";

    if (!android::base::WriteStringToFile(strData, std::string(kSysChargeFullFile)))
        LOG(ERROR) << "Write to SRAM error: " << strerror(errno);
}

void LearnedCapacityBackupRestore::UpdateAndSave() {
    bool backup = false;
    bool restore = false;
    if (hw_cap_) {
        if ((hw_cap_ < sw_cap_) || (sw_cap_ == 0)) {
            sw_cap_ = hw_cap_;
            backup = true;
        } else if (hw_cap_ > sw_cap_) {
            hw_cap_ = sw_cap_;
            restore = true;
        }
    }
    if (restore)
        SaveToSRAM();
    if (backup)
        SaveToStorage();
}

}  // namespace health
}  // namespace sony
}  // namespace device
