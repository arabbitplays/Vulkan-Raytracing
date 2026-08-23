#!/usr/bin/env bash

cd buildDir

meson compile Edna-Engine -j 10 || exit 1

./Edna-Engine --config ../resources/configs/config.yaml --resources ../resources -v