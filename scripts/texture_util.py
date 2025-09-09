from PIL import Image
import numpy as np
import os

def print_image_pixels(image_path):
    # Open the image file
    img = Image.open(image_path)
    # Convert the image to RGB format if not already in that mode
    img = img.convert('RGB')
    # Convert the image to a NumPy array
    pixel_data = np.array(img)

    # Iterate over each pixel and print the RGB values
    for y in range(pixel_data.shape[0]):
        for x in range(pixel_data.shape[1]):
            pixel = pixel_data[y, x]
            pixel = pixel / 255 * 2 - 1;
            print(f"Pixel at ({x}, {y}): {pixel}")


def combine_rgb_channels(r_path, g_path, b_path, output_path):
    # Open the three images
    img_r = Image.open(r_path).convert('RGB')
    r_data = np.array(img_r)

    img_g = Image.open(g_path).convert('RGB')
    g_data = np.array(img_g)

    if os.path.exists(b_path):
        img_b = Image.open(b_path).convert('RGB')
        b_data = np.array(img_b)
    else:
        b_data = np.full_like(r_data, 1, dtype=np.uint8)


    # Ensure all images have the same dimensions
    if r_data.shape != g_data.shape or g_data.shape != b_data.shape:
        raise ValueError("All images must have the same dimensions")

    # Create a new array for the combined image
    combined_data = np.zeros_like(r_data)

    # Combine the R, G, B channels from the respective images
    combined_data[..., 0] = r_data[..., 0]  # R channel from the first image
    combined_data[..., 1] = g_data[..., 1]  # G channel from the second image
    combined_data[..., 2] = b_data[..., 2]  # B channel from the third image

    # Convert the combined array back to an image
    combined_img = Image.fromarray(combined_data)

    # Save the combined image
    combined_img.save(output_path)

    print(f"Combined image saved to {output_path}")

folder_name = "rusted_iron"
file_name = "rusted_iron"
r_path = 'resources/textures/' + folder_name + '/' + file_name + '_metallic.png'
g_path = 'resources/textures/' + folder_name + '/' + file_name + '_roughness.png'
b_path = 'resources/textures/' + folder_name + '/' + file_name + '_ao.png'
output_path = 'resources/textures/' + folder_name + '/' + file_name + '_metal_rough_ao.png'  # Replace with desired output path
combine_rgb_channels(r_path, g_path, b_path, output_path)
#print_image_pixels("tmp/5.png")