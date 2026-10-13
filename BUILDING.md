# Building UnholyUnion

The firmware uses the JieLi pi32v2 Linux x86-64 toolchain. An ordinary host C compiler runs the tests, but cannot produce FM-1 firmware.

## Target firmware

Obtain the vendor toolchain and inspect its license before use. Release 0.2.3 used `jieli-linux-toolchains-20260730.1.tar.xz`, SHA-256 `a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd`. The compiler root contains `pi32v2/` and `common/`.

The source/SDK-data release archive includes the three pinned build inputs beneath `sdk-data/cpu/wl82/tools/`, plus their license and provenance. It does not bundle the compiler. From the archive's `source/` directory on Linux x86-64:

```sh
export JIELI_TOOLCHAIN=/path/to/jieli-linux-toolchains
python3 -m venv .venv
. .venv/bin/activate
python3 -m pip install Pillow fonttools
python3 tools/build.py --release 0.2.3 --sdk ../sdk-data
python3 tests/target_budget.py build/felucca.dis
```

The build checks RAM text, flash/RAM/pool fit, register access and package integrity. Output is `build/unholyunion-0.2.3-experimental.fwsc`; inherited intermediate names still use `felucca`. Do not substitute upstream binaries for this fork. Independent builds of the released app, ELF, loader and package matched byte for byte.

## Host tests

```sh
python3 tools/test_unholy_host.py --generate
node web/test_web.mjs
node web/test_backup.mjs
node web/test_holy.mjs
node web/test_unholy_installer.mjs
node web/test_unholy_channels.mjs
node web/test_unholy_clean_install.mjs
```

The native runner generates firmware descriptors before editor parity tests. Run C suites sequentially. Full web tests need those descriptors. Tests use mocks and native DSP; they do not flash a device. Target instruction accounting keeps the inherited +10% threshold, and is not measured hardware timing.

## Local browser preview

Copy the released `.fwsc` file into `web/` with its published filename before testing the installer. From the repository root:

```sh
python3 -m http.server 8768 --bind 127.0.0.1 --directory web
```

Open `http://127.0.0.1:8768/unholy-installer.html?demo=1` for a simulation with no MIDI access. Opening the ordinary page also does not request permissions or start installation. A real update requires the compatible device, matching package, backup acknowledgement, risk consent and explicit Install click. Cancel is available before update entry; keep power stable once writing starts.
