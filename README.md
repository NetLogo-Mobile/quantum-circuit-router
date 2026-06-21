# quantum-circuits-router

A native diagram-routing engine that computes **component layout** and **orthogonal wire
routing** for circuit diagrams. It is packaged as a small C ABI (`qcrouter`) that takes a
JSON description of a circuit's nodes and edges and returns updated node positions and wire
routes. The library is consumed as a native plugin by the *Quantum Circuits* application
(Unity), with builds for Windows, Android, and iOS.

Layout and routing are built on top of the [Adaptagrams](http://www.adaptagrams.org/)
toolkit:

- **libavoid** – orthogonal connector (wire) routing around obstacles.
- **libcola / ACA** – constrained, force-directed node layout.

JSON parsing uses [nlohmann/json](https://github.com/nlohmann/json) (v2.1.1, vendored as
`json.hpp`).

## Repository layout

| Path | Description |
| --- | --- |
| `qcrouter.cpp`, `qcrouter.h` | The core engine and its exported C ABI. |
| `json.hpp` | Vendored nlohmann/json single-header library. |
| `libavoid/` | Vendored Adaptagrams sources (libavoid + cola/vpsc). **Third-party, LGPL** — see `libavoid/LICENSE.LGPL`. |
| `qcrouter/` | Android build of the native library (`DynamicLibrary`, Clang). `android.{cpp,h}` provide logging shims. |
| `qcrouter-win32/` | Windows DLL build (`qcrouter.dll`). |
| `import-console/` | .NET Framework 4.8 C# console harness that P/Invokes the native DLL for manual testing. |
| `quantum-circuits-router.sln` | Visual Studio solution tying the three projects together. |
| `distribute.bat` | Copies `qcrouter.*` and the libavoid sources into the Unity app's iOS plugin folder. |

## The native API

All entry points are exported with C linkage (`extern "C"`) and exchange data as UTF-8 JSON
strings. The caller is responsible for providing an output buffer; each call returns the
number of bytes written.

| Function | Purpose |
| --- | --- |
| `int HelloWorld(char* in, char* out)` | Smoke test — echoes the input JSON with `"hello": "world"` added. |
| `int RegisterRouter()` | Creates a router instance and returns its integer **ID** (reused slots when available). |
| `void UnregisterRouter(int id)` | Releases the router with the given ID. |
| `int CalculateLayout(int id, char* in, char* out)` | Applies the changes in `in`, then computes a new component layout. |
| `int CalculateRoute(int id, char* in, char* out)` | Applies the changes in `in`, then routes the wires between components. |

Routers are stateful: `CalculateLayout` / `CalculateRoute` first import the incremental
changes described in the input, mutate the persistent router model, then run the
layout/routing pass.

### Input format

```jsonc
{
  "Vertexes": [
    {
      "ID": 1,
      "Status": 2,                 // 1 = modified, 2 = added, 3 = removed
      "Position": { "X": 0, "Y": 0 },
      "Size":     { "X": 100, "Y": 60 },
      "Pins": [ { "X": 50, "Y": 0 } ] // pin offsets relative to the component centre
    }
  ],
  "Edges": [
    {
      "ID": 1,
      "Status": 2,                 // 2 = added, 3 = removed
      "Source": 1, "Target": 2,
      "SourcePin": 0, "TargetPin": 0
    }
  ]
}
```

### Output format

```jsonc
{
  "Succeeded": true,
  "Nodes": [ { "ID": 1, "X": 0.0, "Y": 0.0 } ],
  "Edges": [ { "ID": 1, "Routes": [ { "X": 0.0, "Y": 0.0 } ] } ],
  "Statistics": { "NodesChanged": 1, "EdgesChanged": 1 }
}
```

`CalculateLayout` populates `Nodes` (new component centres); `CalculateRoute` populates
`Edges` (poly-line routes) via `ExportChanges`. On failure, `Succeeded` is `false`.

## Building

Open `quantum-circuits-router.sln` in Visual Studio 2017+ (the solution was authored with
VS 15). It contains three projects:

- **qcrouter-win32** — builds `qcrouter.dll` for Windows (x64 / Win32).
- **qcrouter** — builds the native library for Android (Clang toolset; requires the
  Visual Studio C++ for Android workload).
- **import-console** — builds the C# test console (.NET Framework 4.8).

The C# project expects the native `qcrouter.dll` next to its executable (the Win32 project's
output path is configured to drop the DLL into the console's `bin\Debug`). Note that the
Windows project still carries machine-specific absolute paths (`OutDir`, `IncludePath`)
from the original author's environment; adjust these to your local checkout before building.

## Testing with the console harness

`import-console` is a minimal manual test driver. It registers a router, reads a circuit
description from a local `Input.txt`, prints the results of `CalculateLayout` and
`CalculateRoute`, then unregisters the router. Place an `Input.txt` (in the input format
above) next to the executable before running.

## License

The `libavoid/` directory contains third-party Adaptagrams code distributed under the
**GNU LGPL** (see `libavoid/LICENSE.LGPL`). The vendored `json.hpp` is MIT-licensed. 