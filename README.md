# WeaponRequirementsFramework

Configurable weapon strength, ammunition, skill, movement, and power armor
requirements for Fallout 4. The native plugin integrates with
[ItemIntegrationFramework](https://github.com/wushen233/ItemIntegrationFramework)
to add requirement information to item cards and apply shared damage modifiers.

Mod page and downloads: [Weapon Requirements Framework on Nexus Mods](https://www.nexusmods.com/fallout4/mods/101555)

## Features

- Data-driven strength and skill requirements for weapons
- Optional ammunition requirement calculation
- Movement and power armor restrictions
- Configurable combat penalties when requirements are not met
- Item card integration through ItemIntegrationFramework
- Public native Papyrus query API
- Multi-runtime Fallout 4 support through CommonLibF4

## Source layout

- `src`: native F4SE/CommonLibF4 plugin and IIF provider integration.
- `papyrus`: public native Papyrus declarations.
- `config/F4SE`: default WRF rules, IIF integration, and RobCo Patcher data.
- `config/MCM`: Mod Configuration Menu definition and defaults.
- `config/Interface`: English and Chinese translations.

Compiled DLL, PEX, ESP, and UI assets are not included. Install the complete
mod package from Nexus Mods for normal gameplay.

## Requirements

- Windows 10 or later
- Visual Studio 2022 Build Tools with Desktop development with C++
- Git
- XMake 3.0 or later
- [Dear-Modding-FO4/commonlibf4](https://github.com/Dear-Modding-FO4/commonlibf4)
- Fallout 4 Script Extender
- ItemIntegrationFramework

## Build

This project must be built against the
[Dear-Modding-FO4 CommonLibF4 fork](https://github.com/Dear-Modding-FO4/commonlibf4).
The build script downloads the pinned revision from that fork into `.deps/`
when needed, configures XMake, and builds the native plugin:

```powershell
.\scripts\build.ps1
```

To use an existing checkout of the same CommonLibF4 fork:

```powershell
.\scripts\build.ps1 -CommonLibF4Path D:\path\to\commonlibf4
```

The resulting DLL is placed under `build/windows/x64/releasedbg/` by XMake.
Compile `papyrus/WRF_Native.psc` with the Fallout 4 Papyrus compiler when
building a complete mod package.

## Configuration

The repository keeps editable runtime configuration under `config` using the
same directory structure expected below Fallout 4's `Data` directory. Copy
only the files needed by your development install or package pipeline.

The released ESP is required by the MCM and some default rules. It is supplied
through the Nexus package because compiled game plugins are outside this source
repository.

## License

Original WRF source is released under the MIT License. Built binaries also
contain `commonlib-shared` code distributed under GPL-3.0 with the CommonLib
Modding Exception. Vendored Xbyak remains under its BSD-3-Clause license. The
copied IIF consumer API remains covered by the IIF project license. See
`THIRD_PARTY_NOTICES.md` and `licenses/commonlib-shared`.
