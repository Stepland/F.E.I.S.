# Setting up the tools you need to work with F.E.I.S.'s code

## Debian-ish Linux

### I just want to compile

Install the big general "I'm a real C/C++ dev !" package.

```console
$ sudo apt install build-essential
```

If this only gave you gcc version 9 or older, you need to install 10 : follow
[these instructions](https://web.archive.org/web/20220317024657/https://ahelpme.com/linux/ubuntu/install-and-make-gnu-gcc-10-default-in-ubuntu-20-04-focal/).

Install meson (the build system). I recommend doing so via python's `pip` to
get a more up-to-date version than what your distro packages might have

```console
$ pip install meson
```

Unfortunately this also means meson will not come with ninja so we need to
install it ourselves :

```console
$ sudo apt install ninja-build
```

Install dependencies

```console
sudo apt install libsfml-dev libopenal-dev libgmp-dev libaubio-dev libfftw3-dev
```

Then checkout [this page](Compiling.md) for instructions on how to compile

### I also want to contribute some code

Install `clang-format`

```console
$ sudo apt install clang-format
```

## Windows

### I just want to compile

#### MSYS2

MSYS2 is not the *usual* way to compile things for windows but it's the only
thing I know for now. If you know better, by all means, do what you think is
best (and also please share some of your knowledge with me, I absolutely *suck*
at build systems and would be delighted to learn from an expert)

Installing MSYS2 is pretty simple. [Follow their instructions](https://www.msys2.org/)

Once you're done `pacman -Syu`ing and `pacman -Su`ing your system, open a new
`MSYS2 UCRT64` terminal and install the required packages :

```console
$ pacman -S \
    mingw-w64-ucrt-x86_64-meson \
    mingw-w64-ucrt-x86_64-cmake \
    mingw-w64-ucrt-x86_64-sfml \
    mingw-w64-ucrt-x86_64-ntldd-git \
	mingw-w64-ucrt-x86_64-aubio \
	mingw-w64-ucrt-x86_64-eigen3 \
	mingw-w64-ucrt-x86_64-fftw \
	mingw-w64-ucrt-x86_64-mpdecimal
```

Once this is done, open a new `MSYS2 UCRT64` terminal and follow the
[compilation instructions](Compiling.md)
