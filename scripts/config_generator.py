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
        meta_config_path,
    ):
        self.scene_dir = Path(scene_dir)
        self.templates_dir = Path(templates_dir)
        self.references_dir = Path(references_dir)
        self.output_dir = Path(output_dir)
        self.meta_config_path = Path(meta_config_path)

        self.selected_scenes = []
        self.selected_templates = []

    # ---------- setup ----------

    def get_ref_scenes(self):
        return [
            file.name
            for file in Path(self.scene_dir).iterdir()
            if file.is_file() and file.name.startswith("ref_")
        ]

    def get_config_templates(self):
        return [
            file.name
            for file in Path(self.templates_dir).iterdir()
            if file.is_file()
        ]

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

    # ---------- user input ----------

    @staticmethod
    def multi_select(options, displayed_options):
        if not options:
            return []

        print("Select one or more items (comma-separated):")
        for i, option in enumerate(displayed_options, start=1):
            print(f"{i}. {option}")

        while True:
            try:
                selection = input("\nSelection: ").strip()

                indices = {
                    int(x.strip()) - 1
                    for x in selection.split(",")
                    if x.strip()
                }

                if not all(0 <= i < len(options) for i in indices):
                    raise ValueError

                print("")

                return [options[i] for i in sorted(indices)]

            except ValueError:
                print("Invalid input. Example: 1,3,5")


    # ---------- IO ----------

    @staticmethod
    def load_text_file(path):
        return Path(path).read_text(encoding="utf-8")

    @staticmethod
    def load_yaml_file(path):
        with open(Path(path), "r", encoding="utf-8") as f:
            return yaml.safe_load(f) or {}

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

    def get_config_name(self, scene, template):
        return f"{self.strip_scene_name(scene)}-{self.strip_yaml(template)}"

    # ---------- templating ----------

    @staticmethod
    def replace_placeholder(content, key, value):
        return content.replace(f"[[{key}]]", str(value))

    # ---------- main generate ----------

    def build_configs(self):
        self.clear_output()

        meta_config = self.load_yaml_file(self.meta_config_path)

        for template in self.selected_templates:
            base = self.load_text_file(self.templates_dir / template)

            for scene in self.selected_scenes:
                config = base
                name = self.get_config_name(scene, template)

                config = self.replace_placeholder(
                    config, "SCENE_NAME", self.strip_yaml(scene)
                )
                config = self.replace_placeholder(config, "NAME", name)
                config = self.replace_placeholder(config, "EXPECTED_SAMPLES", self.find_best_reference_sample_count(scene))

                for k, v in meta_config.items():
                    config = self.replace_placeholder(config, k, v)

                self.write_config(name, config)
                print(f"Generated {name}")

    def generate(self):
        scenes = self.get_ref_scenes()
        self.selected_scenes = self.multi_select(scenes, self.strip_scene_name(scenes))

        templates = self.get_config_templates()
        self.selected_templates = self.multi_select(templates, self.strip_yaml(templates))

        self.build_configs()