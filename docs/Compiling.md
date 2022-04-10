# Compiling

In other words, how to create a new F.E.I.S. executable from the source code.

0. (If not done already) Set up you work environment by following
   [the Setup Steps](docs/Setup.md)
0. `cd` into the root of your local copy of F.E.I.S.'s source code

    ```console
    $ cd F.E.I.S./
    ```

0. Setup a build directory called `build`

    ```console
    $ meson setup build
    ```

   If you want to compile the unit tests as well, pass in `-D tests=true`.
   You can also set this option later by doing :

   ```console
   $ meson configure build -D tests=true
   ```

0. Compile in that directory

    ```console
    $ meson compile -C build
    ```

0. Setup the assets

    ```console
    $ ln -s ../assets build/assets
    ```