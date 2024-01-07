# How to make releases

## Git

1. Add everything you want in the release to the `main` branch
1. Use the `utils/bump_version.py` script to bump the version
    - `$ python utils/bump_version.py {version}`
1. Push the commit and the tag

## Release archives

### Windows (MSYS2)

1. Pull the latest version of the code
1. Follow the [compilation instructions](docs/Compiling.md)
1. Open an MSYS2 x64 terminal
1. `cd` into FEIS's source code root
1. Use the release making script

    For a regular semver release

    ```console
    python utils/make_windows_release.py --release-version 2.x.x
    ```

    For preview builds

    ```console
    python utils/make_windows.release.py --release-version 2.x.x-alpha --timestamp
    ```
1. Distribute the generated `.zip` file

### Debian

