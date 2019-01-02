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

#include "CycleCountBackupRestore.h"

/* TODO: Log tag here or in header? */
#define LOG_TAG "android.hardware.health@2.0-service.sony"

// LOG_LEVEL = 6?

#define BCP__ "BatteryCycleCount: "

namespace device {
namespace sony {
namespace health {

static constexpr int kBackupTrigger = 20;

CycleCountBackupRestore::CycleCountBackupRestore(int nb_buckets)
    : nb_buckets_(nb_buckets),
      saved_soc_(-1),
      soc_inc_(0)
{
    sw_bins_ = new int[nb_buckets];
    memset(sw_bins_, 0, sizeof(sw_bins_));
    hw_bins_ = new int[nb_buckets];
    memset(hw_bins_, 0, sizeof(hw_bins_));
}

void CycleCountBackupRestore::Restore() {
    Read(kPersistCycleFile, sw_bins_);
    Read(kSysCycleFile, hw_bins_);
    UpdateAndSave();
}

void CycleCountBackupRestore::Backup(int battery_level) {
    if (saved_soc_ == -1) {
        saved_soc_ = battery_level;
        return;
    }
    // Cycle counts only increases on increasing level
    if (battery_level > saved_soc_) {
        soc_inc_ += battery_level - saved_soc_;
    }
    saved_soc_ = battery_level;
    // To avoid writting file too often just rate limit it
    if (soc_inc_ >= kBackupTrigger) {
        Read(kSysCycleFile, hw_bins_);
        UpdateAndSave();
        soc_inc_ = 0;
    }
}

void CycleCountBackupRestore::Read(const std::string &path, int *bins) {
    std::string buffer;

    if (!android::base::ReadFileToString(path, &buffer)) {
        LOG(ERROR) << BCP__ << "Failed to read cycles from " << path;
        return;
    }

    buffer = ::android::base::Trim(buffer);
    std::vector<std::string> counts = android::base::Split(buffer, " ");
    if (counts.size() != (size_t)nb_buckets_) {
        LOG(ERROR) << BCP__ << "data format \"" << buffer << "\" is wrong in " << path;
    } else {
        LOG(INFO) << BCP__ << "Read \"" << buffer << "\" cycles from " << path;
        for (int i = 0; i < nb_buckets_; ++i) {
            bins[i] = std::stoi(counts[i]);
        }
    }
}

//int CycleCountBackupRestore::Write(int *bins, const std::string &path) {
void CycleCountBackupRestore::Write(int *bins, const std::string &path) {
    std::string str_data = "";

    for (int i = 0; i < nb_buckets_; ++i) {
        if (i) {
            str_data += " ";
        }
        str_data += std::to_string(bins[i]);
    }

    // TODO: Use more gene_BCP << "Write to " << path << " error: " << strerror(errno);
    //    return -1;
    if (path == kPersistCycleFile) {
        //LOG(VERBOSE) << BCP__ << "Wrote \"" << str_data << "\" cycles to " << path;
        LOG(VERBOSE) << BCP__ << "Backed up cycle count of \"" << str_data << "\" to " << kPersistCycleFile;
    } else if (path == kSysCycleFile) {
        //LOG(VERBOSE) << BCP__ << "Wrote \"" << str_data << "\" cycles to " << path;
        LOG(VERBOSE) << BCP__ << "Restored cycle count of \"" << str_data << "\" from " << kPersistCycleFile;
    } else {
        // Add case here to avoid this message
        LOG(INFO) << BCP__ << "Wrote \"" << str_data << "\" to unknown file: " << path;
    }
    //return;
}

void CycleCountBackupRestore::UpdateAndSave() {
    bool backup = false;
    bool restore = false;
    for (int i = 0; i < nb_buckets_; i++) {
        if (hw_bins_[i] < sw_bins_[i]) {
            hw_bins_[i] = sw_bins_[i];
            restore = true;
        } else if (hw_bins_[i] > sw_bins_[i]) {
            sw_bins_[i] = hw_bins_[i];
            backup = true;
        }
    }
    //int ret;
    if (restore)
    {
        //ret = Write(hw_bins_, kSysCycleFile);
        Write(hw_bins_, kSysCycleFile);
        //if (ret == 0)
        //    LOG(VERBOSE) << BCP__ << "Restored cycle count from " << kPersistCycleFile;
    }
    if (backup)
    {
        //ret = Write(sw_bins_, kPersistCycleFile);
        Write(sw_bins_, kPersistCycleFile);
#if 0
        if (ret == 0)
        {
            //LOG(VERBOSE) << BCP__ << "Write \"" << str_data << "\" cycles to " << kPersistCycleFile;
            LOG(VERBOSE) << BCP__ << "Backed up cycle count to " << kPersistCycleFile;
        }
#endif
    }
}

}  // namespace health
}  // namespace sony
}  // namespace device
