Prizoop
=======

Fork of the multiplatform Game Boy emulator, Cinoop, by CTurt, with intense focus on optimization and improved feature set for the Casio Prizm.

Currently 100% C with a bit of direct register usage for the Prizm, and will likely feature a bit of assembly language hopefully soon :-)

Read my article about writing Cinoop [here](http://cturt.github.io/cinoop.html).

## Support

**Games:** Tetris is most likely the only playable game,

## Building

Project root must be within /projects directory of publicly available community SDK v0.3 for Prizm. To build on a Windows machine simply run make.bat. For other systems please refer to your Prizm SDK documentation on how to compile projects.

## Usage

Currently hardcoded to look for tetris.gb in the root of your Prizm, will hopefully support filesystem crawl in the future.

### Prizm Controls
- B: ALPHA
- A: SHIFT
- Select: OPTN
- Start: VARS
- Dpad : Dpad

## Special Thanks

A huge special thanks obviously goes to CTurt, who's simple explanations and easy to read source got this project rolling by making it seem much less scary. See the original Cinoop source code here:
https://github.com/CTurt/Cinoop
