# Malicious PixelCode Delivery Technique

Malicious PixelCode is a security research project that demonstrates a covert technique for encoding executable files into pixel data and storing them inside images or videos. A lightweight loader retrieves the media file, reconstructs the original binary, and executes it in memory. This project highlights unconventional data delivery and obfuscation methods for educational and research purposes only.

This Research was covered by Cyber Security News: https://cybersecuritynews.com/pixelcode-attack/

![photo_2025-11-22_09-20-43](https://github.com/user-attachments/assets/73929cb5-399a-4003-a198-301c165d71fb)

---------------------------------------------------------------------------------------------------------------------

> [!CAUTION]
> **Important Notice**: This project is strictly for educational, research, and defensive security purposes only. Unauthorized use of these techniques may violate laws and result in serious legal consequences.

## What is the Pixel Code?

Pixel Code is a research concept that represents binary data in the form of visual pixels where
each pixel encodes a small portion of the original file by converting executable bytes into a
structured color matrix the data can be stored, transmitted, or embedded inside images or
videos without exposing its raw form.

This technique is often explored for studying data obfuscation, covert storage, and
unconventional encoding channels. Pixel based encoding allows researchers to analyze
how information can be hidden inside multimedia content and later reconstructed through
decoding algorithms.

It also helps in understanding the security implications of transporting encoded data through
common platforms such as image or video sharing services. Overall Pixel Code focuses
on transforming digital files into a visual representation that can be recovered using a
corresponding decoder.

![New PixelCode Attack Smuggles Malware via Image Pixel Encoding (1)](https://github.com/user-attachments/assets/97e497f1-e49a-4a7a-b6f1-2fd96733ace9)


Malicious actors could potentially misuse Pixel Code by encoding harmful binaries or scripts
into visual data such as images or video frames. By transforming malware into pixel based
representations, attackers may attempt to disguise their payloads as harmless multimedia
content.

These encoded files can then be uploaded to legitimate platforms like YouTube, giving them
an appearance of trust and bypassing traditional security filters.

A loader or decoder under the attacker’s control could later retrieve the video and reconstruct
the original malicious file. This abuse scenario highlights how unconventional data encoding
techniques can create new attack vectors and challenge existing detection mechanisms.

-------------------------------------------------------------------------------------------------

In this research process we begin with a C++ payload responsible for command and control
communication. After compiling it into an EXE the binary is converted into Pixel Code MP4
using a Python based tool. The generated Pixel Code video is then uploaded to YouTube and
its URL is embedded inside a C++ loader.

Since C++ lacked the required multimedia libraries to reverse Pixel Code a Python stager was
used instead; it was compiled into an EXE, Base64 encoded, and embedded directly into the
loader alongside the YouTube URL.

When executed on the target environment, the loader downloads the Pixel Code MP4 from
YouTube and uses the embedded Python stager to reconstruct the original payload and
execute it ultimately allowing the research environment to re-establish command and control
through BEAR-C2.

![photo_2025-11-22_07-03-08](https://github.com/user-attachments/assets/c7cf4650-510a-41ab-9a8c-3647cb4e58a8)

# Pixel Code Workflow — Detailed Steps
-----------------------------------------------------------------------------------

## (C2 Payload.cpp)

This is a stealthy Windows reverse shell payload written in C++ that persistently connects
back to a command and control server uses AES-CBC encryption with a hardcoded static
key and random IVs for all communication, authenticates with a unique identifier, executes
received commands via cmd.exe or PowerShell (when prefixed with "EP") supports directory
navigation streams command output in real time using length prefixed encrypted chunks
runs without a visible console window, and automatically reconnects on failure a fully
featured encrypted remote access trojan designed for post exploitation control.


<img width="1366" height="648" alt="Screenshot From 2025-11-22 08-16-43" src="https://github.com/user-attachments/assets/92fbeff0-5b07-4f5a-9d0e-ec2498230ad9" />

## (Converting the EXE into Pixel Code)

Once the payload is compiled, the resulting EXE is processed through a Python based tool
specifically designed to convert binary data into Pixel Code. This tool converts the executable
into a visual MP4 representation where the binary bytes are embedded as pixel values across
video frames.

<img width="1239" height="676" alt="Screenshot From 2025-11-22 08-41-51" src="https://github.com/user-attachments/assets/c5023493-cba2-491d-88e6-f637de61bef6" />

## (Uploading the Pixel Code Video to YouTube) 

After generating the Pixel Code MP4 file, it is uploaded to YouTube as a standard video.
Hosting the encoded binary on a legitimate platform allows the research to evaluate how
external trusted services handle non traditional encoded data. The YouTube video URL is
collected for use in the next stages.

<img width="1245" height="668" alt="Screenshot From 2025-11-22 08-51-34" src="https://github.com/user-attachments/assets/eaf688e5-ffa3-40f3-bd0c-c046a982c5e9" />

## (Embedding the YouTube URL Inside the C++ Loader)

A custom loader written in C++ is prepared to retrieve the Pixel Code from its YouTube
location. The video URL obtained in the previous step is embedded directly into this loader
so it can download the MP4 during execution.

<img width="1366" height="718" alt="Screenshot From 2025-11-22 08-57-44" src="https://github.com/user-attachments/assets/ef6d6f34-a4c8-4291-bb71-324fe6bbb315" />

## (Preparing the Python Stager for Decoding)

Because C++ lacks the multimedia libraries required to decode Pixel Code back into binary
form, a Python stager is used to handle reconstruction. This Python script is compiled into an
EXE and then Base64-encoded. The encoded stager is embedded directly inside the C++
loader alongside the YouTube URL.

Stager.py -> Stager.exe -> base64 -> Loader.cpp -> Loader.exe

<img width="1366" height="718" alt="Screenshot From 2025-11-22 09-01-17" src="https://github.com/user-attachments/assets/e222147b-c289-4b3e-8145-543086fab6c0" />

## (Reconstructing and Executing the Original Payload)

The Python stager processes the downloaded MP4 video frame by frame, extracts the
encoded pixel data and reconstructs the original C++ payload in memory.

<img width="1247" height="666" alt="Screenshot From 2025-11-22 09-29-20" src="https://github.com/user-attachments/assets/4c9a11c0-2e37-4012-9ee6-54c05ac71e0b" />

Once restored the payload is executed, ultimately re-establishing command and control communication
with the BEAR-C2 server used in the research setup.

<img width="1247" height="666" alt="Screenshot From 2025-11-22 09-08-27" src="https://github.com/user-attachments/assets/1953c15a-f67e-497b-bcfd-21d4b31653f8" />

---------------------------------------------------------------------------------------------------------------------

## (PixelCode Demo)


https://github.com/user-attachments/assets/1dcb1570-b116-438e-b826-4ce639a1c13c


## ⚠️ Legal Disclaimer
This is for research, awareness, and educational purposes, I am not responsible if anyone uses this technique for illegal purposes.









