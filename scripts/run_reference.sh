#!/usr/bin/env bash

cd buildDir

meson compile renderer -j 10

./renderer --config ../resources/configs/ref_config.yaml --resources ../resources -v --ref