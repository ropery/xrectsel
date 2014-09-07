xrectsel
========

`xrectsel` is a small program that tells you the geometry of a rectangular
screen region which you have selected by dragging the mouse / pointer.

When arguments are passed to `xrectsel`, a limited set of `%` format strings
are recognized and substituted. Look at the source to see what are supported.

`xrectsel` requires `libX11` to build.

To build and install from source,

    ./bootstrap  # required if ./configure is not present
    ./configure --prefix /usr
    make
    make DESTDIR="$directory" install
