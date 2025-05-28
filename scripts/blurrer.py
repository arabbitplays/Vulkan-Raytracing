from PIL import Image, ImageFilter

# Load the image
input_path = "input.png"   # Replace with your actual input file path
output_path = "output_blurred.png"

# Open the image
image = Image.open(input_path)

# Apply a blur filter
blurred_image = image.filter(ImageFilter.GaussianBlur(radius=50))  # Adjust radius for stronger/weaker blur
blurred_image = blurred_image.filter(ImageFilter.GaussianBlur(radius=50))  # Adjust radius for stronger/weaker blur

# Save the result
blurred_image.save(output_path)

print(f"Blurred image saved to: {output_path}")
