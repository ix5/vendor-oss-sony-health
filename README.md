# Custom vendor health HAL

Check:

- system/core/healthd/
- hardware/interfaces/health/2.0/
- frameworks/native/services/batteryservice/include/batteryservice/BatteryService.h
- frameworks/native/services/sensorservice/BatteryService.h
- frameworks/base/services/core/java/com/android/server/am/ActivityManagerService.java
- frameworks/base/core/jni/com_android_server_AlarmManagerService.cpp

Add to common/vintf/manifest.xml before vendor.display.config:
```
    <hal format="hidl">
        <name>android.hardware.health</name>
        <transport>hwbinder</transport>
        <version>2.0</version>
        <interface>
            <name>IHealth</name>
            <instance>default</instance>
        </interface>
    </hal>
```

common-treble.mk:
```
# Health (in addition to own board libhealth)
PRODUCT_PACKAGES += \
    android.hardware.health@2.0-service.sony
```

init.common.rc:
```
on fs
    # For android.hardware.health@2.0-service.sony battery stats
    mkdir /mnt/vendor/persist/battery 0700 system system
```
(or let health@sony.rc do it)
```
service vendor-health...
    class hal
    # Use dynamic HALs?
    # see https://source.android.com/devices/architecture/hal/dynamic-lifecycle
    # init language extension, provides information of what service is served
    # if multiple interfaces are served, they can be specified one on each line
    interface android.hardware.health@2.0::IHealth default
    # restarted if hwservicemanager dies
    # would also cause the hal to start early during boot if oneshot wasn't set
    # will not be restarted if it exits until it is requested to be restarted
    oneshot
    # will only be started when requested
    disabled
```


sepolicy file_contexts:
```
/(system/vendor|vendor)/bin/hw/android\.hardware\.health@2\.0-service\.sony                  u:object_r:hal_health_default_exec:s0
```
sepolicy vendor_init.te:
```
allow vendor_init persist_battery_file:dir create_dir_perms;
```

## Terminology

- SoC = State of Charge from 0-100% (n.b. SoC is normally understood to mean System-on-chip)
- BMS = Battery Management System

**USB terms**

- SDP = Standard downstream port(2.5-500mA)
- CDP = Charging downstream port(up to 1500mA)
- DCP = Dedicated charging port(up to 1500mA)

**Type-C terms:**

- UFP = "Upstream-facing port", i.e. the port that flows towards the computer
- DFP = "Downstream-facing port", i.e. the port that flows from the device
  towards another device, perhaps USB audio output

## Sources

- [The Basics of USB charging](https://www.maximintegrated.com/en/app-notes/index.mvp/id/4803)
