from config_generator import ConfigGenerator
import subprocess

gen = ConfigGenerator(
    "./resources/scenes",
    "./resources/configs/benchmarks/templates",
    "./resources/references",
    "./resources/configs/benchmarks_to_run",
    "./resources/configs/benchmarks/meta_config.yaml",
)
gen.generate()

subprocess.run(["bash", "scripts/run_benchmark.sh"], check=True)