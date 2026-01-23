from PIL import Image
import numpy as np

def unorm_to_srgb(image_array):
    """ Convert an image from UNORM (linear) to sRGB using gamma correction (γ = 2.2). """
    gamma = 1 / 2.2  # sRGB gamma correction
    return np.power(image_array, gamma)  # Apply gamma correction

def load_image(image_path):
    """ Load an image as a float32 UNORM (linear space) normalized to [0, 1]. """
    image = Image.open(image_path).convert("RGB")  # Load as RGB
    image_array = np.asarray(image, dtype=np.float32) / 255.0  # Normalize to [0,1]
    return image_array

def save_image(image_array, output_path):
    """ Save the image as an 8-bit sRGB PNG. """
    image_array = np.clip(image_array * 255, 0, 255).astype(np.uint8)  # Convert back to 8-bit
    image = Image.fromarray(image_array)
    image.save(output_path)

folder = "tmp/"
unorm_image = load_image(folder + "in.png")
srgb_image = unorm_to_srgb(unorm_image)
save_image(srgb_image, folder + "out.png")
