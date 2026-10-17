# BEAR-C2 Framework


BEAR-C2 is an adversary simulation and emulation framework built around real world TTPs inspired by `Russian, Chinese, North Korean, and Iranian APT groups.` It provides a flexible environment for diverse engagement scenarios and delivers a realistic foundation for red team operations and adversary emulation drawing from related simulation research in the [APT Attack Simulation Repository](https://github.com/S3N4T0R-0X0/APT-Attack-Simulation). It supports defense evasion techniques and multiple encryption options for accurate representation of real world intrusion scenarios.

---


<img width="1735" height="906" alt="image psd(1)" src="https://github.com/user-attachments/assets/cbbd263e-951e-4ee5-9049-9f238aad0ddb" />


> [!CAUTION]
> It's essential to note that this project is for educational and research purposes only, and any unauthorized use of it could lead to legal consequences.

### 🏗 Install dependencies and compile:

```bash
git clone https://github.com/S3N4T0R-0X0/BEAR-C2.git && cd BEAR-C2

chmod +x Makefile && ./Makefile

./BEAR-C2
```
---


## 🧠 The Challenge with Adversary Simulation:

Accurately replicating **APT techniques** requires a `flexible environment capable of mimicking connection protocols, encryption methods, exfiltration techniques, and C2 Channels/Profiles` used in modern intrusions. However, achieving this level of precision has always been a challenge.

<img width="1366" height="738" alt="1" src="https://github.com/user-attachments/assets/1454a709-46be-4677-a3d0-7b9f6abadfd7" />


Every time an operator needs to test a specific **encryption scheme** with a particular **exfiltration profile**, a separate **C2 script** `must be built to match the attack scenario.` For example, one simulation might require **AES encryption** with **OneDrive exfiltration**, while another might need **a different encryption method** combined with **Dropbox exfiltration** to reflect the techniques observed in real world attacks. This lack of flexibility makes the process inefficient and time consuming.

<img width="1358" height="684" alt="2" src="https://github.com/user-attachments/assets/23d901d8-6c2a-4cf0-834b-40ff63cb2601" />

This is why **BEAR C2** was developed to provide **adversary simulation** with full customization through the new listener, allowing seamless configuration of  `connection protocols, encryption, exfiltration,` and automated loading techniques. This ensures that simulations can accurately reflect real **APT intrusions** without the need to build custom scripts for every scenario.

## 💉 Payload Generator

**payload generator** of BEAR-C2. It automatically builds customized payloads / beacons according to the selected listener configuration, supporting Payload Types (`EXE`, `DLL`, `BIN`, `HEX`), Network Protocols (`DoH`, `HTTPS`, `HTTP`, `QUIC`, `Reverse TCP`), and Encryption types (`AES`, `XOR`, `RC4`, `DES`, `ChaCha20`, `RSA`, or none). The **Auto Fills** button opens a listener selector window that lists all previously configured listeners from the Listeners Table. When you select a listener, Tactical Load automatically populates the Listener Name, Operator Address + Port, Network Protocol, Encryption type + key, Beacon Auth ID, and URI path. This allows operators to quickly generate matching payloads without manually re-entering listener details.

<img width="929" height="407" alt="3" src="https://github.com/user-attachments/assets/174f8af1-cdba-435e-a6d1-0e38d086806c" />

## 📋 Listeners Table 

The Listeners Table provides a centralized overview of all active and configured C2 listeners. It displays essential details such as **listener name, address, network protocol, encryption method, exfiltration profile**, and current status (Active or Stopped/Disconnected). From this interface, operators can start, stop, rename, or remove listeners with ease. It also offers quick access to encryption keys and authentication IDs for managing beacon communication. This table serves as the command hub for orchestrating and monitoring your C2 infrastructure.

<img width="999" height="486" alt="Screenshot From 2026-08-26 15-15-37" src="https://github.com/user-attachments/assets/123ceba6-fa3f-4c19-9286-d2ffc8c1909c" />



---


## 📤 Exfiltration Profiles

Configure per-session exfiltration settings for supported cloud storage providers such as `Google Drive, OneDrive, and Dropbox.` The **Exfiltration Profile** interface allows you to define API access tokens and destination folder paths, enabling you to customize data collection workflows for each session. Each session can use its own exfiltration profile, making it easy to route collected data to different cloud storage providers or destinations depending on the operation.


<img width="2423" height="708" alt="4" src="https://github.com/user-attachments/assets/72b66bb2-335f-4a96-ba77-9f76b40d55e0" />

---

## 💬 Integrated C2 Channels 

### (Telegram-based Agent)

The Telegram communication layer uses a Telegram bot as the intermediary between the C2 server and the payload. The C2 server authenticates to the Telegram account using the configured API ID, API Hash, and Phone Number, then connects to the previously created Telegram bot through that account.

<img width="1366" height="739" alt="4" src="https://github.com/user-attachments/assets/195df4ad-20da-41b3-ad17-0913474b2df0" />

---

### (Discord-based Agent)

This stage replaces the traditional command and control communication channel with a **Discord-based communication layer** using the **Discord Gateway API**. Instead of relying on dedicated servers, fixed IP addresses, or custom domains, operators communicate with the simulated implant through a private Discord channel.

<img width="1366" height="739" alt="5" src="https://github.com/user-attachments/assets/4ab2dc7a-363e-4d18-bc5e-4b1db35f5a50" />


---

## 🪝 Spear Phishing Simulation

Simulate spear-phishing campaigns through a dedicated interface for configuring and managing phishing scenarios during authorized adversary simulation exercises. The module provides campaign controls such as **victim submission limits**, configurable phishing parameters, and session management, allowing operators to control campaign behavior and evaluate user interaction with simulated phishing scenarios. It is designed to support controlled phishing assessments while providing operators with greater visibility and control over campaign execution.

<img width="1336" height="553" alt="6" src="https://github.com/user-attachments/assets/78a106c7-0244-4693-9027-0a83d389f1be" />



## 🔗 Host File

Host and distribute files through a dedicated **Host File** interface with configurable server settings and automated file delivery. Hosted files are automatically loaded when accessed, without requiring user interaction or a manual download prompt. The interface provides centralized file hosting and delivery management for controlled adversary simulation and authorized security testing workflows.


<p align="center">
  <img width="774" height="511" alt="Screenshot From 2026-10-05 04-37-52" src="https://github.com/user-attachments/assets/d6ff0a8e-d9ad-4a8c-8938-85fa30333e73" />
</p>


## 🔐 Script Obfuscator

The **Script Obfuscator** provides a comprehensive obfuscation engine supporting **PS1, BAT, HTML, VBS, JS, and PY** payloads with multiple configurable obfuscation layers. It includes `variable and function renaming, string encryption, junk code insertion, multi-layer obfuscation, anti-debugging techniques, and XOR-based payload encryption.` These features increase analysis complexity, reduce script readability, and make reverse engineering significantly more difficult while helping payloads better withstand static analysis.


<p align="center">
<img width="595" height="416" alt="Screenshot From 2026-10-05 07-27-27" src="https://github.com/user-attachments/assets/dc9bb5af-2e79-46a6-915a-9633eaf7dd09" />
</p>


----

<p align="center"><strong>The complete list of APT groups simulated by BEAR-C2 throughout its development</em></strong></p>


<div align="center">

  <table>
  <tr>
<th align="center"><strong>Country<br>of Origin</br></strong></th>
    <th align="center">Russia 🇷🇺</th>
    <th align="center">China 🇨🇳</th>
    <th align="center">North Korea 🇰🇵</th>
    <th align="center">Iran 🇮🇷</th>
  </tr>
  <tr>
    <td align="center"><strong>APT Groups</strong></td>
    <td align="center">

[**Cozy Bear ✅**](https://github.com/S3N4T0R-0X0/APT29-Adversary-Simulation.git)<br>
[**Voodoo Bear ✅**](https://github.com/S3N4T0R-0X0/Voodoo-Bear-APT.git)<br>
[**Fancy Bear ✅**](https://github.com/S3N4T0R-0X0/APT28-Adversary-Simulation.git)<br>
[**Energetic Bear ✅**](https://github.com/S3N4T0R-0X0/Energetic-Bear-APT.git)<br>
[**Berserk Bear ✅**](https://github.com/S3N4T0R-0X0/Berserk-Bear-APT.git)<br>
[**Gossamer Bear ✅**](https://github.com/S3N4T0R-0X0/Gossamer-Bear-APT.git)<br>
[**Primitive Bear ✅**](https://github.com/S3N4T0R-0X0/Primitive-Bear-APT.git)<br>
[**Ember Bear ✅**](https://github.com/S3N4T0R-0X0/Ember-Bear-APT.git)<br>
[**Venomous Bear ✅**](https://github.com/S3N4T0R-0X0/Venomous-Bear-APT.git)

</td>
<td align="center">

[**Mustang Panda ✅**](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Chinese%20APT/Mustang%20Panda)<br>
[Glacial Panda](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Chinese%20APT/Glacial-Panda)<br>
[**Wicked Panda ✅**](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Chinese%20APT/Wicked%20Panda)<br>
[Goblin Panda](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Chinese%20APT/Goblin-Panda)<br>
[Anchor Panda](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Chinese%20APT/Anchor-Panda)<br>
[Deep Panda](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Chinese%20APT/Deep-Panda)<br>
[Samurai Panda](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Chinese%20APT/Samurai-Panda)<br>
[Phantom Panda](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Chinese%20APT/Phantom-Panda)<br>
[Sunrise Panda](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Chinese%20APT/Sunrise-Panda)<br>
[Ethereal Panda](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Chinese%20APT/Ethereal-Panda)

</td>
<td align="center">

[**Labyrinth Chollima ✅**](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/North%20Koreans%20APT/Labyrinth%20Chollima)<br>
[**Velvet Chollima ✅**](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/North%20Koreans%20APT/Velvet%20Chollima)<br>
[**Famous Chollima ✅**](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/North%20Koreans%20APT/Famous%20Chollima)<br>
[**Stardust Chollima ✅**](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/North%20Koreans%20APT/Stardust%20Chollima)<br>
[**Ricochet Chollima ✅**](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/North%20Koreans%20APT/Ricochet%20Chollima)<br>
[**Silent Chollima ✅**](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/North%20Koreans%20APT/Silent%20Chollima)

</td>
<td align="center">

[Helix Kitten ✅](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Iranian%20APT/Helix%20Kitten)<br>
[Pioneer Kitten](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Iran%20APT/Pioneer-Kitten)<br>
[Clever Kitten](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Iran%20APT/Clever-Kitten)<br>
[**Static Kitten ✅**](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Iranian%20APT/Static%20Kitten)<br>
[Tracer Kitten](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Iran%20APT/Tracer-Kitten)<br>
[Nemesis Kitten](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Iran%20APT/Nemesis-Kitten)<br>
[**Muddy Water ✅**](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Iranian%20APT/Muddy%20Water)<br>
[Pulsar Kitten](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Iran%20APT/Pulsar-Kitten)<br>
[Remix Kitten](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Iran%20APT/Remix-Kitten)<br>
[Haywire Kitten](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Iran%20APT/Haywire-Kitten)

</td>
  </tr>
</table>

</div>


## 📫 Contact

<table align="center">
  <tr>
    <td align="center"><a href="https://t.me/BearC2"><img src="https://img.icons8.com/color/48/000000/telegram-app.png" width="40"/></a></td>
    <td align="center"><a href="https://x.com/S3N4T0R_0X0"><img src="https://img.icons8.com/color/48/000000/twitterx--v1.png" width="40"/></a></td>
    <td align="center"><a href="https://eg.linkedin.com/in/s3n4t0r"><img src="https://img.icons8.com/color/48/000000/linkedin.png" width="40"/></a></td>
    <td align="center"><a href="https://www.reddit.com/u/S3N4T0R-0X0/s/pF0LHjjCOs"><img src="https://img.icons8.com/color/48/FF4500/reddit.png" width="40"/></a></td>
  </tr>
  <tr>
    <td align="center"><a href="https://t.me/BearC2">Telegram</a></td>
    <td align="center"><a href="https://x.com/S3N4T0R_0X0">Twitter/X</a></td>
    <td align="center"><a href="https://eg.linkedin.com/in/s3n4t0r">LinkedIn</a></td>
    <td align="center"><a href="https://www.reddit.com/u/S3N4T0R-0X0/s/pF0LHjjCOs">Reddit</a></td>
  </tr>
</table>


