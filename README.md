# Custom vendor health HAL

Check:

- system/core/healthd/
- hardware/interfaces/health/2.0/
- frameworks/native/services/batteryservice/include/batteryservice/BatteryService.h
- frameworks/native/services/sensorservice/BatteryService.h
- frameworks/base/services/core/java/com/android/server/am/ActivityManagerService.java
- frameworks/base/services/core/jni/com_android_server_AlarmManagerService.cpp
- kernel: fs/timerfd.c for timerfd_create() from healthd_common.cpp, included
  through the android.hardware.health@2.0-impl shared_lib

Add to common/vintf/manifest.xml, e.g. before vendor.display.config:
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
# Remove deprecated backup healthd
DEVICE_FRAMEWORK_MANIFEST_FILE += \
    system/libhidl/vintfdata/manifest_healthd_exclude.xml
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
- FG = Fuel Gauge, similar to SoC
- BMS = Battery Management System

**USB terms**

- SDP = Standard downstream port(2.5-500mA)
- CDP = Charging downstream port(up to 1500mA)
- DCP = Dedicated charging port(up to 1500mA)

**Type-C terms:**

- UFP = "Upstream-facing port", i.e. the port that flows towards the computer
- DFP = "Downstream-facing port", i.e. the port that flows from the device
  towards another device, perhaps USB audio output

**Internals:**
- CC/CV = Constant current / constant voltage (CC/CV)
  For lithium batteries: When nearly depleted, use constant current for
  charging. When approaching max voltage, use contant voltage.
- OCV = Open circuit voltage
- CC = Coulomb counter
- PMIC, SPMI = ...
- FCC = Full charge capacity

## Sources

- [The Basics of USB charging](https://www.maximintegrated.com/en/app-notes/index.mvp/id/4803)
- [What is CC/CV mode?](http://www.bestgopower.com/faq/27-what-is-cc-cv-mode.html)
