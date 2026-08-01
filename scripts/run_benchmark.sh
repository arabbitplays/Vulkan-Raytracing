#!/usr/bin/env bash

get_yaml_files() {
    local dir="${1:-.}"
    local yaml_files=()

    for file in "$dir"/*; do
        [ -f "$file" ] || continue

        case "$file" in
            *.yaml|*.yml)
                yaml_files+=("$file")
                ;;
        esac
    done

    printf '%s\n' "${yaml_files[@]}"
}

get_csv_files() {
    local dir="${1:-.}"
    local csv_files=()

    for file in "$dir"/*; do
        [ -f "$file" ] || continue

        case "$file" in
            *.csv)
                csv_files+=("$file")
                ;;
        esac
    done

    printf '%s\n' "${csv_files[@]}"
}

bm_configs=$(get_yaml_files "./resources/configs/benchmarks_to_run")
build_dir="./buildDir"
out_dir="./resources/benchmarks"
tmp_dir="$build_dir/tmp"
csv_out_path="$out_dir/bm_out.csv"
flip_csv_out_path="$out_dir/flip_out.csv"

# remove all existing csv files
csv_files=$(get_csv_files "$out_dir")
for file in $csv_files; do
  rm -fv "$file"
done

# remove leftover preview images from the last run
rm -fv "$out_dir"/*.png 2>/dev/null
rm -fv "$tmp_dir"/*.png 2>/dev/null

# compile the project
(
  cd "$build_dir"
  if ! meson compile Edna-Engine -j 10; then
      echo "Build failed"
      exit 1
  fi
) || exit 1

# run benchmarks
(
  cd "$build_dir"
  for config in $bm_configs; do
    echo ""
    echo "Running benchmark with $config"
    ./Edna-Engine --config ../$config --resources ../resources --benchmark

    # Move the final preview image left in tmp into the benchmarks folder,
    # tagged with the benchmark config name so it survives the next run.
    bm_name=$(basename "$config")
    bm_name="${bm_name%.*}"
    for img in tmp/*.png; do
      [ -f "$img" ] || continue
      mv -v "$img" "../$out_dir/${bm_name}_$(basename "$img")"
    done
  done
)

# Store everything in the out csv
first=1
csv_files=$(get_csv_files "$out_dir")
for file in $csv_files; do
  if [ "$first" -eq 1 ]; then
    cat "$file" > "$csv_out_path"
    first=0
  else
    # skip header row
    tail -n +2 "$file" >> "$csv_out_path"
  fi
  rm -fv "$file"
done

python3 ./scripts/compute_error_metrics.py --csv "$flip_csv_out_path" --merge-into "$csv_out_path"

Rscript ./scripts/generate_plots.R $csv_out_path $out_dir/bm_plots.pdf