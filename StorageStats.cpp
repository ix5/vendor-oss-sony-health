/*
 * Report storage health and disk stats by reading EMMC/UFS sysfs nodes.
 * Storage health reporting is not available for UFS since there is no kernel
 * support for UFS health as of 2019-07-12.
 *
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

#include <errno.h>
#include <stdexcept>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/strings.h>

#include <fstream>

#include "StorageStats.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#ifdef HEALTH_DEBUG
#include "Mock.h"
#endif  // HEALTH_DEBUG

namespace device {
namespace sony {
namespace health {

using android::base::ReadFileToString;

using android::hardware::health::V2_0::DiskStats;
using android::hardware::health::V2_0::StorageInfo;

#ifdef HEALTH_DEBUG
static HealthTestingMock mock;
#endif  // HEALTH_DEBUG

StorageStats::StorageStats() {
    /* static const std::string kBlockPath = "/sys/block/mmcblk0"; */
    kBlockPath = "/sys/block/mmcblk0";
    /* Expected to resolve to e.g. */
    /* /sys/devices/platform/soc/7464900.sdhci/mmc_host/mmc0/mmc0:0001/block/mmcblk0 */
    /* We want the raw emmc/ufs infos from here as well: */
    /* /sys/devices/platform/soc/7464900.sdhci/mmc_host/mmc0/mmc0:0001/ */

    // Moved from "/sys/devices/soc" on 4.4
    /* static const std::string kSocPath = "/sys/devices/platform/soc"; */
    kSocPath = "/sys/devices/platform/soc";

    //static std::string kEmmcDir;
    //static std::string kEmmcSocDir;
    //static std::string kEmmcEolFile;       // Format: 01
    //static std::string kEmmcLifetimeFile;  // Format: 0x02 0x02
    //static std::string kEmmcVersionFile;   // Format: 0x8
    //static std::string kDiskStatsFile;
    /* static const std::string kEmmcName = "MMC0"; */
    //kEmmcName = "MMC0";
    /* TODO: Average voltage */

    //static std::string kUfsDir;
    //static std::string kUfsSocDir;
    //static std::string kUfsEolFile;
    //static std::string kUfsLifetimeAFile;
    //static std::string kUfsLifetimeBFile;
    //static std::string kUfsVersionFile;
    /* static const std::string kUfsName = "UFS0"; */
    //kUfsName = "UFS0";

    //static bool kStoragePathsAvailable;

    // Only log errors reading/scanning stats files once
    // TODO: Decide whether to ditch these and spam logcat to *really* alert users
    kLoggedErrorForPathsUnavailable = false;
    kLoggedErrorForEol = false;
    kLoggedErrorForVersion = false;
    kLoggedErrorForLifetime = false;
    kLoggedErrorForDiskStats = false;

    LOG(WARNING) << "kBlockPath = " << kBlockPath;
    LOG(WARNING) << "kEmmcDir = " << kEmmcDir;
    LOG(WARNING) << "Constructor: Running GetStorageVariant()";
    GetStorageVariant();
    LOG(WARNING) << "Constructor: Running FillStoragePaths()";
    FillStoragePaths();
}

void StorageStats::ReadEmmcEol(StorageInfo *storage_info) {
    std::string buffer;
    if (!ReadFileToString(kEmmcEolFile, &buffer)) {
        if (!kLoggedErrorForEol) {
            LOG(WARNING) << "Could not read " << kEmmcEolFile;
            kLoggedErrorForEol = true;
        }
        return;
    }
    buffer = android::base::Trim(buffer);
    try {
        storage_info->eol = (uint16_t)std::stoul(buffer, nullptr, 0);
    } catch (std::invalid_argument) {
        if (!kLoggedErrorForEol) {
            LOG(WARNING) << "Could not parse " << kEmmcEolFile << ": invalid argument";
            kLoggedErrorForEol = true;
        }
    } catch (std::out_of_range) {
        if (!kLoggedErrorForEol) {
            LOG(WARNING) << "Could not parse " << kEmmcEolFile << ": '"
                         << buffer << "' out of range";
            kLoggedErrorForEol = true;
        }
    } catch (...) {
        // TODO: BAD BAD BAD
        LOG(ERROR) << "Unexpected exception while parsing " << kEmmcEolFile
                   << ", buffer contents: '" << buffer << "'. FIX THIS!!!";
    }
}

void StorageStats::ReadEmmcVersion(StorageInfo* storage_info) {
    uint16_t version;
    std::string buffer;
    if (!ReadFileToString(kEmmcVersionFile, &buffer)) {
        if (!kLoggedErrorForVersion) {
            LOG(WARNING) << "Could not read " << kEmmcVersionFile;
            kLoggedErrorForVersion = true;
        }
        return;
    }
    buffer = android::base::Trim(buffer);
    if (sscanf(buffer.c_str(), "0x%hx", &version) < 1) {
        if (!kLoggedErrorForVersion) {
            LOG(WARNING) << "Could not scan " << kEmmcVersionFile;
            kLoggedErrorForVersion = true;
        }
        return;
    }
    if (version < 7 || version > ARRAY_SIZE(kEmmcVersionString)) {
        if (!kLoggedErrorForVersion) {
            LOG(WARNING) << "Invalid emmc version: " << buffer;
            kLoggedErrorForVersion = true;
        }
        return;
    }
    storage_info->version = "emmc " + std::string(kEmmcVersionString[version]);
}

void StorageStats::ReadEmmcLifetimes(StorageInfo *storage_info) {
    // TODO: Use individual pointers
    std::string lifetimes;
    uint16_t lifetime_a = 0, lifetime_b = 0;
    if (!ReadFileToString(kEmmcLifetimeFile, &lifetimes)) {
        if (!kLoggedErrorForLifetime) {
            LOG(WARNING) << "Could not read " << kEmmcLifetimeFile << ": "
                         << strerror(errno);
            kLoggedErrorForLifetime = true;
        }
        return;
    }
    lifetimes = android::base::Trim(lifetimes);
    if ((sscanf(lifetimes.c_str(), "0x%2hx 0x%2hx", &lifetime_a, &lifetime_b) < 2)
            || (lifetime_a == 0 && lifetime_b == 0)) {
        if (!kLoggedErrorForLifetime) {
            LOG(WARNING) << "Could not scan " << kEmmcLifetimeFile;
            LOG(WARNING) << "Contents do not match '0x\%hx 0x\%hx' scheme: "
                         << lifetimes;
            kLoggedErrorForLifetime = true;
        }
        return;
    }
    storage_info->lifetimeA = lifetime_a;
    storage_info->lifetimeB = lifetime_b;
}

// TODO: Maybe replace with standard android::base::ReadFileToString ?
template <typename T>
bool read_value_from_file(const std::string &path, T *field) {
    std::ifstream stream(path);
    if (!stream.is_open()) {
        LOG(WARNING) << "Cannot read " << path;
        return false;
    }
    stream.unsetf(std::ios_base::basefield);
    stream >> *field;
    return true;
}

void StorageStats::ReadUfsVersion(StorageInfo *storage_info) {
    // TODO: Maybe use decimal values, but it seems hex is the way to go
    std::string version;
    if (!ReadFileToString(kUfsVersionFile, &version)) {
        if (!kLoggedErrorForVersion) {
            LOG(WARNING) << "Could not read " << kUfsVersionFile;
            kLoggedErrorForVersion = true;
        }
        return;
    }
    version = android::base::Trim(version);
    storage_info->version = "ufs " + version;  // "ufs 0x08"
}

void StorageStats::GetStorageVariant() {
    // TODO: Auto expressions like:
    // "if ((auto res = get_prop("prop.name", "0") == "0") {"
    const std::string bootdevice = android::base::GetProperty("ro.boot.bootdevice", "0");
    LOG(WARNING) << "Bootdevice: " << bootdevice;
    if (bootdevice == "0") {
        LOG(WARNING) << "Could not determine bootdevice storage type from props,"
                     << " storage health info will be unavailable";
        return;  // FAIL
    }
    if (bootdevice.find(".sdhci") != std::string::npos) {
        kStorageVariant = STORAGE_TYPE_EMMC;
        kEmmcSocDir = kSocPath + "/" + bootdevice;
        LOG(WARNING) << "Bootdevice is EMMC, kEmmcSocDir = " << kEmmcSocDir;
        LOG(WARNING) << "Bootdevice: storage type = " << STORAGE_TYPE_EMMC;
    } else if (bootdevice.find(".ufshc") != std::string::npos) {
        kStorageVariant = STORAGE_TYPE_UFS;
        kUfsSocDir = kSocPath + "/" + bootdevice;
        LOG(WARNING) << "Bootdevice is EMMC, kUfsSocDir = " << kUfsSocDir;
        LOG(WARNING) << "Bootdevice: storage type = " << STORAGE_TYPE_UFS;
    } else {
        LOG(WARNING) << "Bootdevice type not recognized (" << bootdevice
                     << "), storage health info will be unavailable";
    }
}

void StorageStats::FillStoragePaths() {
    LOG(WARNING) << "FillStoragePaths: storage type: " << kStorageVariant;
    switch (kStorageVariant) {
        case STORAGE_TYPE_EMMC:
            LOG(WARNING) << "Storage type EMMC";
            kStoragePathsAvailable = ResolveEmmcPaths();
            kEmmcEolFile = kEmmcDir + "/pre_eol_info";    // Format: 01
            kEmmcLifetimeFile = kEmmcDir + "/life_time";  // Format: 0x02 0x02
            // TODO: Maybe use "fwrev", "hwrev" instead?
            kEmmcVersionFile = kEmmcDir + "/rev";         // Format: 0x8
            kDiskStatsFile = kBlockPath + "/stat";
            break;
        case STORAGE_TYPE_UFS:
            LOG(WARNING) << "Storage type UFS";
            kUfsEolFile = kUfsSocDir + "/health/eol";
            // TODO: Format might be different
            kUfsLifetimeAFile = kUfsSocDir + "/health/lifetimeA";
            kUfsLifetimeBFile = kUfsSocDir + "/health/lifetimeB";
            kUfsVersionFile = kUfsSocDir + "/version";
            // TODO: Get path for UFS specifically
            kDiskStatsFile =  "/sys/block/sda/stat";
            break;
        default:
            kStoragePathsAvailable = false;
            LOG(WARNING) << "No storage path found"; // TODO: Removeme
            return;
    }
    LOG(WARNING) << "Filled storage paths";
}

/* Resolve the symlink from /sys/block/mmcblk0 and strip off the "block" part */
/* TODO: Might be easier to read from here: */
/* kEmmcDir = "/sys/bus/mmc/devices/mmc0:0001/"; */
bool StorageStats::ResolveEmmcPaths() {
    // TODO: Does this need to be freed? Compiler doesn't think so
    const char *realpath_buf = realpath(kBlockPath.c_str(), nullptr);
    if (realpath_buf == nullptr) {
        LOG(WARNING) << "Could not resolve " << kBlockPath
                     << ", storage health info will be unavailable";
        return false;
    }
    kEmmcDir.clear();  // TODO: Necessary?
    kEmmcDir.assign(realpath_buf);
    /* Trim the trailing part to get the mmc non-block sysfs paths */
    /* Full path would look like: */
    /* /sys/devices/platform/soc/7464900.sdhci/mmc_host/mmc0/mmc0:0001/block/mmcblk0 */
    /* But the node name could also be called "mmc0":aaaa or something similar */
    const std::size_t pos = kEmmcDir.find("/block/mmcblk0");
    if (pos == std::string::npos) {
        LOG(WARNING) << "Could not find a valid EMMC/SDHCI sysfs node in path " << kEmmcDir;
        LOG(WARNING) << "Storage health info will be unavailable";
        return false;
    }
    kEmmcDir = kEmmcDir.substr(0, pos);
    LOG(WARNING) << "ResolveEmmcPaths: kEmmcDir = " << kEmmcDir;
    return true;
}

void StorageStats::GetStorageInfo(std::vector<StorageInfo> &vec_storage_info) {
    vec_storage_info.resize(1);
    StorageInfo *storage_info = &vec_storage_info[0];
#ifdef HEALTH_DEBUG
    mock.FakeStorageInfo(storage_info);
    return;
#endif  // HEALTH_DEBUG
    if (!kStoragePathsAvailable) {
        if (!kLoggedErrorForPathsUnavailable) {
            LOG(WARNING) << "Could not get storage info because EMMC/UFS paths"
                         << " are not accessible";
            kLoggedErrorForPathsUnavailable = true;
        }
        return;
    }
    storage_info->attr.isInternal = true;
    storage_info->attr.isBootDevice = true;

    switch (kStorageVariant) {
        case STORAGE_TYPE_EMMC:
            ReadEmmcVersion(storage_info);
            ReadEmmcEol(storage_info);
            ReadEmmcLifetimes(storage_info);
            storage_info->attr.name = kEmmcName;
        case STORAGE_TYPE_UFS:
            // Not available since there is no kernel support for UFS health
            // reporting as of 2019-07-12
            return;
            /*
            ReadUfsVersion(storage_info);
            read_value_from_file(kUfsEolFile, &(storage_info)->eol);
            read_value_from_file(kUfsLifetimeAFile, &(storage_info)->lifetimeA);
            read_value_from_file(kUfsLifetimeBFile, &(storage_info)->lifetimeB);
            storage_info->attr.name = kUfsName;
            */
    }
}

// Based on crosshatch's HealthService.cpp
void StorageStats::GetDiskStats(std::vector<DiskStats> &vec_stats) {
    vec_stats.resize(1);
    DiskStats *stats = &vec_stats[0];
#ifdef HEALTH_DEBUG
    // Not implemented yet
    mock.FakeDiskStats(stats);
#endif  // HEALTH_DEBUG
    stats->attr.isInternal = true;
    stats->attr.isBootDevice = true;
    switch (kStorageVariant) {
        case STORAGE_TYPE_EMMC:
            stats->attr.name = kEmmcName;
        case STORAGE_TYPE_UFS:
            stats->attr.name = kUfsName;
    }

    std::ifstream stream(kDiskStatsFile);
    if (stream.is_open()) {
        // Regular diskstats entries
        stream >> stats->reads
               >> stats->readMerges
               >> stats->readSectors
               >> stats->readTicks
               >> stats->writes
               >> stats->writeMerges
               >> stats->writeSectors
               >> stats->writeTicks
               >> stats->ioInFlight
               >> stats->ioTicks
               >> stats->ioInQueue;
    } else {
        if (!kLoggedErrorForDiskStats) {
            LOG(WARNING) << "Could not open disk stats file " << kDiskStatsFile
                << ": " << strerror(errno);
            kLoggedErrorForDiskStats = true;
        }
    }
}

}  // namespace health
}  // namespace sony
}  // namespace device
