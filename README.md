# libsureelec

This is a library intended to drive the Sure Electronics series of LCD
displays. Currently it will successfully drive a DE-LD023 device, but I suspect
it should work for the DE-LD021. too. It currently doesn't detect any of the
features of the device, so it will not automatically resize, but this is on the
list of things to add.

There is an example program in test.c which prints some system information to
the device.

## Things to do

* Make a proper build system
* Detect device features

