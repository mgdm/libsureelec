#!/bin/sh

aclocal --install -I m4 &&
	autoreconf --force --install &&
	./configure "$@"

