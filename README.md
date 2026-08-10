# BEAR-C2 [![Project Status](https://img.shields.io/badge/Status-BETA-yellow?style=flat-square)]() [![Adversary Simulation](https://img.shields.io/badge/Adversary-Simulation-purple?style=flat-square)]() [![TTPs](https://img.shields.io/badge/TTPs-Emulation-blue?style=flat-square)]() [![APT Simulation](https://img.shields.io/badge/Red-Team-darkred?style=flat-square)]() [![MITRE ATT&CK](https://img.shields.io/badge/MITRE-ATT%26CK-orange?style=flat-square)]()  [![Red Team](https://img.shields.io/badge/APT-groups-red?style=flat-square)]()

---

BEAR-C2 is an adversary simulation and emulation framework built around real world TTPs inspired by `Russian, Chinese, North Korean, and Iranian APT groups.` It provides a flexible environment for diverse engagement scenarios and delivers a realistic foundation for red team operations and adversary emulation drawing from related simulation research in the [APT Attack Simulation Repository](https://github.com/S3N4T0R-0X0/APT-Attack-Simulation). It supports defense evasion techniques and multiple encryption options for accurate representation of real world intrusion scenarios.

---

<img width="1735" height="906" alt="ae958cb6-ee6e-4bc1-85b7-bb1681acbdf7" src="https://github.com/user-attachments/assets/9e6be27f-d213-4d1d-bab4-d205a95a8808" />

> [!CAUTION]
> It's essential to note that this project is for educational and research purposes only, and any unauthorized use of it could lead to legal consequences.

## 🏗 Install dependencies and Usage:

```bash
chmod +x requirements.sh

./BEAR-C2

```
---


## 🧠 The Challenge with Adversary Simulation:

Accurately replicating **APT techniques** requires a `flexible environment capable of mimicking encryption methods, exfiltration techniques, and connection protocols` used in modern intrusions. However, achieving this level of precision has always been a challenge.

<img width="1366" height="745" alt="Screenshot From 2026-06-10 22-56-09" src="https://github.com/user-attachments/assets/08e8cceb-2382-40e2-8574-59da028b738f" />

Every time an operator needs to test a specific **encryption scheme** with a particular **exfiltration profile**, a separate **C2 script** `must be built to match the attack scenario.` For example, one simulation might require **AES encryption** with **OneDrive exfiltration**, while another might need **a different encryption method** combined with **Dropbox exfiltration** to reflect the techniques observed in real world attacks. This lack of flexibility makes the process inefficient and time consuming.

<img width="1359" height="679" alt="Screenshot From 2026-07-23 19-41-01" src="https://github.com/user-attachments/assets/cb48d4e4-b36a-4caa-b196-0c2c77398bca" />


This is why **BEAR C2** was developed to provide **adversary simulation** with full customization through the new listener, allowing seamless configuration of  `connection protocols, encryption, exfiltration,` and automated loading techniques. This ensures that simulations can accurately reflect real **APT intrusions** without the need to build custom scripts for every scenario.

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
| **TLS Certificate Generation** | Self-signed TLS certificates mimicking trusted vendors (e.g., Google LLC) |
| **HTTP Customization** | Base64 URL encoding and custom HTTP headers for both client and server communication |
| **Real-time Session Manager** | Live status tracking, session monitoring, and real‑time update capabilities |
| **Custom Naming & URI Paths** | User‑defined campaign names and configurable URI paths for operational flexibility |
| **Reconnect & Timeout Controls** | Configurable reconnect delays and adjustable timeout thresholds per session |
| **Authentication Identifiers** | Unique authentication tokens with built‑in expiration controls for enhanced security |
| **Session Hardening Utilities** | History cleaner, session limiter, and authentication timeout management for active sessions |

---



## 📤 Exfiltration Profiles

Configure per-session exfiltration settings for supported cloud storage providers such as `Google Drive, OneDrive, and Dropbox.` The **Exfiltration Profile** interface allows you to define API access tokens and destination folder paths, enabling you to customize data collection workflows for each session. Each session can use its own exfiltration profile, making it easy to route collected data to different cloud storage providers or destinations depending on the operation.


<img width="2423" height="708" alt="1785656148307" src="https://github.com/user-attachments/assets/d749026f-fb76-4483-b4d6-bdb4248577fc" />


## 💬 Integrated C2 Channels 

### (Telegram-based Agent)

BEAR C2 supports operator notifications and basic command relay through **Telegram** using the `telethon` library. Configure your API ID, API hash, phone number, and bot username in the Authentication settings to enable Telegram integration for operator notifications and command relay.

The initial objective of this stage is to enhance the realism of the adversary simulation by replacing the traditional direct command and control communication channel with a Telegram-based communication layer. Instead of requiring operators to interact with the payload through a dedicated control server, commands are exchanged through a Telegram bot, allowing the simulation to emulate an alternative communication workflow commonly observed in modern threat campaigns.

This phase focuses on demonstrating how a trusted cloud messaging platform can serve as an intermediary communication channel between the operator and the simulated implant. By leveraging Telegram as the transport layer, the simulation highlights how legitimate online services may be used to blend command and control traffic with normal network activity while maintaining reliable bidirectional communication.

<img width="1276" height="585" alt="Screenshot From 2026-08-06 12-53-38 (Edited)" src="https://github.com/user-attachments/assets/6b1c51a4-7e72-4d54-bca4-e831eefb5df1" />


### (Discord-based Agent)

This stage replaces the traditional command and control communication channel with a **Discord-based communication layer** using the **Discord Gateway API**. Instead of relying on dedicated servers, fixed IP addresses, or custom domains, operators communicate with the simulated implant through a private Discord channel.

The implant maintains a persistent connection to the Discord Gateway to receive operator tasking in near real time, while command results are returned through the Discord API. This demonstrates how trusted cloud communication platforms can be leveraged as alternative transport layers for command and control.

The objective is to provide a realistic environment for studying cloud-based communication patterns and to emphasize behavioral detection over destination-based detection.


<img width="1276" height="577" alt="Screenshot From 2026-08-06 13-10-30" src="https://github.com/user-attachments/assets/ed625043-4a73-4fe6-83f9-173aa2e20b24" />


---

## 🪝 Spear Phishing Simulation

Simulate spear phishing campaigns through a dedicated interface for creating, managing, and tracking phishing scenarios during authorized security assessments. The module enables operators to evaluate user awareness and emulate phishing based attack techniques as part of adversary simulation exercises.

<img width="1340" height="562" alt="616869182-80c361fc-c25c-40d0-8e47-cc0a4dea6325" src="https://github.com/user-attachments/assets/f6da7972-5f8b-45a8-9c09-0749290bbe73" />


## 🔗 Host File

Host and distribute files through a dedicated interface with configurable server settings, download tracking, and detailed access logging. Designed to simplify controlled file delivery and monitor client download activity.

<p align="center">
  <img width="774" height="511" alt="Screenshot From 2026-07-03 09-18-06" src="https://github.com/user-attachments/assets/098f1774-dc84-40f0-a84c-ceabb75fcdcd" />
</p>


## 🔐 Script Obfuscator

The **Script Obfuscator** provides a comprehensive obfuscation engine for PowerShell payloads. It includes variable and function renaming, string encryption, junk code insertion, multi layer obfuscation, anti debugging techniques, and XOR based payload encryption. These features increase analysis complexity, reduce script readability, and make reverse engineering significantly more difficult while helping payloads better withstand static analysis.

<p align="center">
  <img width="599" height="420" alt="Script Obfuscator" src="https://github.com/user-attachments/assets/44bd087f-7ca0-4676-8a6c-ad03b52439f2" />
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


