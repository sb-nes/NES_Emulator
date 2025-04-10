# NES_Emulator

## Build Instructions:

Ufter cloning the repository, Build the project first to automatically download and link `glfw` using nuget. Then add `glew` [2.2.0 | rename the folder as 'glew.2.2.0' or update the include, lib and post-build 'xcopy' command location in project properties] and `sdl`[3.2.10 | rename the folder as 'sdl.3.2.10' or update the include, lib and post-build 'xcopy' command location in project properties] to the packages folder in solution directory.

If you decide to use newer versions of `glew` and/or `sdl`, make sure to update the include, lib and post-build 'xcopy' command location in project properties.

[Also, Remember to add a `Mapper 0` game as 'test.nes' to the debug output directory or change the location in the constructor of `Bus.h`]

## Currently Tested:
1. SRAM/WRAM - `Passed`
2. CPU/R-MOS-6502 - `Passed`
3. Cartridge ROM Loading [.nes/ines 1.0] - `Passed`
4. Mapper 0 - `Passed`
5. PPU/R-MOS-2C02 - Testing | Failing
6. Output Window - Testing | Somehow Passing
7. Timing - `Passed`
8. Basic NES Controller - `Testing` | holding keys doesn't works all the time and there's a key rollover error.


![palette read 1](https://github.com/sb-nes/NES_Emulator/blob/sane/images/emu1.gif)

There was an error in reading `pattern tables`, due to the way I implemented switching between different emulated address spaces. It is seen on the right/second pattern table, which should have shown no data at all.


~~Speed of the output using `GDI` is slower than expected due to using Software Rendering. I'll either have to optimise some other parts `[hard way]`, or use some other window api which includes Hardware Rendering`[easy and logical way]`.~~


The real reason the CPU and the whole application itself was slow because of `std::cout` (and other functions related to it). (I'll post the time it takes for `cout` later)
![console output](https://github.com/sb-nes/NES_Emulator/blob/sane/images/console1.gif)


![pattern table output](https://github.com/sb-nes/NES_Emulator/blob/sane/images/emu3.gif)


Why does it feel like I'm in **Conjuring**?
![Unholy](https://github.com/sb-nes/NES_Emulator/blob/sane/images/SomethingWrong.png)


I can't seem to figure out the root of this problem, but it *plagues* my soul away.
![OnlyPipes](https://github.com/sb-nes/NES_Emulator/blob/sane/images/OnlyPipes.png)


<ins>The timing seems to work, but i don't know for sure.</ins> I'll just say it passed and move on. Maybe Someday, I'll Visit This Part. 
![Timing](https://github.com/sb-nes/NES_Emulator/blob/sane/images/FPS.png)


`Whoops`:
![Failed Tests](https://github.com/sb-nes/NES_Emulator/blob/sane/images/FailedTests.png)


Fixed it after a whole day of brain-storming. The whole program was failing majorly due to BIT test setting the `N` and `V` Flags after performing the AND operation (*It was supposed to set the flags from the data read before performing the AND operation*). And Zero-Page with Offset addressing mode was supposed to wrap around, if it went past `0x00FF`. (Which I didn't implement at all)

**TLDR; Two Problems, Bit test and Zero-Page(Offset) addressing created the major nightmare.**
![Fixed it Finally](https://github.com/sb-nes/NES_Emulator/blob/sane/images/MuchBetterTests.png)


I opened task manager to see that, it was using my GPU. On a Win32 window, without any graphics API.

 ***SURPRISED PIKACHU FACE***

![Task my Manager](https://github.com/sb-nes/NES_Emulator/blob/sane/images/GPUonGDI.png)

[Debugging Funny Outputs](https://www.gridbugs.org/nes-emulator-debugging/)

This error was caused by trying to create pixels at the cycle 0 of a scanline.

![Fixed Glitch](https://github.com/sb-nes/NES_Emulator/blob/sane/images/BorderGlitch.png)

Another glitch (image below): I have **no** idea why this happens, but the MESEN emulator also faces the same problem.

![Common Glitch](https://github.com/sb-nes/NES_Emulator/blob/sane/images/RenderingAtBorder.png)

#### Credit to javidx9 [not a clone of his olc_nes project] for his basic overview explanation of the Nintendo Entertainment System, NesHacker for his in-depth explanations and all the people behind the NesDev Wiki Reference Guide for it's documentations.

##### Copyright of the Hardware belongs to Nintendo, 1985.
