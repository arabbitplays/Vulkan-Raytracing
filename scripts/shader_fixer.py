from pathlib import Path
import re

def collect_glsl_files(root_dir):
    root = Path(root_dir)
    return [str(p) for p in root.rglob("*.glsl") if p.is_file()]

def load_glsl_file_contents(paths, as_dict=True, encoding="utf-8"):
    if as_dict:
        contents = {}
        for p in paths:
            p = Path(p)
            try:
                contents[str(p)] = p.read_text(encoding=encoding)
            except Exception as e:
                contents[str(p)] = None  # or handle/log error
        return contents
    else:
        contents = []
        for p in paths:
            p = Path(p)
            try:
                contents.append(p.read_text(encoding=encoding))
            except Exception:
                contents.append(None)
        return contents

def extract_ifndef_identifier(content):
    pattern = re.compile(r'^\s*#ifndef\s+([A-Za-z_][A-Za-z0-9_]*)', re.MULTILINE)
    match = pattern.search(content)
    return match.group(1) if match else None

def get_expected_identifier(file_path):
    path = Path(file_path)
    stem = path.stem
    normalized = "".join(c if c.isalnum() else "_" for c in stem).upper()
    expected = f"{normalized}_GLSL"
    return expected

def write_content_to_file(file_path, content, encoding="utf-8"):
    path = Path(file_path)
    path.write_text(content, encoding=encoding)

def add_include_guard(file_path, content):
    identifier = get_expected_identifier(file_path)

    header = f"#ifndef {identifier}\n#define {identifier}\n"
    footer = f"\n#endif // {identifier}\n"

    content = header + content.lstrip() + footer
    write_content_to_file(file_path, content)

    print("Fixed " + file_path)

def edit_include_guard(file_path, content):
    print("lol")

if __name__ == "__main__":
    directory = "./shaders"
    glsl_files = collect_glsl_files(directory)

    data = load_glsl_file_contents(glsl_files)

    for path, content in data.items():
        identifier = extract_ifndef_identifier(content)
        expected_identifier = get_expected_identifier(path)

        if identifier == expected_identifier:
            continue;
            
        if identifier == None:
            add_include_guard(path, content)
            continue;

        print(f"Wrong identifier found: {path} - {identifier}")

