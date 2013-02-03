Building libsol
===============
Building libsol only requires a C compiler (preferably Clang or GCC) and a
single dependency.

Requirements
------------
To build libsol, you'll need to have the following installed on your system:

* [libYAML](http://pyyaml.org/wiki/LibYAML) - a C YAML parser

Building
--------
To build libsol, cd into the project's folder and run the following commands:

    make
    make install

You should now have a successfully built sol runtime installed in your /usr/lib
directory.

Compiling Sol Programs
======================
In order to compile a sol program, you will need to have generated a C source
file from a standard sol source file. This can be done with the solc utility.
This will output a .c file, which can then be compiled with the following
command (using Clang):

    clang -lsol -lyaml -o my-program my-program.c

This should generate a standard executable which can be run view the command
line as usual.

Developing libsol
=================
The libsol project is developed in the NetBeans IDE (with the appropriate C/C++
modules installed). It is recommended that you view and edit the project in
NetBeans as well until a better development environment is set up.
