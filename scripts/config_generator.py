from pathlib import Path
import yaml
import shutil
import re

class ConfigGenerator:
    def __init__(
        self,
        scene_dir,
        templates_dir,
        references_dir,
        output_dir,
        meta_configs_dir,
        scenes_file,
        templates_file,
        meta_configs_file,
    ):
        self.scene_dir = Path(scene_dir)
        self.templates_dir = Path(templates_dir)
        self.references_dir = Path(references_dir)
        self.output_dir = Path(output_dir)
        self.meta_configs_dir = Path(meta_configs_dir)
        self.scenes_file = Path(scenes_file)
        self.templates_file = Path(templates_file)
        self.meta_configs_file = Path(meta_configs_file)

    # ---------- setup ----------

    def find_best_reference_sample_count(self, scene):
        base = self.strip_yaml(scene)

        pattern = re.compile(rf"^(\d+)_({re.escape(base)})\.png$")

        max_num = None

        for file in self.references_dir.iterdir():
            if not file.is_file():
                continue

            match = pattern.match(file.name)
            if match:
                num = int(match.group(1))
                max_num = num if max_num is None else max(max_num, num)

        if max_num is None:
            raise Exception("No reference found for scene " + base)
        return max_num

    # ---------- IO ----------

    @staticmethod
    def load_text_file(path):
        return Path(path).read_text(encoding="utf-8")

    @staticmethod
    def load_yaml_file(path):
        with open(Path(path), "r", encoding="utf-8") as f:
            return yaml.safe_load(f) or {}

    @staticmethod
    def load_yaml_list(path):
        with open(Path(path), "r", encoding="utf-8") as f:
            data = yaml.safe_load(f) or []
        if not isinstance(data, list):
            raise ValueError(f"{path} must contain a YAML list")
        return data

    def write_config(self, name, content):
        self.output_dir.mkdir(parents=True, exist_ok=True)
        (self.output_dir / f"{name}.yaml").write_text(content, encoding="utf-8")

    def clear_output(self):
        if not self.output_dir.exists():
            return

        for item in self.output_dir.iterdir():
            if item.is_dir():
                shutil.rmtree(item)
            else:
                item.unlink()

    # ---------- name utils ----------

    @staticmethod
    def strip_scene_name(scenes):
        prefix = "ref_scene_"
        suffix = ".yaml"

        if isinstance(scenes, str):
            return Path(scenes).name.removeprefix(prefix).removesuffix(suffix)

        return [
            Path(f).name.removeprefix(prefix).removesuffix(suffix)
            for f in scenes
        ]

    @staticmethod
    def strip_yaml(files):
        suffix = ".yaml"

        if isinstance(files, str):
            return Path(files).name.removesuffix(suffix)

        return [
            Path(f).name.removesuffix(suffix)
            for f in files
        ]

    def get_config_name(self, scene, template, meta_config):
        return (
            f"{self.strip_scene_name(scene)}"
            f"_{self.strip_yaml(template)}"
            f"_{self.strip_yaml(meta_config)}"
        )

    # ---------- templating ----------

    @staticmethod
    def replace_placeholder(content, key, value):
        return content.replace(f"[[{key}]]", str(value))

    # ---------- main generate ----------

    def generate(self):
        self.clear_output()

        scenes = self.load_yaml_list(self.scenes_file)
        templates = self.load_yaml_list(self.templates_file)
        meta_configs = self.load_yaml_list(self.meta_configs_file)

        for template in templates:
            base = self.load_text_file(self.templates_dir / template)

            for scene in scenes:
                expected_samples = self.find_best_reference_sample_count(scene)

                for meta_config in meta_configs:
                    meta_values = self.load_yaml_file(
                        self.meta_configs_dir / meta_config
                    )

                    name = self.get_config_name(scene, template, meta_config)
                    config = base
                    config = self.replace_placeholder(
                        config, "SCENE_NAME", self.strip_yaml(scene)
                    )
                    config = self.replace_placeholder(config, "NAME", name)
                    config = self.replace_placeholder(
                        config, "EXPECTED_SAMPLES", expected_samples
                    )

                    for k, v in meta_values.items():
                        config = self.replace_placeholder(config, k, v)

                    self.write_config(name, config)
                    print(f"Generated {name}")
