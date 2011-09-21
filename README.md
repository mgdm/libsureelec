# libsureelec

This is a library intended to drive the Sure Electronics series of LCD
displays. Currently it will successfully drive a DE-LD023 device, but I suspect
it should work for the DE-LD021. too. It ought to detect the available device
features.

It uses waf as the build system. To install:

    ./waf configure 
    ./waf build
    ./waf install # as root, probably

There is an example program in sureelec_test.c which prints some system information
to the device. This will be compiled as build/sureelec_test. To run, use a
command like:

    ./build/sureelec_test /dev/ttyUSB0

There's another example in sureelec_test2.c which accepts input using a readline
interface, which will then be sent straight to the device. It is run similarly:

    ./build/sureelec_test2 /dev/ttyUSB0