![regs_background_hw](https://user-images.githubusercontent.com/51358194/188197489-cdcd3f04-01a4-4c27-9cef-82f0f3eeffa5.png)

# reGS [![C/C++ CI](https://github.com/ScriptedSnark/reGS_win32/actions/workflows/CI.yml/badge.svg)]

Reverse-engineered GoldSrc. **Based on [GoldSourceRebuild](https://github.com/Triang3l/GoldSourceRebuild) repository.**

## Current projects

All projects based on latest Steam version of Half-Life. Compatibility with older versions is not guaranteed!

Original `version` output:
<pre>
Protocol version 48
Exe version 1.1.2.2/Stdio (valve)
Exe build: 19:53:27 Aug  3 2020 (8684)
</pre>

- dedicated (hlds.exe) | Status: WIP (not implemented yet)
- GameUI (GameUI.dll) | Status: WIP (without CareerUI)
- hw (hw.dll) | Status: WIP (only game menu without sound engine)
- launcher (hl.exe) | Status: Finished (also has improvements)
- particleman (particleman.dll) | Status: WIP (not implemented yet)
- tier0 (tier0.dll) | Status: Finished?
- tier1 (tier1.lib) | Status: Finished?
- vgui2 (vgui2.dll) | Status: WIP
- vgui2_controls (vgui2_controls.lib) | Status: WIP (requires re-reversing)
- vgui2_surfacelib (vgui2_surfacelib.lib) | Status: Finished (by [TheDoomsayer](https://github.com/TheDoomsayer))
- vstdlib (vstdlib.dll) | Status: WIP (not implemented yet)

## Future projects

- AdminServer | GUI for HLDS
- FileSystem_Stdio | File system
- ServerBrowser | VGUI2 window with a list of all available servers
- vgui_dll | VGUI1

## Downloads
* Release builds are unavailable.
* [Dev builds](https://github.com/ScriptedSnark/reGS_win32/actions/workflows/CI.yml)

<b>Warning!</b> reGS is still WIP so there are bugs, crashes, flaws and other issues. If you encounter them, report to [Issues](https://github.com/ScriptedSnark/reGS_win32/issues).

## Building

This project is for Windows 32-bit systems only.

### Visual Studio 2022
1. Install [Visual Studio 2022](https://visualstudio.microsoft.com/ru/vs/). In the Visual Studio Installer, select Desktop Development for C++.
2. Open Visual Studio.
3. On the starting screen, click "Clone or check out code".
4. Enter `https://github.com/ScriptedSnark/reGS_win32.git` and press the Clone button. Wait for the process to finish.
5. Choose Debug or Release configuration. You can build the workspace using Build→Build All. If you want to build only VGUI2 or something else, right click on wanted project (for example, `GameUI`) and select Build.

## Credits

Thanks to [TheDoomsayer](https://github.com/TheDoomsayer) (the real skillful gigachad)! He helped me to learn reverse-engineering.

Thanks to [Terrorgor](https://github.com/Terrorgor) for reverse-engineering collaboration when reGS was private.
