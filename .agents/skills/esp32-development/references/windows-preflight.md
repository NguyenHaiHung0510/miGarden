# Windows preflight and command effects

Use this to plan or inspect an ESP32 bring-up on Windows without inventing board detection. No helper is bundled in this installed skill.

The reviewed upstream `esp32-preflight.py` uses Python standard-library `shutil.which` and filesystem patterns. Its supported port patterns cover Linux and macOS. On Windows it deliberately returns `candidate_ports: []` with an unsupported-enumeration message; that result does **not** mean no board is attached. It was statically inspected and was not executed in the acquisition batch.

For a task that permits host discovery, correlate the exact board's markings/user guide with Windows Device Manager's **Ports (COM & LPT)** and USB device properties or an already-installed framework's documented listing. Record the observation and ambiguity. Do not auto-select the first COM port, infer SoC or firmware from a USB bridge, install a driver to clear a gap, disconnect an unrelated device, or open every port. A plug/unplug comparison itself requires access to the correct physical device.

[Espressif's serial connection guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/establish-serial-connection.html#check-port-on-windows) documents Device Manager and Windows COM naming. Select the correct chip and SDK-version documentation; this link's `stable` content can change. Driver installation is a separate system change, not part of skill discovery.

| Operation | Evidence and possible effects |
|---|---|
| Read existing logs/manifests/schematics | Does not contact the target; logs/config can contain credentials |
| Inspect a host's tool path or device properties | Does not identify running image or commission a COM mapping on its own |
| SDK help/version | Establishes available syntax/version, not correct target or harmlessness of another command |
| Build/configure | Writes generated output/config; build scripts may fetch dependencies or run hooks; inspect before execution |
| Serial monitor, ROM-loader identity query, debugger | May acquire exclusive port, toggle DTR/RTS, reset, load a RAM stub or affect timing even if called a read operation |
| Native test setup | May discover a different target, flash, reset or erase; inspect the exact fixture/plugin version first |
| Flash/reset/erase/OTA/power cycling | Changes target state or persistent data and requires the corresponding authorized sequence and recovery evidence |

For an authorized ESP-IDF build, preserve source/config/tool versions, partition table, generated flash arguments and image/ELF identity. Use the project's generated target/offset metadata; never fill `PORT`, `TARGET` or flash offsets by guessing. Native commands in external guides are examples to validate, not a run list. Stack/board/version choices stay unresolved until supported by project decisions and exact artifacts.
