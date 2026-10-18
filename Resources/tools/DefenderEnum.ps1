    $ErrorActionPreference = 'SilentlyContinue'

    Write-Output "[*] Checking System Privileges..."
    $adminCheck = whoami /groups | Select-String "S-1-16-12288"
    if ($adminCheck) {
        Write-Output "[+] Running as Administrator"
    } else {
        Write-Output "[-] Not running as Administrator"
    }

    Write-Output ""
    Write-Output "[*] Scanning for Security Products..."
    $tools = "WinDefend", "Kaspersky", "ESET", "CrowdStrike", "SentinelOne", "Sophos", "McAfee", "Avast", "Bitdefender", "Cylance", "Norton", "Webroot", "TrendMicro", "Symantec", "Malwarebytes", "CarbonBlack", "PaloAlto", "CortexXDR", "FireEye", "MicrosoftDefender", "Trellix", "DeepInstinct", "Harmony", "FortiEDR", "CiscoSecureEndpoint", "ElasticEDR"
    $found = 0

    foreach ($tool in $tools) {
        $service = Get-Service | Where-Object { $_.Name -like "*$tool*" -or $_.DisplayName -like "*$tool*" }
        if ($service) {
            Write-Output "[+] Found: $tool"
            $found = 1
        }
    }
    if ($found -eq 0) {
        Write-Output "[-] No Security Products Found"
    }

    Write-Output ""
    Write-Output "[*] Scanning for Network Monitoring Tools..."
    $nettools = "Wireshark", "Fiddler", "TCPView", "NetMon", "EtherApe", "SmartSniff", "MicrosoftNetworkMonitor"
    $found = 0

    foreach ($nettool in $nettools) {
        $process = Get-Process | Where-Object { $_.ProcessName -like "*$nettool*" }
        if ($process) {
            Write-Output "[+] Running: $nettool"
            $found = 1
        }
    }
    if ($found -eq 0) {
        Write-Output "[-] No Network Monitoring Tools Found"
    }

    Write-Output ""
    Write-Output "[*] Scanning for Reverse Engineering Tools..."
    $revtools = "ida", "x64dbg", "ghidra", "pe-studio", "pestudio", "procmon", "procexp", "dnSpy", "OllyDbg", "radare"
    $found = 0

    foreach ($revtool in $revtools) {
        $process = Get-Process | Where-Object { $_.ProcessName -like "*$revtool*" }
        if ($process) {
            Write-Output "[+] Running: $revtool"
            $found = 1
        }
    }
    if ($found -eq 0) {
        Write-Output "[-] No Reverse Engineering Tools Found"
    }

    Write-Output ""
    Write-Output "[*] Checking for Sandbox or VM..."
    $indicators = "VBoxService", "vboxmrxnp", "VMwareService", "vmtoolsd", "vmsrvc", "vboxtray", "vmware-authd", "vmacthlp", "vmwareuser", "vmwaretray", "virtualbox", "vmware-vmx", "vmx86", "VirtualBoxVM"
    $found = 0

    foreach ($indicator in $indicators) {
        $process = Get-Process | Where-Object { $_.ProcessName -like "*$indicator*" }
        if ($process) {
            Write-Output "[+] Virtual Environment Detected: $indicator (Process)"
            $found = 1
        }
    }

    foreach ($indicator in $indicators) {
        $service = Get-Service | Where-Object { $_.Name -like "*$indicator*" }
        if ($service) {
            Write-Output "[+] Virtual Environment Detected: $indicator (Service)"
            $found = 1
        }
    }

    $regCheck = Get-ItemProperty -Path "HKLM:\\SYSTEM\\CurrentControlSet\\Services" -ErrorAction SilentlyContinue | Where-Object { $_ -match "vbox|vmware|virtualbox" }
    if ($regCheck) {
        Write-Output "[+] Virtual Environment Detected: Registry Keys"
        $found = 1
    }

    $biosCheck = Get-WmiObject Win32_BIOS | Where-Object { $_.SMBIOSBIOSVersion -match "vbox|vmware|virtualbox" }
    if ($biosCheck) {
        Write-Output "[+] Virtual Environment Detected: BIOS"
        $found = 1
    }

    $systemCheck = Get-WmiObject Win32_ComputerSystem | Where-Object { $_.Manufacturer -match "vmware|virtualbox" }
    if ($systemCheck) {
        Write-Output "[+] Virtual Environment Detected: System Manufacturer"
        $found = 1
    }

    $diskCheck = Get-WmiObject Win32_DiskDrive | Where-Object { $_.Model -match "vbox|vmware|virtual" }
    if ($diskCheck) {
        Write-Output "[+] Virtual Environment Detected: Disk Drive"
        $found = 1
    }

    $macAddresses = Get-WmiObject Win32_NetworkAdapterConfiguration | Where-Object { $_.MACAddress -like "00:0C:29*" -or $_.MACAddress -like "08:00:27*" }
    foreach ($mac in $macAddresses) {
        if ($mac.MACAddress -like "00:0C:29*") {
            Write-Output "[+] Virtual Environment Detected: VMware MAC Address ($($mac.MACAddress))"
            $found = 1
        }
        if ($mac.MACAddress -like "08:00:27*") {
            Write-Output "[+] Virtual Environment Detected: VirtualBox MAC Address ($($mac.MACAddress))"
            $found = 1
        }
    }

    if ($found -eq 0) {
        Write-Output "[-] No Virtual Environment Detected"
    }

    Write-Output ""
    Write-Output "[*] System Info and Persistence Mechanisms..."
    $runKey = Get-ItemProperty -Path "HKLM:\\Software\\Microsoft\\Windows\\CurrentVersion\\Run" -ErrorAction SilentlyContinue
    if ($runKey) {
        Write-Output "[+] Startup Persistence Found"
    } else {
        Write-Output "[-] No Startup Persistence Found"
    }

