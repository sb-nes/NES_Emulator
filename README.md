# NES_Emulator

## Currently Tested:
1. SRAM/WRAM - `Passed`
2. CPU/R-MOS-6502 - `Passed`
3. Cartridge ROM Loading [.nes/ines 1.0] - `Passed`
4. Mapper 0 - `Passed`
5. PPU/R-MOS-2C02 - Testing | Failing
6. Output Window - Testing | Somehow Passing
7. Timing - `Passed`
8. Basic NES Controller - `Passed`


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

#### Credit to javidx9 [not a clone of his olc_nes project] for his basic overview explanation of the Nintendo Entertainment System, NesHacker for his in-depth explanations and all the people behind the NesDev Wiki Reference Guide for it's documentations.

##### Copyright of the Hardware belongs to Nintendo, 1985.
