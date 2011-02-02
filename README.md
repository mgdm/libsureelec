# libsureelec

This is a library intended to drive the Sure Electronics series of LCD
displays. Currently it will successfully drive a DE-LD023 device, but I suspect
it should work for the DE-LD021. too. It ought to detect the available device
features.

It uses waf as the build system. To install:
<pre>
./waf configure
./waf build
./waf install # as root, probably
</pre>

There is an example program in test.c which prints some system information to
the device, which will be compiled as build/sureelec_test. To run, use a
command like:

<pre>
./build/sureelec_test /dev/ttyUSB0
</pre>

