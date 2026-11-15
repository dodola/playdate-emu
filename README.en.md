<p align="center">
  <img src="docs/assets/playdate-cover.svg" alt="Playdate — The little yellow handheld on Android, Switch, and Linux" width="100%">
</p>

<p align="center">
  <a href="README.en.md"><strong>English</strong></a> &nbsp; | &nbsp; <a href="README.md"><strong>简体中文</strong></a>
</p>

<h1 align="center">Playdate</h1>

<p align="center">
  <strong>Turn the crank, discover your next favorite game.</strong><br>
  Bringing the joy of the little yellow console to your smartphone and handheld devices.
</p>

<p align="center">
  <a href="https://github.com/dodola/cranklab/releases/tag/v0.3.0-beta.1"><strong>Download Beta</strong></a>
  &nbsp; · &nbsp;
  <a href="#how-to-play-on-android">Features</a>
  &nbsp; · &nbsp;
  <a href="#choose-your-device">Choose Device</a>
</p>

Playdate (CrankLab) is a community-driven Playdate compatibility project. Love fidgeting with the console? Enjoy the interactive 3D view. Want to focus purely on gameplay? Switch to immersive mode. You can also play on Nintendo Switch or Linux handhelds using familiar physical buttons.

## How to Play on Android

- **A 3D handheld you can see and interact with.** Drag the background to change perspective, double-tap the screen to zoom in; enable motion follow and the virtual console tilts gently with your phone.
- **Let games fill your screen.** Immersive mode enlarges game visuals, D-pad, and A/B buttons, playable in both portrait and landscape orientations.
- **The crank right at your fingertips.** Drag the on-screen crank dial or swipe up/down anywhere in immersive mode to experience Playdate's signature mechanic.
- **From discovery to gaming.** Launches into PDGames by default. Browse descriptions, download publisher-provided game packages, and play immediately after installation. Also supports importing local `.pdx` games.
- **Read game descriptions in your language.** Switch between original text and translated descriptions with automatic caching. Supports both online translation services and local on-device neural translation.
- **Tailored to your preferences.** Adjust motion tilt follow, haptic vibration, resolution scaling, and frame rate in Settings.

## Interface Preview

<p align="center">
  <img src="docs/assets/android-3d.png" alt="Android landscape 3D console interface showing PDGames list and interactive crank" width="100%">
  <br><sub>3D Handheld · PDGames · Interactive Crank</sub>
</p>

<p align="center">
  <img src="docs/assets/android-immersive.png" alt="Android portrait immersive interface with PDGames game list, large D-pad, A/B buttons, and crank dial" width="240">
  &nbsp; &nbsp;
  <img src="docs/assets/android-chinese.png" alt="PDGames details page showing translated description with original language toggle" width="240">
  <br><sub>Immersive Play · Game Details & Translation</sub>
</p>

Above screenshots were taken from **v0.3.0-beta.1 running on a Pixel 4**. You can also view the [landscape immersive interface](docs/assets/android-immersive-landscape.png).

## Choose Your Device

Current Release: **[v0.3.0-beta.1](https://github.com/dodola/cranklab/releases/tag/v0.3.0-beta.1)**

| Device | Best Experience | Download |
|---|---|---|
| **Android Phone** | 3D handheld view, immersive touch controls, PDGames & translation; Android 8.0+ ARM64 devices | [Android APK](https://github.com/dodola/cranklab/releases/download/v0.3.0-beta.1/Playdate-v0.3.0-beta.1-arm64.apk) |
| **Switch / Lite / OLED** | Physical buttons and analog stick crank, PDGames, local library; Requires homebrew-capable device | [Switch NRO](https://github.com/dodola/cranklab/releases/download/v0.3.0-beta.1/Playdate.nro) |
| **Linux Desktop & Handhelds** | Play in RetroArch with physical buttons and right analog stick | [Desktop x86_64](https://github.com/dodola/cranklab/releases/download/v0.3.0-beta.1/CrankLab-libretro-linux-x86_64-v0.3.0-beta.1.zip) · [Handheld ARM64](https://github.com/dodola/cranklab/releases/download/v0.3.0-beta.1/CrankLab-libretro-linux-aarch64-v0.3.0-beta.1.zip) |

The Android version provides the complete UI experience shown above. The initial Switch and Linux builds focus on **Lua games** and do not include the 3D console model or description translation. Android retains the ARM C game execution backend, though game compatibility across all platforms is continuously improving. Switch builds have not yet undergone full real-hardware validation.

## Starting Your First Game

**Android:** Download and install the APK, launch the app to enter PDGames. Choose download / run in game details, or import your own `.pdx` game packages. Updating an existing install with the same signature can be done directly over the previous version.

**Switch:** Place `Playdate.nro` into `/switch/playdate/` on your SD card, then launch from Homebrew Menu. Control games with D-pad and A/B buttons, and rotate the crank by rolling the right analog stick or holding ZR/ZL.

**Linux:** Download the zip package for your architecture, follow package instructions to install the core into RetroArch, then load your game. L/R or right stick rotates the crank; Start opens the game menu.

Setup guides: [Android](https://github.com/dodola/cranklab/releases/download/v0.3.0-beta.1/README-Android.zh-CN.md) · [Switch](outputs/ue5-playdate/switch/README.md) · [RetroArch / Linux](outputs/ue5-playdate/libretro/README.md)

---

This is an evolving **community beta project**, not an official emulator from Panic. Compatibility varies between games; feedback on devices, games tested, and issues encountered is warmly welcomed.

Web preview source code is also included: **[Playdate Web](outputs/ue5-playdate/web/README.md)**. Allows running games like Pooh Mini Mahjong and Music Box in a web browser using the 3D console or immersive touch interface (requires local build and hosting).

Interested in contributing? [Development & Building](docs/DEVELOPMENT.en.md) · [Translation Guide](outputs/ue5-playdate/translation/LOCAL-TRANSLATION.md) · [Third-Party Credits & Assets](outputs/ue5-playdate/third-party.json)
