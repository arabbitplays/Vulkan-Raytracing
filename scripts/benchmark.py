import subprocess
from PIL import Image
import numpy as np
import sys
import matplotlib.pyplot as plt
import flip_evaluator as flip

class Tee:
    def __init__(self, file):
        self.file = file
        self.stdout = sys.stdout  # Save original stdout

    def write(self, message):
        self.stdout.write(message)  # Print to console
        self.file.write(message)  # Write to file

    def flush(self):  # Ensure proper flushing for real-time output
        self.stdout.flush()
        self.file.flush()

def run_program(program, args):
    try:
        result = subprocess.run([program, *args], capture_output=True, text=True)
        print("Output:")
        print(result.stdout)
        print("Error:")
        print(result.stderr)
        print("Exit Code:", result.returncode)
    except FileNotFoundError:
        print(f"Error: Program '{program}' not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

def mse_images(image_path1, image_path2):
    """
    Compute the Mean Squared Error (MSE) between two images.

    :param image_path1: Path to the first image.
    :param image_path2: Path to the second image.
    :return: Mean Squared Error (float).
    """
    img1 = Image.open(image_path1).convert("L")  # Convert to grayscale
    img2 = Image.open(image_path2).convert("L")  # Convert to grayscale

    if img1.size != img2.size:
        raise ValueError("Images must have the same dimensions for MSE calculation.")

    img1_array = np.array(img1, dtype=np.float32)
    img2_array = np.array(img2, dtype=np.float32)

    mse = np.mean((img1_array - img2_array) ** 2)

    flipErrorMap, meanFLIPError, parameters = flip.evaluate(image_path1, image_path2, "LDR")
    result = {"mse" : mse, "flip": round(meanFLIPError, 6)}

    return result

def plot_difference(image_path1, image_path2):
    flipErrorMap, meanFLIPError, parameters = flip.evaluate(image_path1, image_path2, "LDR")

    plt.figure(figsize=(8, 6))
    plt.imshow(flipErrorMap, cmap='hot', interpolation='nearest')
    plt.colorbar(label='Difference Intensity')
    plt.title('Image Difference Heatmap')
    plt.axis('off')
    plt.savefig("heatmap.png")  # Save instead of show

def run_benchmark(program, reference_dir, reference_name, sample_counts):
    output_path = reference_dir + "/benchmark"

    with open(output_path + "/log.txt", "w") as f:
        tee = Tee(f)
        sys.stdout = tee  # Redirect stdout to both file and console

        results = []
        for sample_count in sample_counts:
            args = [
                "--output", output_path,
                "--samples", str(sample_count),
                "--resources", "resources",
                "--reference_scene", reference_dir + "/" + "scene.yaml"]
            run_program(build_path, args)

            results.append(mse_images(reference_dir + "/" + reference_name, output_path + "/" + str(sample_count) + "_ref.png"))

        plot_difference(reference_dir + "/" + reference_name, output_path + "/" + str(sample_counts[-1]) + "_ref.png")

        print("\n" + "-" * 50 + " RESULTS: " + "-" * 50 + "\n")
        for i in range(len(results)):
            print(str(sample_counts[i]) + " samples; MSE: " + str(results[i]["mse"]) + "; Flip: " + str(results[i]["flip"]))

        sys.stdout = sys.__stdout__  # Restore stdout

build_path = "buildDir/renderer"
reference_dir = "resources/references/big_light_cornell"
reference_name = "100000_ref_sphere.png"

run_benchmark(build_path, reference_dir, reference_name, [1000])