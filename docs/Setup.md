# Setting up the tools you need to work on F.E.I.S.

## Debian-ish Linux

### I just want to compile

Install the big general "I'm a real C/C++ dev !" package.

```console
$ sudo apt install build-essential
```

Install meson (the build system). I recommend doing so via python's `pip` to
get a more up-to-date version than what your distro packages might have

```console
$ pip install meson
```

Unfortunately this also means meson will not come with ninja so we need to install it ourselves :

```console
$ sudo apt install ninja-build
```

Install SFML

```console
sudo apt install libsfml-dev
```

Then checkout [this page](docs/Compiling.md) for instructions on how to compile

### I also want to contribute some code

Install `clang-format`

```console
$ sudo apt install clang-format
```

## Windows

### MSYS2

MSYS2 is the only windows compiling thingy I bother looking into nowadays,
there has to be a way to adapt everything to other toolchains, but I didn't bother
learning anything on this yet. I need help on this, reach out if you feel like you
could give me a hand.

Installing MSYS2 is pretty simple. [Follow their instructions](https://www.msys2.org/)

Once you're done `pacman -Syu`ing and `pacman -Su`ing your system, open a new
`MSYS2 MSYS` terminal and install the required packages :

```console
$ pacman -S \
    mingw-w64-x86_64-meson \
    mingw-w64-x86_64-cmake \
    mingw-w64-x86_64-sfml \
    mingw-w64-x86_64-boost
```
