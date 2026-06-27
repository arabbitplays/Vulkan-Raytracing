#!/usr/bin/env bash

cd buildDir

meson compile renderer -j 10 || exit 1

./renderer --config ../resources/configs/config.yaml --resources ../resources -v