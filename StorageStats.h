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

#ifndef DEVICE_SONY_HEALTH_STORAGESTATS_H
#define DEVICE_SONY_HEALTH_STORAGESTATS_H

#include <health2/Health.h>

namespace device {
namespace sony {
namespace health {


class StorageStats {
   public:
    StorageStats();

    void GetStorageInfo(std::vector<StorageInfo> &vec_storage_info);
    void GetDiskStats(std::vector<DiskStats> &vec_stats);

   private:

    void GetStorageVariant();
    void FillStoragePaths();
    bool ResolveEmmcPaths();

    void ReadEmmcEol(StorageInfo *storage_info);
    void ReadEmmcLifetimes(StorageInfo *storage_info);
    void ReadEmmcVersion(StorageInfo *storage_info);
    void ReadUfsVersion(StorageInfo *storage_info);

    enum storagevariants {
        STORAGE_TYPE_EMMC = 1,
        STORAGE_TYPE_UFS = 2
    };
    int kStorageVariant;

    // UGLY
    /* const std::string kBlockPath; */
    /* const std::string kSocPath; */
    std::string kBlockPath;
    std::string kSocPath;

    std::string kEmmcDir;
    std::string kEmmcSocDir;
    std::string kEmmcEolFile;       // Format: 01
    std::string kEmmcLifetimeFile;  // Format: 0x02 0x02
    std::string kEmmcVersionFile;   // Format: 0x8
    std::string kDiskStatsFile;
    /* const std::string kEmmcName; */
    std::string kEmmcName = "MMC0";
    /* TODO: Average voltage */

    std::string kUfsDir;
    std::string kUfsSocDir;
    std::string kUfsEolFile;
    std::string kUfsLifetimeAFile;
    std::string kUfsLifetimeBFile;
    std::string kUfsVersionFile;
    /* const std::string kUfsName; */
    std::string kUfsName = "UFS0";

    // From system/core/storaged/storaged_info.cpp
    /* const char *kEmmcVersionString[9]; */
    const char *kEmmcVersionString[9] = {
        "4.0", "4.1", "4.2", "4.3", "Obsolete", "4.41", "4.5", "5.0", "5.1"
    };

    bool kStoragePathsAvailable;

    // Only log errors reading/scanning stats files once
    // TODO: Decide whether to ditch these and spam logcat to *really* alert users
    bool kLoggedErrorForPathsUnavailable;
    bool kLoggedErrorForEol;
    bool kLoggedErrorForVersion;
    bool kLoggedErrorForLifetime;
    bool kLoggedErrorForDiskStats;
};

}  // namespace health
}  // namespace sony
}  // namespace device

#endif  // #ifndef DEVICE_SONY_HEALTH_STORAGESTATS_H
