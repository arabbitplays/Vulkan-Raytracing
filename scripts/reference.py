import sys
import subprocess
from PIL import Image
import numpy as np
import os
import time


def run_program(program, args):
    try:
        result = subprocess.run([program, *args], capture_output=True, text=True)
        if (result.returncode != 0):
            print("Error:")
            print(result.stderr)
            print("Exit Code:", result.returncode)
    except FileNotFoundError:
        print(f"Error: Program '{program}' not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

def render_images(program, samples_per_image, image_count, reference_dir, output_dir):
    for i in range(0, image_count):
        start_time = time.time()  # Record the start time

        args = [
        "--output", output_dir,
        "--samples", str(samples_per_image),
        "--resources", "resources",
        "--reference_scene", reference_dir + "/" + "ExtraCornell.yaml"]
        run_program(program, args)
        os.rename(output_dir + "/" + str(samples_per_image) + "_ref.png", output_dir + "/" + str(i) + ".png")

        end_time = time.time()
        execution_time = end_time - start_time
        time_left = execution_time * (image_count - i - 1)

        hours = int(time_left // 3600)
        minutes = int((time_left % 3600) // 60)
        seconds = int(time_left % 60)

        print(f"Current image count: {i+1}, progress: {((i+1)/image_count):.2f}%, estimated time remaining: {hours}h {minutes}m {seconds}s")

def calculate_final_image(image_count, output_dir, reference_name):
    images = load_images_to_numpy(image_count, output_dir)

    while len(images) > 1:
        average_image_pairs(images)
        print("Averaging images... " + str(len(images)) + " images left")

    save_image_from_array(images[0], output_dir + "/" + reference_name + ".png")

def load_images_to_numpy(image_count, output_dir):
    images = []
    for i in range(image_count):
        filename = output_dir + f"/{i}.png"
        try:
            img = Image.open(filename)
            img_array = np.array(img)
            images.append(img_array)
        except FileNotFoundError:
            print(f"Warning: {filename} not found.")
    return images

"""Halves the list by averaging image pairs. If odd, duplicates the last image."""
def average_image_pairs(image_list):
    """Averages adjacent image pairs in-place. If odd number of images, average the last one with itself."""
    length = len(image_list) // 2  # Number of full pairs
    for i in range(length):
        if i == length - 1 and len(image_list) % 2 == 1:
            image_list[i] = (image_list[2 * i].astype(np.float32) + image_list[2 * i + 1].astype(np.float32) + image_list[2 * i + 2].astype(np.float32)) / 3
        else:
            image_list[i] = (image_list[2 * i].astype(np.float32) + image_list[2 * i + 1].astype(np.float32)) / 2
        image_list[i] = image_list[i].astype(np.uint8)  # Convert back to uint8 after averaging

    del image_list[length:]

def save_image_from_array(image_array, filename):
    img = Image.fromarray(image_array)  # Convert NumPy array to Pillow image
    img.save(filename)  # Save as an image file
    print(f"Image saved as {filename}")

def delete_images(image_count, output_dir):
    for i in range(image_count):
        filename = output_dir + f"/{i}.png"
        try:
            os.remove(filename)  # Delete the file
            print(f"Deleted {filename}")
        except FileNotFoundError:
            print(f"Warning: {filename} not found.")

def render_reference(program, total_samples, samples_per_image, reference_dir, output_dir):
    assert(total_samples % samples_per_image == 0)

    image_count = int(total_samples / samples_per_image)
    render_images(program, samples_per_image, image_count, reference_dir, output_dir)

    calculate_final_image(image_count, output_dir, str(total_samples) + "_ref")
    delete_images(image_count, output_dir)

build_path = "buildDir/renderer"
reference_dir = "resources/references/simple_cornell"
output_dir = "tmp"
render_reference(build_path, 100000, 5000, reference_dir, output_dir)
