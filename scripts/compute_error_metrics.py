#!/usr/bin/env python3
"""Compute image error metrics (FLIP by default) between benchmark renders and
their highest-sample reference for the same scene.

Benchmark filenames are expected to embed a `ref_scene_<name>` token that also
appears in the reference filename, e.g.:

    benchmark:  homo_volume-similarity_mlmc_bm_1024_256_ref_scene_homo_volume.png
    reference:  262144_ref_scene_homo_volume.png

Extending with a new metric: subclass `ErrorMetric`, implement `compute()` and
register it in `METRICS`.
"""

from __future__ import annotations

import argparse
import csv
import re
import sys
from abc import ABC, abstractmethod
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

import numpy as np
from PIL import Image


REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_BENCHMARK_DIR = REPO_ROOT / "resources" / "benchmarks"
DEFAULT_REFERENCE_DIR = REPO_ROOT / "resources" / "references"
DEFAULT_ERROR_MAP_DIR = REPO_ROOT / "resources" / "error_maps"
DEFAULT_CSV_PATH = REPO_ROOT / "resources" / "benchmarks" / "error_metrics.csv"

SCENE_TOKEN_RE = re.compile(r"(ref_scene_[A-Za-z0-9_]+?)(?:\.png)?$")
REFERENCE_RE = re.compile(r"^(\d+)_(ref_scene_[A-Za-z0-9_]+)\.png$")
# Benchmark filenames have the form `{name}_bm_{biased}_{diff}_{scene}.png`.
# BenchmarkRunner's `samples` key in bm_out.csv equals biased + diff in all
# emission paths (unbiased phase, diff phase, combined mode).
BENCHMARK_NAME_RE = re.compile(r"^(.+)_bm_(\d+)_(\d+)_.+\.png$")


@dataclass
class MetricResult:
    scalar: float
    error_map: np.ndarray  # HxW float32 in [0, 1]


class ErrorMetric(ABC):
    name: str

    @abstractmethod
    def compute(self, test_rgb: np.ndarray, reference_rgb: np.ndarray) -> MetricResult:
        """test_rgb, reference_rgb: HxWx3 float32 in [0, 1]. Returns scalar and per-pixel map."""


class FlipMetric(ErrorMetric):
    name = "flip"

    def __init__(self) -> None:
        try:
            import flip_evaluator  # type: ignore
        except ImportError as exc:
            raise RuntimeError(
                "flip_evaluator is not installed. Install with `pip install flip-evaluator`."
            ) from exc
        self._flip = flip_evaluator

    def compute(self, test_rgb: np.ndarray, reference_rgb: np.ndarray) -> MetricResult:
        # flip_evaluator.evaluate expects HxWx3 float32 arrays in [0, 1] or file paths.
        error_map, mean_error, _params = self._flip.evaluate(
            reference_rgb, test_rgb, "LDR"
        )
        error_map = np.asarray(error_map, dtype=np.float32)
        if error_map.ndim == 3:
            error_map = error_map[..., 0]
        return MetricResult(scalar=float(mean_error), error_map=error_map)


METRICS: dict[str, type[ErrorMetric]] = {
    FlipMetric.name: FlipMetric,
}


def extract_scene_token(filename: str) -> str | None:
    match = SCENE_TOKEN_RE.search(filename)
    return match.group(1) if match else None


def index_references(reference_dir: Path) -> dict[str, Path]:
    """Map scene token -> path of the reference with the highest sample count."""
    best: dict[str, tuple[int, Path]] = {}
    for path in sorted(reference_dir.glob("*.png")):
        match = REFERENCE_RE.match(path.name)
        if not match:
            continue
        samples = int(match.group(1))
        scene = match.group(2)
        current = best.get(scene)
        if current is None or samples > current[0]:
            best[scene] = (samples, path)
    return {scene: path for scene, (_, path) in best.items()}


def load_rgb(path: Path) -> np.ndarray:
    img = Image.open(path).convert("RGB")
    arr = np.asarray(img, dtype=np.float32) / 255.0
    return arr


def save_error_map(error_map: np.ndarray, output_path: Path) -> None:
    clipped = np.clip(error_map, 0.0, 1.0)
    try:
        import matplotlib  # type: ignore

        colored = matplotlib.colormaps["magma"](clipped)[..., :3]
        arr = (colored * 255.0).astype(np.uint8)
    except ImportError:
        arr = (clipped * 255.0).astype(np.uint8)
    Image.fromarray(arr).save(output_path)


def iter_benchmark_images(benchmark_dir: Path) -> Iterable[Path]:
    return sorted(p for p in benchmark_dir.glob("*.png") if p.is_file())


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--benchmark-dir", type=Path, default=DEFAULT_BENCHMARK_DIR)
    parser.add_argument("--reference-dir", type=Path, default=DEFAULT_REFERENCE_DIR)
    parser.add_argument("--error-map-dir", type=Path, default=DEFAULT_ERROR_MAP_DIR)
    parser.add_argument("--csv", type=Path, default=DEFAULT_CSV_PATH)
    parser.add_argument(
        "--merge-into",
        type=Path,
        default=None,
        help=(
            "Optional path to a bm_out-style CSV. If provided, adds a `flip` "
            "column keyed on (name, samples = biased + diff) parsed from the "
            "benchmark image filenames."
        ),
    )
    parser.add_argument(
        "--metrics",
        nargs="+",
        default=[FlipMetric.name],
        choices=sorted(METRICS.keys()),
        help="Error metrics to compute (default: flip).",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    if args.error_map_dir.exists():
        for existing in args.error_map_dir.iterdir():
            if existing.is_file() or existing.is_symlink():
                existing.unlink()
    args.error_map_dir.mkdir(parents=True, exist_ok=True)
    args.csv.parent.mkdir(parents=True, exist_ok=True)
    args.csv.unlink(missing_ok=True)

    references = index_references(args.reference_dir)
    if not references:
        print(f"No references matching pattern found in {args.reference_dir}", file=sys.stderr)
        return 1

    metrics = [METRICS[name]() for name in args.metrics]

    rows: list[dict[str, object]] = []
    for bench_path in iter_benchmark_images(args.benchmark_dir):
        scene = extract_scene_token(bench_path.name)
        if scene is None:
            print(f"[skip] no scene token in {bench_path.name}")
            continue
        ref_path = references.get(scene)
        if ref_path is None:
            print(f"[skip] no reference for scene '{scene}' ({bench_path.name})")
            continue

        print(f"[match] {bench_path.stem} -> {ref_path.stem}")

        test_rgb = load_rgb(bench_path)
        ref_rgb = load_rgb(ref_path)
        if test_rgb.shape != ref_rgb.shape:
            print(
                f"[skip] shape mismatch: {bench_path.name} {test_rgb.shape} vs "
                f"{ref_path.name} {ref_rgb.shape}"
            )
            continue

        row: dict[str, object] = {
            "benchmark": bench_path.name,
            "reference": ref_path.name,
            "scene": scene,
        }
        for metric in metrics:
            result = metric.compute(test_rgb, ref_rgb)
            row[f"{metric.name}_mean"] = result.scalar
            map_path = args.error_map_dir / f"{bench_path.stem}__{metric.name}.png"
            save_error_map(result.error_map, map_path)
            print(f"       {metric.name}: {result.scalar:.6f}  map -> {map_path}")
        rows.append(row)

    if not rows:
        print("No benchmark/reference pairs processed.", file=sys.stderr)
        return 1

    fieldnames = list(rows[0].keys())
    with args.csv.open("w", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)
    print(f"\nWrote {len(rows)} rows -> {args.csv}")

    if args.merge_into is not None:
        merge_flip_into_bm_csv(args.merge_into, rows)
    return 0


def merge_flip_into_bm_csv(bm_path: Path, flip_rows: list[dict[str, object]]) -> None:
    """Add a `flip` column to a bm_out-style CSV by parsing the benchmark filename.

    Match key is (name, samples) where `name` comes from the prefix before
    `_bm_` and `samples` equals biased + diff from the filename numeric parts.
    """
    if not bm_path.exists():
        print(f"[merge] skip: {bm_path} does not exist", file=sys.stderr)
        return

    flip_lookup: dict[tuple[str, int], object] = {}
    for row in flip_rows:
        benchmark = str(row.get("benchmark", ""))
        flip_mean = row.get("flip_mean")
        if flip_mean is None:
            continue
        match = BENCHMARK_NAME_RE.match(benchmark)
        if not match:
            continue
        key = (match.group(1), int(match.group(2)) + int(match.group(3)))
        flip_lookup[key] = flip_mean

    with bm_path.open(newline="") as fh:
        reader = csv.DictReader(fh)
        fieldnames = list(reader.fieldnames or [])
        merged_rows = list(reader)

    if "flip" not in fieldnames:
        fieldnames.append("flip")

    for row in merged_rows:
        try:
            key = (row["name"], int(row["samples"]))
        except (KeyError, ValueError):
            row["flip"] = ""
            continue
        value = flip_lookup.get(key)
        row["flip"] = "" if value is None else value

    with bm_path.open("w", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(merged_rows)
    print(f"[merge] added flip column to {bm_path}")


if __name__ == "__main__":
    raise SystemExit(main())
