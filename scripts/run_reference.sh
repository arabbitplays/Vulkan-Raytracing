#!/usr/bin/env bash

cd buildDir

meson compile Edna-Engine -j 10

./Edna-Engine --config ../resources/configs/ref_config.yaml --resources ../resources -v --ref