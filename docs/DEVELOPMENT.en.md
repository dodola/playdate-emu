# Development & Building

[English](DEVELOPMENT.en.md) · [简体中文](DEVELOPMENT.md)

For user introduction and downloads, see [Project Home](../README.en.md). This page covers the development environment, source structure, and build commands; refer to platform-specific documents for detailed operations.

## Source Tree Navigation

| Directory | Contents |
|---|---|
| [`Unreal`](../outputs/ue5-playdate/Unreal) | Android UE project, CrankRuntime plugin, content assets, UI and interaction |
| [`runtime`](../outputs/ue5-playdate/runtime) | Lua / C API compatibility layer, ARM execution backend, and shared host interface |
| [`translation`](../outputs/ue5-playdate/translation) | Local inference, online translation, Chinese fonts, and historical experiments |
| [`switch`](../outputs/ue5-playdate/switch/README.md) | Switch native frontend, PDGames integration, NRO build and verification |
| [`libretro`](../outputs/ue5-playdate/libretro/README.md) | Linux RetroArch core, build and regression tests |
| [`tools`](../outputs/ue5-playdate/tools) | Android / Windows build, asset conversion, and verification scripts |
| [`playdate-local`](../outputs/playdate-local/README.md) | Initial web prototype model and interaction reference |

## Android / Windows

```powershell
git clone --recurse-submodules https://github.com/dodola/cranklab.git
cd cranklab/outputs/ue5-playdate
```

Tested with Windows, UE 5.8.3, Visual Studio 2022 C++ tools, Android NDK 27.2.12479018, CMake 3.22.1, and JDK 21. Default script paths match the development environment; pass `EngineDirectory`, `SdkDirectory`, `NdkDirectory`, and `JdkDirectory` according to your installation. Local Playdate SDK is required for native test games and some generated APIs.

Windows Native Components & UE Editor:

```powershell
./tools/Build-Runtime.ps1
./tools/Build-Unreal.ps1
```

Android ARM64:

```powershell
./tools/Build-AndroidRuntime.ps1 -Abi arm64-v8a
./tools/Build-Unreal.ps1 -Platform Android -Architecture arm64
./tools/Cook-Unreal.ps1 -Architecture arm64 -SkipBuild
```

The current Android APK also requires the local translation library and backend plugin. Build with `translation/build-snapdragon.sh` and place artifacts into the plugin's Android translation library directory following [Local Translation Build Instructions](../outputs/ue5-playdate/translation/LOCAL-TRANSLATION.md#构建与打包) before packaging with UE.

Project entry point: `Unreal/CrankLab.uproject`. The repository retains converted content assets; regular source builds do not need to re-run asset conversions. APK verification entry point: `tools/verify-translation-package.py`.

## Switch & Linux

- [Switch Build & Controls](../outputs/ue5-playdate/switch/README.md): Native NRO frontend; initial release supports Lua only, independent of UE.
- [Linux Core Quickstart](../outputs/ue5-playdate/libretro/README.md) and [Full Guide](../outputs/ue5-playdate/libretro/docs/GUIDE.md): Lua core by default; experimental ARM backend enabled separately.

## Translation & Dependencies

- [Local Translation](../outputs/ue5-playdate/translation/LOCAL-TRANSLATION.md): Model sources, download behavior, CPU / OpenCL / HTP, build and device records.
- [Online Translation](../outputs/ue5-playdate/translation/ONLINE-TRANSLATION.md): Microsoft & Tencent services, caching, and fallback records.
- [Third-Party Credits](../outputs/ue5-playdate/third-party.json): Compatible runtimes, original models, assets, and dependency versions.

Unicorn uses a locked Git submodule; SDL, SDL_mixer, zlib, and compatible Lua sources are kept in the repository.
