from PIL import Image
import math, os
import numpy as np
from moviepy import ImageSequenceClip
from tqdm import tqdm

def file_to_video(input_file, width=1920, height=1080, pixel_size=4, fps=24):
    if not os.path.isfile(input_file):
        print(f"Error: File '{input_file}' does not exist.")
        return

    file_size = os.path.getsize(input_file)
    binary_string = ""
    with open(input_file, "rb") as f:
        for chunk in tqdm(iterable=iter(lambda: f.read(1024), b""), 
                         total=math.ceil(file_size/1024), unit="KB"):
            binary_string += "".join(f"{byte:08b}" for byte in chunk)

    pixels_per_image = (width // pixel_size) * (height // pixel_size)
    num_images = math.ceil(len(binary_string) / pixels_per_image)
    frames = []

    for i in tqdm(range(num_images), desc="Generating frames"):
        start = i * pixels_per_image
        bits = binary_string[start:start + pixels_per_image]
        img = Image.new('RGB', (width, height), color='white')
        
        for r in range(height // pixel_size):
            row = bits[r * (width // pixel_size):(r + 1) * (width // pixel_size)]
            for c, bit in enumerate(row):
                color = (0, 0, 0) if bit == '1' else (255, 255, 255)
                x1, y1 = c * pixel_size, r * pixel_size
                img.paste(color, (x1, y1, x1 + pixel_size, y1 + pixel_size))
        
        frames.append(np.array(img))

    clip = ImageSequenceClip(frames, fps=fps)
    clip.write_videofile('video.mp4', fps=fps, codec='libx264')
    print("Video generated successfully: video.mp4")



if __name__ == "__main__":
    print("Convert file → video")
    path = input("Enter file path: ")
    file_to_video(path)

