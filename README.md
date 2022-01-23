[![N-Gage SDK.](https://raw.githubusercontent.com/ngagesdk/ngage-toolchain/master/media/ngagesdk-readme-header.png)](https://raw.githubusercontent.com/ngagesdk/ngage-toolchain/master/media/ngagesdk-header-2x-white.png?raw=true "N-Gage SDK.")

# ngage-toolchain

A toolchain for Symbian S60v1 devices such as the Nokia N-Gage.

## About

First of all, this is not a leak of the so far unreleased Nokia N-Gage
SDK (which was ultimately just an extended Symbian SDK), but a
comprehensive system that serves the development of games and
applications for the Nokia N-Gage on modern systems.

This toolchain is the core of this project and provides the basis for
all projects that build on it.

If you are interested in the Nokia N-Gage in general, you are cordially
invited to visit our small online community. You can find us on
[Discord](https://discord.gg/dbUzqJ26vs),
[Telegram](https://t.me/nokia_ngage) and in #ngage on
[EFnet](http://www.efnet.org/).

## Important

The GCC compiler for Symbian S60 is based on Cygwin. For it to work, its
path `[Project_Dir]\sdk\6.1\Shared\EPOC32\gcc\bin` should be included in
your `$PATH` environment variable, otherwise it will not be able to find
the file `cygwin1.dll`.

## Related repositories

- [SDL-1.2](https://github.com/ngagesdk/SDL-1.2)

## Credits

- The [N-Gage SDK logo](media/) by [Dan Whelan](https://danwhelan.ie) is
  licensed under a Creative Commons [Attribution-ShareAlike 4.0
  International (CC BY-SA
  4.0)](https://creativecommons.org/licenses/by-sa/4.0/) license.
