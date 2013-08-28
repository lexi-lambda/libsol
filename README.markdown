Building libsol
===============
Building libsol only requires a C compiler (preferably Clang or GCC) and a
few dependencies.

Requirements
------------
To build libsol, you'll need to have the following installed on your system:

* [libYAML](http://pyyaml.org/wiki/LibYAML) - a C YAML parser
* [libuv](https://github.com/joyent/libuv) - a C event library, originally designed for use in Node.js

If you're running Debian, you can get libYAML from the `lib-yaml-0-2` and `lib-yaml-dev`
packages. libuv does not currently distribute packages, so it will need to be built
from source (see the libuv readme).

Building
--------
The libsol project can be built using [CMake](http://www.cmake.org/). It is
installed to the /usr/local directories. Currently, most Unix-based operating
systems should be supported, including Linux and Mac OS X.

It is recommended that you keep all build files in a subdirectory to avoid
cluttering the repository. The `build` folder is automatically ignored
in the .gitignore file, so you could build the project using GNU Make like this:

    cd libsol
    mkdir build
    cd build
    cmake ..
    make
    sudo make install

Alternatively, you can specify other generators to use other build systems. For
example, you could generate an Xcode project with the following command:

    cmake -G Xcode ..

Compiling Sol Programs
======================
In order to compile a sol program, you will need to have generated a C source
file from a standard sol source file. This can be done with the solc utility.
This will output a .c file, which can then be compiled with the following
command (using Clang):

    clang -lsol -o my-program my-program.c

This should generate a standard executable which can be run view the command
line as usual.
