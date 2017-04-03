![Options Screen](/Screens/Options.png?raw=true)

Prizoop
=======

Prizoop is a Game Boy emulator for the Casio Prizm series graphing calculator, with intense focus on optimization and decent feature set for the target device. As such, it is not a very accurate emulator but is very fast and compatible. 

It got its name because it started out as a fork of the multiplatform Game Boy emulator, Cinoop, by CTurt. It has since undergone significant rewrites and improvements, and shares some of the cpu code, organization and constructs from Cinoop, but for the most part resembles Cinoop much less than a normal fork.

## Install

Copy the .g3a file and the Prizoop folder from this repository to your Casio Prizm calculator's root path when linked via USB. Gameboy roms (.gb) also should go inside of the root directory.

## Support

![Game Banner](/Screens/GameBanner.png?raw=true)

The emulator now plays over 95% of the games I have been able to test smoothly at this point. Some games where timing accuracy is very important suffer, specifically racing games. Road Rash and F1 Race are playable but have some visual issues, and F1 Pole Position does not play at all.

## Building

Project root must be within /projects directory of publicly available community SDK v0.3 for Prizm. The SDK must be patched with the latest version of libFXCG as well. To build on a Windows machine simply run make.bat. For other systems please refer to your Prizm SDK documentation on how to compile projects.

If you use Visual Studio, a project is included that uses a Windows Simulator I wrote that wraps Prizm OS functions so that the code and emulator can easily be tested and iterated on within Visual Studio. See the prizmsim.cpp/h code for details on its usage.

## Usage

In the menu system use the arrow keys and SHIFT to select.

F1 - Select ROM
F2 - Settings (most are self explanatory)
F6 - Play ROM (it will first show diagnostic information)

When inside a game, the MENU key will exit to the settings screen, and pressing MENU again will take you back to the calculator OS.

### Prizm Controls

You can configure your own keys in the Settings menu, these are the default I found to work well:

- B: OPTN
- A: SHIFT
- Select: F5
- Start: F6
- Dpad : Dpad

## Special Thanks

BGB was a huge part of bug fixing and obtaining great ROM compatibility. It is a Gameboy emulator with great debugging and memory visualization tools:
http://bgb.bircd.org/

A huge special thanks obviously goes to CTurt, who's simple explanations and easy to read source got this project rolling by making it seem much less scary. See the original Cinoop source code here:
https://github.com/CTurt/Cinoop
