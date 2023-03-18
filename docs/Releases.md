# How to make releases

## Windows (MSYS2)

0. Pull the latest version of the code
0. Follow the [compilation instructions](Compiling.md)
0. Open an MSYS2 x64 terminal
0. `cd` into FEIS's source code root
0. Use the release making script

    For a regular semver release

    ```console
    python utils/make_windows.release.py 2.x.x
    ```

    For preview builds

    ```console
    python utils/make_windows.release.py 2.x.x-alpha --timestamp
    ```
0. Distribute the generated `.zip` file