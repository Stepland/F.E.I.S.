# How to make releases

## Git

1. Add everything you want in the release to the `main` branch
1. Use the `utils/bump_version.py` script to bump the version
    - `$ python utils/bump_version.py {version}`
1. Push the commit and the tag

## Release archives

### Windows (MSYS2)

1. Pull the release tag
1. Follow the [compilation instructions](Compiling.md)
1. Open an MSYS2 UCRT64 terminal
1. `cd` into FEIS's source code root
1. Use the release making script

    For a regular semver release

    ```console
    $ python utils/make_windows_release.py
    ```

    For preview builds

    ```console
    $ python utils/make_windows_release.py --timestamp
    ```
1. Distribute the generated `.zip` file

### Debian

1. Pull the release tag
1. Follow the [compilation instructions](Compiling.md)
1. `cd` into the `packaging/debian` folder
1. Run `./build_deb.sh` :

    ```console
    $ ./build_deb.sh (executable) (assets) (icon)
    ```

    `executable`: Path to the FEIS executable you just build, probably named 'FEIS'
    
    `assets`: Path to the assets folder to ship with this release

    `icon`: Path to the app icon

    a typical, fully specified command looks like this :

    ```console
    $ ./build_deb.sh ../../build_release/FEIS ../../assets/ ../../images/feis\ icon.svg
    ```
1. Distribute the generated `.deb` file, found in the `packaging/debian/package` folder
