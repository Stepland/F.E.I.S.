# How to compile

I can never remember this so here we go :

0. Install [meson](https://mesonbuild.com/Running-Meson.html)
0. Install SFML

    - Debian / Ubuntu / Mint

        ```console
        sudo apt install libsfml-dev
        ```

0. `cd` into the root of your local copy of F.E.I.S.'s source code

    ```console
    $ cd F.E.I.S./
    ```

0. Setup `build` directory

    ```console
    $ meson setup build
    ```

0. Build

    ```console
    $ meson compile -C build
    ```

0. Setup assets

    ```console
    $ ln -s ../assets build/assets
    ```