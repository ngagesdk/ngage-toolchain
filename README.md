[![N-Gage SDK.](https://raw.githubusercontent.com/ngagesdk/ngage-toolchain/master/media/ngagesdk-readme-header.png)](https://raw.githubusercontent.com/ngagesdk/ngage-toolchain/master/media/ngagesdk-header-2x-white.png?raw=true "N-Gage SDK.")

# ngage-toolchain

[![Continuous Integration](https://github.com/ngagesdk/ngage-toolchain/actions/workflows/nokia-ngage.yml/badge.svg)](https://github.com/ngagesdk/ngage-toolchain/actions/workflows/nokia-ngage.yml)

A C/C++ homebrew toolchain for the Nokia N-Gage.

## What is this all about?

This toolchain is the heart of the Nokia N-Gage SDK project.

The project provides a collection of tools, libraries, and
documentation that allows to develop homebrew software for the
Nokia N-Gage.  It includes a C/C++ toolchain, a collection of
example applications, and a set of libraries that are compatible
with the N-Gage's hardware. 

## Where do I start?

Clone the toolchain:

```bash
git clone https://github.com/ngagesdk/ngage-toolchain.git
```

Create the environment variable `$NGAGESDK` and set it to the
root-directory of the toolchain.  
**Important**: Since normalizing paths with CMake does not seem to work
properly, be sure to use slashes instead of backslashes,
e.g. `C:/ngage-toolchain`.

Once you are done, open the `setup` directory in Visual Studio and wait
until the CMake solution has finished generating.  That's it.

### Important

The C++ compiler for Symbian S60 is based on Cygwin and requires access
to `cygwin1.dll`.  In order for this DLL to be found, the `$PATH`
environment variable must be updated.

The same applies to the C compiler, which, however, requires access to
different DLLs.

It should be sufficient to add these two paths to your `$PATH`
environment variable:

`%NGAGESDK%\sdk\sdk\6.1\Shared\EPOC32\gcc\bin`  
`%NGAGESDK%\sdk\sdk\6.1\Shared\EPOC32\ngagesdk\bin`

## Licence and Credits

- This project is licensed under the "The MIT License".  See the file
  [LICENSE.md](LICENSE.md) for details.

- The SDK itself is from Nokia and various third-party sources.
  These components are not subject to the terms of "The MIT License".

- The [N-Gage SDK logo](media/) by [Dan Whelan](https://danwhelan.ie) is
  licensed under a Creative Commons [Attribution-ShareAlike 4.0
  International (CC BY-SA
  4.0)](https://creativecommons.org/licenses/by-sa/4.0/) license.

If you are interested in the Nokia N-Gage in general, you are cordially
invited to visit our small online community. You can find us on
[Discord](https://discord.gg/dbUzqJ26vs) and
[Telegram](https://t.me/nokia_ngage).
