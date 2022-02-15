[![N-Gage SDK.](https://raw.githubusercontent.com/ngagesdk/ngage-toolchain/master/media/ngagesdk-readme-header.png)](https://raw.githubusercontent.com/ngagesdk/ngage-toolchain/master/media/ngagesdk-header-2x-white.png?raw=true "N-Gage SDK.")

# ngage-toolchain

A toolchain for Symbian S60v1 devices such as the Nokia N-Gage.

## What is this all about?

First of all, this is not a leak of the so far unreleased Nokia N-Gage
SDK (which was ultimately just an extended Symbian SDK), but a
comprehensive system that allows the original and freely available
Symbian S60v1 SDK to be used with modern tools such as CMake and Visual
Studio 2022.

[![VS 2022 animation](https://raw.githubusercontent.com/ngagesdk/ngage-toolchain/master/media/ngagesdk-vs-2022.gif)](https://raw.githubusercontent.com/ngagesdk/ngage-toolchain/master/media/ngagesdk-vs-2022.gif?raw=true "VS 2022 animation")

This toolchain is the core of this project and provides the basis for
all projects that build on it.

## The problem

Developing apps for Symbian is unnecessarily complicated.  That was no
different 20 years ago than it is today and is certainly one of the main
reasons why a homebrew scene never emerged for the Nokia N-Gage.

Until now, the easiest way to do this was to set up a virtual machine,
install the SDK and a completely outdated version of Visual Studio.  In
addition, there was the proprietary build system, which required a Perl
installation that could be installed along with it.

In a nutshell, Symbian development has never been a convenient thing.

## The solution

CMake, because it provides a perfect foundation for automating
complicated build procedures and is natively supported by many modern
development environments such as Visual Studio.

Even though this project is still in its early stages, it is already
possible to compile apps for the platform in a relatively uncomplicated
manner.  Overall, the toolchain has a modular design and can be extended
by additional libraries as needed.

## Where do I start?

Clone the toolchain including all sub-modules:

```bash
git clone --recurse-submodules -j4 https://github.com/ngagesdk/ngage-toolchain.git
```

Create the environment variable `$NGAGESDK` and set it to the
root-directory of the toolchain.  
**Important**: Since normalizing paths with CMake does not seem to work
properly, be sure to use slashes instead of backslashes,
e.g. `C:/ngage-toolchain`.

Once you are done, open the `setup` directory in Visual Studio and wait
until the CMake solution has finished generating.  Then build the
project so that the necessary dependencies are downloaded and installed.

### Important

The GCC compiler for Symbian S60 is based on Cygwin. For it to work, its
path `%NGAGESDK%\sdk\6.1\Shared\EPOC32\gcc\bin` should be included in
your `$PATH` environment variable, otherwise it will not be able to find
the file `cygwin1.dll`.

## Future goals

The project has an old, functional port of SDL 1.2, which can be used to
develop small demos and games quite easily, although the use of SDL 1.2
is generally not recommended.

As a result, many open source games that were initially ported or
developed using SDL 1.2 have been adapted and now only support SDL 2.

For this reason, one of the main goals is to port SDL 2 to make it as
convenient as possible for future developers to create new games or to
port existing games to the platform.

In addition, a Symbian Application Information File (`.aif`) cannot yet
be generated with the SDK.  This file allows you to customise the icon
of an application. This is one of the short-term goals

Also, eventually a customisable exe-launcher will be added, which can
be used to start the generated SDL applications. This was common at the
time because Symbian DLLs, unlike `.exe` files, do not have writable
static data.  There are ways and means to overcome this difficulty, but
a separate process also offers the benefit of isolation.

## Licence and Credits

- This project is licensed under the "The MIT License".  See the file
  [LICENSE.md](LICENSE.md) for details.

- The [N-Gage SDK logo](media/) by [Dan Whelan](https://danwhelan.ie) is
  licensed under a Creative Commons [Attribution-ShareAlike 4.0
  International (CC BY-SA
  4.0)](https://creativecommons.org/licenses/by-sa/4.0/) license.

If you are interested in the Nokia N-Gage in general, you are cordially
invited to visit our small online community. You can find us on
[Discord](https://discord.gg/dbUzqJ26vs),
[Telegram](https://t.me/nokia_ngage) and in #ngage on
[EFnet](http://www.efnet.org/).
