![1280x640_regs_](https://user-images.githubusercontent.com/51358194/182458618-a3d8876f-d48a-4f91-a3da-d21c0658e189.png)

# reGS: reverse-engineered GoldSrc

This is the repository of reGS (comeback xD) source code without engine (hardware). **Based on [GoldSourceRebuild](https://github.com/Triang3l/GoldSourceRebuild) repository.**

## Current projects

- dedicated (hlds.exe) | Status: WIP (not implemented yet)
- GameUI (GameUI.dll) | Status: WIP (many bugs from Solokiller's project)
- launcher (hl.exe) | Status: Finished (also has improvements)
- particleman (particleman.dll) | Status: WIP (not implemented yet)
- tier0 (tier0.dll) | Status: Finished?
- tier1 (tier1.lib) | Status: Finished?
- vgui2 (vgui2.dll) | Status: WIP
- vgui2_controls (vgui2_controls.lib) | Status: WIP (requires re-reversing because also has many bugs from Solokiller's project)
- vgui2_surfacelib (vgui2_surfacelib.lib) | Status: Finished (by [TheDoomsayer](https://github.com/TheDoomsayer))
- vstdlib (vstdlib.lib) | Status: WIP (not implemented yet)

## Future projects

- AdminServer | GUI for HLDS
- FileSystem_Stdio | File system
- ServerBrowser | VGUI2 window with a list of all available servers
- vgui_dll | VGUI1

## Credits

Thanks to [TheDoomsayer](https://github.com/TheDoomsayer) (the real skillful gigachad)! He helped me to learn reverse-engineering.

Thanks to [xWhitey](https://github.com/xWhitey) for reverse-engineering collaboration when reGS was private.


## Building

This project is for Windows 32-bit systems only.

### Visual Studio 2019 & Visual Studio 2022
1. Install [Visual Studio 2019](https://my.visualstudio.com/Downloads?q=Visual%20Studio%20Community%202019) or [Visual Studio 2022](https://visualstudio.microsoft.com/vs/preview/vs2022/#download-preview). In the Visual Studio Installer, select Desktop Development for C++.
2. Open Visual Studio.
3. On the starting screen, click "Clone or check out code".
4. Enter `https://github.com/ScriptedSnark/reGS_win32.git` and press the Clone button. Wait for the process to finish.
5. Choose Debug or Release. You can build the workspace using Build→Build All. If you want to build only VGUI2 or something else, right click on wanted project (for example, `GameUI`) and select Build.
