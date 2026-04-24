#!/usr/bin/env bash

cd buildDir

meson compile renderer -j 10

./renderer --config ../resources/configs/bm_config.yaml --resources ../resources --benchmark

Rscript ../scripts/generate_plots.R ../resources/benchmarks/bm_out.csv ../resources/benchmarks/bm_plots.pdf