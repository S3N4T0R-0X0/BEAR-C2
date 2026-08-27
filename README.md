# BEAR-C2 Adversary Simulation Framework
---
 [![Project Status](https://img.shields.io/badge/Status-BETA-yellow?style=flat-square)]()  [![verigen: 2.0](https://img.shields.io/badge/verigen-2.0-green?style=flat-square)](https://img.shields.io/badge/verigen-2.0-green?style=flat-square) [![Adversary Simulation](https://img.shields.io/badge/Adversary-Simulation-purple?style=flat-square)]() [![TTPs](https://img.shields.io/badge/TTPs-Emulation-blue?style=flat-square)]() [![APT Simulation](https://img.shields.io/badge/Red-Team-darkred?style=flat-square)]() [![MITRE ATT&CK](https://img.shields.io/badge/MITRE-ATT%26CK-orange?style=flat-square)]() [![Linux](https://img.shields.io/badge/Platform-Linux-black?style=flat-square)](https://img.shields.io/badge/Platform-Linux-black?style=flat-square)

BEAR-C2 is an adversary simulation and emulation framework built around real world TTPs inspired by `Russian, Chinese, North Korean, and Iranian APT groups.` It provides a flexible environment for diverse engagement scenarios and delivers a realistic foundation for red team operations and adversary emulation drawing from related simulation research in the [APT Attack Simulation Repository](https://github.com/S3N4T0R-0X0/APT-Attack-Simulation). It supports defense evasion techniques and multiple encryption options for accurate representation of real world intrusion scenarios.

---


<img width="1735" height="906" alt="1" src="https://github.com/user-attachments/assets/11b96e6d-c47a-43d2-a579-831d73b7980b" />


> [!CAUTION]
> It's essential to note that this project is for educational and research purposes only, and any unauthorized use of it could lead to legal consequences.

## 🏗 Install dependencies and Usage:

```bash
git clone https://github.com/S3N4T0R-0X0/BEAR-C2.git

cd BEAR-C2

chmod +x requirements.sh && ./requirements.sh

./BEAR-C2
```
---


## 🧠 The Challenge with Adversary Simulation:

Accurately replicating **APT techniques** requires a `flexible environment capable of mimicking connection protocols, encryption methods, exfiltration techniques, and C2 Channels/Profiles` used in modern intrusions. However, achieving this level of precision has always been a challenge.

<img width="1366" height="732" alt="image psd" src="https://github.com/user-attachments/assets/d5d63124-a515-4367-8b70-a78e9acf2622" />


Every time an operator needs to test a specific **encryption scheme** with a particular **exfiltration profile**, a separate **C2 script** `must be built to match the attack scenario.` For example, one simulation might require **AES encryption** with **OneDrive exfiltration**, while another might need **a different encryption method** combined with **Dropbox exfiltration** to reflect the techniques observed in real world attacks. This lack of flexibility makes the process inefficient and time consuming.


<img width="1359" height="679" alt="3" src="https://github.com/user-attachments/assets/30cf94aa-2f99-4f16-8469-c1379a0d84e2" />


This is why **BEAR C2** was developed to provide **adversary simulation** with full customization through the new listener, allowing seamless configuration of  `connection protocols, encryption, exfiltration,` and automated loading techniques. This ensures that simulations can accurately reflect real **APT intrusions** without the need to build custom scripts for every scenario.


The Listeners Table provides a centralized overview of all active and configured C2 listeners. It displays essential details such as **listener name, address, network protocol, encryption method, exfiltration profile**, and current status (Active or Stopped/Disconnected). From this interface, operators can start, stop, rename, or remove listeners with ease. It also offers quick access to encryption keys and authentication IDs for managing beacon communication. This table serves as the command hub for orchestrating and monitoring your C2 infrastructure.

<img width="999" height="486" alt="Screenshot From 2026-08-26 15-15-37" src="https://github.com/user-attachments/assets/123ceba6-fa3f-4c19-9286-d2ffc8c1909c" />

## 📋 What's New in This Version

This version features a full GUI that streamlines adversary simulation operations through centralized listener management, real-time session tracking, customizable communication profiles, integrated exfiltration workflows, and flexible operator controls for efficient engagement management.

> ⚠️ **NOTE:** This project is under active development. Features are continuously added and improved.


| Feature | Description |
|----------|-------------|
| **Multi-Protocol Listeners** | `DoH`, `HTTPS`, `HTTP`, `QUIC`, `Reverse TCP` |
| **Per-Listener Encryption** | `AES`, `XOR`, `RC4`, `DES`, `ChaCha20`, `RSA` |
| **Exfiltration Profiles** | `Google Drive`, `OneDrive`, `Dropbox` |
| **Integrated C2 Channels** | Integrated `Telegram`, `Discord`  C2 communication channel |
| **Proxy Support** | `SOCKS4`, `SOCKS4a`, and `SOCKS5` proxy and redirector support |
| **Dynamic Domain Generation Algorithm** | `DGA` support for resilient infrastructure simulation |
| **JA3S Fingerprinting** | Customizable `JA3S` fingerprints for traffic simulation and network profile tuning |
| **Malleable C2 Profiles** | Support for community `Malleable C2 profiles` for flexible network traffic simulation |
| **Stagers & Loaders** | Automated stager and loader techniques designed for APTs adversary simulation |
| **Integrated Tooling** | Built-in script obfuscator, phishing toolkit, and file hosting |
| **TLS Certificate Generation** | Self-signed TLS certificates mimicking trusted vendors |
| **HTTP Customization** | Base64 URL encoding and custom HTTP headers for both client and server communication |
| **Real-time Session Manager** | Live status tracking, session monitoring, and real‑time update capabilities |
| **Custom Naming & URI Paths** | User‑defined campaign names and configurable URI paths for operational flexibility |
| **Reconnect & Timeout Controls** | Configurable reconnect delays and adjustable timeout thresholds per session |
| **Authentication Identifiers** | Unique authentication tokens with built‑in expiration controls for enhanced security |
| **Session Hardening Utilities** | History cleaner, session limiter, and authentication timeout management for active sessions |

---



## 📤 Exfiltration Profiles

Configure per-session exfiltration settings for supported cloud storage providers such as `Google Drive, OneDrive, and Dropbox.` The **Exfiltration Profile** interface allows you to define API access tokens and destination folder paths, enabling you to customize data collection workflows for each session. Each session can use its own exfiltration profile, making it easy to route collected data to different cloud storage providers or destinations depending on the operation.


<img width="2423" height="708" alt="4" src="https://github.com/user-attachments/assets/72b66bb2-335f-4a96-ba77-9f76b40d55e0" />


## 💬 Integrated C2 Channels 

### (Telegram-based Agent)



The Telegram communication layer uses a [Telegram bot](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Iranian%20APT/Charming%20Kitten#the-third-stage-telegram-based-agent) as the intermediary between the C2 server and the payload. The C2 server authenticates to the Telegram account using the configured API ID, API Hash, and Phone Number, then connects to the previously created Telegram bot through that account

<img width="1276" height="585" alt="5" src="https://github.com/user-attachments/assets/751c6afa-a2fb-45f7-9d99-efcf554f78d8" />



The bot's Bot Token is embedded in the payload, allowing the payload to communicate with the bot through the Telegram API. Commands are sent from the C2 server to the bot, and the bot forwards them to the payload, which executes the received tasking and returns the results through the same communication path. This creates a bidirectional command and control channel using Telegram as the communication layer.

<img width="1920" height="1080" alt="telegram" src="https://github.com/user-attachments/assets/3390bbe7-775f-498e-9526-f611762c16ce" />


---

### (Discord-based Agent)

This stage replaces the traditional command and control communication channel with a **Discord-based communication layer** using the **Discord Gateway API**. Instead of relying on dedicated servers, fixed IP addresses, or custom domains, operators communicate with the simulated implant through a private Discord channel.


<img width="1276" height="577" alt="6" src="https://github.com/user-attachments/assets/e0232e1c-725e-486d-925d-9172c7dd5c06" />


The Discord communication layer uses two Discord bots connected through the same Discord channel. One bot is connected to the C2 server, while the second bot is assigned to the payload. The two bots communicate through the shared Discord channel, allowing tasking and communication between the C2 server and payload through Discord.

Since Discord does not allow bots to directly communicate with other bots, using a shared channel provides the communication path between the two sides.


<img width="3472" height="1784" alt="discord" src="https://github.com/user-attachments/assets/439f79ae-58ed-4846-b332-ab567d0c0e78" />


---

## 🪝 Spear Phishing Simulation

Simulate spear-phishing campaigns through a dedicated interface for configuring and managing phishing scenarios during authorized adversary simulation exercises. The module provides campaign controls such as **victim submission limits**, configurable phishing parameters, and session management, allowing operators to control campaign behavior and evaluate user interaction with simulated phishing scenarios. It is designed to support controlled phishing assessments while providing operators with greater visibility and control over campaign execution.

<img width="1340" height="562" alt="7" src="https://github.com/user-attachments/assets/ceeeee51-bb6c-4e90-b84a-a9c4397cae3a" />



## 🔗 Host File

Host and distribute files through a dedicated **Host File** interface with configurable server settings and automated file delivery. Hosted files are automatically loaded when accessed, without requiring user interaction or a manual download prompt. The interface provides centralized file hosting and delivery management for controlled adversary simulation and authorized security testing workflows.


<p align="center">
  <img width="774" height="511" alt="Screenshot From 2026-07-03 09-18-06" src="https://github.com/user-attachments/assets/0c8dcf87-1f29-4c9d-bfc8-6f6d78f69f64" />
</p>


## 🔐 Script Obfuscator

The **Script Obfuscator** provides a comprehensive obfuscation engine supporting **PS1, BAT, HTML, VBS, JS, and PY** payloads with multiple configurable obfuscation layers. It includes `variable and function renaming, string encryption, junk code insertion, multi-layer obfuscation, anti-debugging techniques, and XOR-based payload encryption.` These features increase analysis complexity, reduce script readability, and make reverse engineering significantly more difficult while helping payloads better withstand static analysis.


<p align="center">
  <img width="599" height="420" alt="Script Obfuscator" src="https://github.com/user-attachments/assets/44c340c2-8f9b-43a3-a4d7-1cb00428bceb" />
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

[Helix Kitten](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Iran%20APT/Helix-Kitten)<br>
[Pioneer Kitten](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Iran%20APT/Pioneer-Kitten)<br>
[Clever Kitten](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Iran%20APT/Clever-Kitten)<br>
[**Static Kitten ✅**](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Iranian%20APT/Static%20Kitten)<br>
[Tracer Kitten](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Iran%20APT/Tracer-Kitten)<br>
[Nemesis Kitten](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Iran%20APT/Nemesis-Kitten)<br>
[Charming Kitten](https://github.com/S3N4T0R-0X0/APTs-Adversary-Simulation/tree/main/Iran%20APT/Charming-Kitten)<br>
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


