from config_generator import ConfigGenerator
import subprocess

gen = ConfigGenerator(
    scene_dir="./resources/scenes",
    templates_dir="./resources/configs/benchmarks/templates",
    references_dir="./resources/references",
    output_dir="./resources/configs/benchmarks_to_run",
    meta_configs_dir="./resources/configs/benchmarks/meta_configs",
    scenes_file="./resources/configs/benchmarks/benchmark_scenes.yaml",
    templates_file="./resources/configs/benchmarks/benchmark_templates.yaml",
    meta_configs_file="./resources/configs/benchmarks/meta_configs.yaml",
)
gen.generate()

subprocess.run(["bash", "scripts/run_benchmark.sh"], check=True)
