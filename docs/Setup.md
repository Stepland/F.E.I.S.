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

*To be filled in when I figure it out*