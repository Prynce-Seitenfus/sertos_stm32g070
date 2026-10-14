# SerTOS STM32G070 Demo

Demo firmware for [SerTOS](https://github.com/Prynce-Seitenfus/sertos) running
on an STM32G070RB (Cortex-M0+, LQFP64). The STM32CubeMX-generated project uses
CMake and Ninja and can run on hardware or in Renode.

## Demo

- The producer polls the user button on PC13. Debounced button presses toggle
  whether it enqueues incrementing values in a statically allocated SerTOS
  queue.
- The consumer reads the queue and changes the green LED according to each
  received value.
- A profiler task measures selected application and SerTOS functions and
  handles profiler commands over USART2 (PA2/PA3, 115200 baud, 8-N-1).

The profiler reports **active CPU cycles**: time while a task is switched out
is excluded. The `Measure` preset collects selected call sites using explicit profiler
scopes; instrumented builds collect function calls as well. The profiler UART
uses the interrupt-driven (IT) backend in every build configuration, so the
same Debug or Measure firmware works on hardware and in Renode.

## Repository layout

| Path | Contents |
| --- | --- |
| `Core/`, `Drivers/` | STM32CubeMX-generated startup, HAL, and CMSIS sources |
| `app/` | Demo tasks, queue, profiler integration, and SerTOS port |
| `cmake/` | ARM toolchain and STM32CubeMX CMake configuration |
| `renode/` | Renode machine scripts, launcher, and profiler-dump capture script |
| `sewcet/` | WCET analysis configuration for this firmware |
| `CMakeLists.txt`, `CMakePresets.json` | Firmware targets and build configurations |
| `build.bat` | Windows build wrapper |
| `burn.ps1` | ST-LINK/SWD programming wrapper |
| `serial-console.ps1` | Interactive terminal for a physical serial COM port |

## Prerequisites

The CMake project expects these sibling checkouts:

```text
<workspace>\
  sertos\
  profiler\
  stream\
  sertos_stm32g070\
```

The `sertos` checkout must contain the built Cortex-M0+ library
`sertos\lib\arm\libsertos_cortex_m0plus.a`. The `profiler` checkout must
contain `lib\arm\libprofiler_cortex_m0plus.a` and the selected backend sources
under `port\`. The `stream` checkout provides the profiler transport headers.
Set the CMake cache paths `SERTOS_DIR`, `PROFILER_DIR`, or `STREAM_DIR` if your
checkouts are elsewhere.

Install CMake 3.22 or newer, Ninja, and the GNU Arm Embedded toolchain
(`arm-none-eabi-gcc`). To use Renode, install Renode separately. To connect to
its TCP terminal interactively, install Ncat (included with Nmap). On Windows,
Nmap can be installed with:

```powershell
winget install --id Insecure.Nmap --exact
```

## Build

Run these commands from the repository root:

```powershell
cmake --preset Debug
cmake --build --preset Debug
```

The Windows wrapper configures and builds in one step:

```powershell
.\build.bat                 # Debug
.\build.bat Release
.\build.bat Measure
```

| Configuration | Profiler setup |
| --- | --- |
| `Debug` | Timer overflow-interrupt port; application instrumentation off |
| `Release` | Cascaded 16-bit timer port; application instrumentation off |
| `Measure` | Debug settings with explicit call-site scopes |

Use `-i` to build the instrumented SerTOS library and instrument selected
application sources. Use `-m` as a shortcut for the `Measure` preset, and `-c`
to clean the selected configuration before building. For example:

```powershell
.\build.bat -i
.\build.bat -m
.\build.bat -c -i
```

Build artifacts are written to `build\<Configuration>\`, including
`sertos_stm32g070.elf`.

## Run on hardware

Flash the ELF with the ST-LINK/SWD programmer wrapper:

```powershell
.\burn.ps1 .\build\Debug\sertos_stm32g070.elf
```

`burn.ps1` expects STM32CubeProgrammer's `STM32_Programmer_CLI.exe` at the path
configured in that script. It also accepts HEX and raw BIN images; BIN files
are written at flash address `0x08000000`.

Connect to the board's ST-LINK virtual COM port (or another serial adapter):

```powershell
.\serial-console.ps1 COM3 -BaudRate 115200 -LogFile .\session.log
```

The COM port is a positional argument. Use `/help` for console commands and
`/exit` or Ctrl+C to disconnect.

## Run in Renode

Build Debug (or Measure) and launch Renode with the same firmware configuration:

```powershell
.\build.bat Debug
.\renode\run.ps1 -Config Debug
```

This opens the USART2 analyzer window. Add `-Headless` to disable the GUI; the
Renode monitor remains attached to the current PowerShell window. The launcher
defaults to `C:\renode\1.17.0\renode.exe`; if Renode is installed elsewhere,
pass its executable path with `-RenodePath`, for example:

```powershell
.\renode\run.ps1 -Config Debug -Headless `
    -RenodePath 'C:\Program Files\Renode\renode.exe'
```

Type `quit` in the Renode monitor to stop the simulation.

### Interactive USART2 terminal over TCP

Expose USART2 on a TCP port. Debug and Measure both use the interrupt-driven
UART backend required by the Renode STM32G0 model:

```powershell
.\build.bat Debug
.\renode\run.ps1 -Config Debug -Headless -ProfilerSocketPort 3456
```

If Renode is not at the launcher's default path, also pass `-RenodePath` as
shown above. In a second PowerShell window, connect with Ncat:

```powershell
ncat 127.0.0.1 3456
```

Enter profiler commands such as `prof-help`, `prof-status`, `prof-top`, or
`prof-reset`. Press Ctrl+C to disconnect Ncat; the Renode simulation continues
running. If Ncat is not recognized after installing Nmap, open a new terminal
or run `ncat.exe` using its full installation path.

This socket is an unauthenticated terminal. Keep it on a trusted machine and
network; do not expose it to untrusted networks.

### Capture and parse a profiler dump

Use `-i` for function-level instrumentation:

```powershell
.\build.bat Debug -i
.\renode\run.ps1 -Config Debug -Headless -ProfilerSocketPort 3456
```

Or use the explicitly scoped call sites:

```powershell
.\build.bat Measure
.\renode\run.ps1 -Config Measure -Headless -ProfilerSocketPort 3456
```

In another PowerShell window, capture a binary PROF-BIN v4 dump. The capture
script connects to the TCP port itself, so disconnect Ncat before running it:

```powershell
.\renode\capture-profiler-dump.ps1 -Port 3456 `
    -OutputFile .\build\renode-profiler.bin
python ..\profiler\parse_prof_dump.py .\build\renode-profiler.bin `
    --elf .\build\Debug\sertos_stm32g070.elf
```

When using `Measure`, change the ELF path to
`.\build\Measure\sertos_stm32g070.elf`.

Run the parser from this repository root. Resolving function addresses from the
ELF also requires `arm-none-eabi-addr2line` to be available on `PATH`. `prof-dump`
transmits binary data, so use the capture script rather than copying its output
from a text terminal.
