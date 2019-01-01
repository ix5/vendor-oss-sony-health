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

sepolicy file_contexts:
```
/(system/vendor|vendor)/bin/hw/android\.hardware\.health@2\.0-service\.sony                  u:object_r:hal_health_default_exec:s0
```

## Terminology

- SoC = State of Charge from 0-100% (n.b. SoC is normally understood to mean System-on-chip)
- BMS = Battery Management System

**USB terms**

- SDP = Standard downstream port(2.5-500mA)
- CDP = Charging downstream port(up to 1500mA)
- DCP = Dedicated charging port(up to 1500mA)

**Type-C terms:**

- UFP = Upstream-facing port
- DFP = Downstream-facing port

## Sources

- [The Basics of USB charging](https://www.maximintegrated.com/en/app-notes/index.mvp/id/4803)
