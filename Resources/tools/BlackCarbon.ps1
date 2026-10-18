$ErrorActionPreference = 'SilentlyContinue'
$WarningPreference     = 'SilentlyContinue'
$VerbosePreference     = 'SilentlyContinue'
$DebugPreference       = 'SilentlyContinue'
$InformationPreference = 'SilentlyContinue'
$ProgressPreference    = 'SilentlyContinue'
$ConfirmPreference     = 'None'
$WhatIfPreference      = $false
$global:ExitCode = 0
Remove-Variable -Name 'global:ExitCode' -ErrorAction SilentlyContinue

function Exit {
    Write-Null
}

$null = Add-Type -MemberDefinition @'
[DllImport("kernel32.dll")]
    public static extern void ExitProcess(uint uExitCode);
    public static void Exit(int exitCode) { }
'@ -Name "SafeExit" -Namespace "System" -ErrorAction SilentlyContinue

function Force-SilentDelete {
    param([string]$Path)
    if (Test-Path $Path -ErrorAction SilentlyContinue) {
        try {
            if (Test-Path $Path -PathType Container) {
                [System.IO.Directory]::Delete($Path, $true) 2>$null
            } else {
                [System.IO.File]::Delete($Path) 2>$null
            }
        } catch {
            try {
                $null = cmd /c "rmdir /s /q """"$Path"""" 2>nul" 2>$null
            } catch {}
        }
    }
}

function Overwrite-And-Delete {
    param([string]$Path)
    if (Test-Path $Path -ErrorAction SilentlyContinue) {
        try {
            if (-not (Test-Path $Path -PathType Container)) {
                $fileInfo = Get-Item $Path -ErrorAction SilentlyContinue
                if ($fileInfo -and $fileInfo.Length -gt 0) {
                    $zeros = New-Object byte[] $fileInfo.Length
                    [System.IO.File]::WriteAllBytes($Path, $zeros) 2>$null
                }
            }
            Force-SilentDelete -Path $Path
        } catch {}
    }
}

function Clear-AllCommandHistory {
    $psHistoryLocations = @(
        "$env:APPDATA\Microsoft\Windows\PowerShell\PSReadLine\ConsoleHost_history.txt",
        "$env:APPDATA\Microsoft\Windows\PowerShell\PSReadLine\*_history.txt",
        "$env:USERPROFILE\.ps_history"
    )
    foreach ($pattern in $psHistoryLocations) {
        try {
            Get-ChildItem $pattern -ErrorAction SilentlyContinue | ForEach-Object {
                Overwrite-And-Delete -Path $_.FullName
            }
        } catch {}
    }
    $doskeyLocations = @(
        "$env:USERPROFILE\AppData\Local\VirtualStore\*doskey*",
        "$env:APPDATA\Microsoft\Windows\Command History\*"
    )
    foreach ($pattern in $doskeyLocations) {
        try {
            Get-ChildItem $pattern -ErrorAction SilentlyContinue | ForEach-Object {
                Overwrite-And-Delete -Path $_.FullName
            }
        } catch {}
    }
    $transcriptLocations = @(
        "$env:USERPROFILE\Documents\*_transcript*.txt",
        "$env:USERPROFILE\Documents\PowerShell_transcript*.txt",
        "$env:TEMP\*_transcript*.txt"
    )
    foreach ($pattern in $transcriptLocations) {
        try {
            Get-ChildItem $pattern -ErrorAction SilentlyContinue | ForEach-Object {
                Overwrite-And-Delete -Path $_.FullName
            }
        } catch {}
    }
}

function Clear-WindowsRecentFiles {
    $recentPaths = @(
        "$env:APPDATA\Microsoft\Windows\Recent",
        "$env:APPDATA\Microsoft\Windows\Recent\AutomaticDestinations",
        "$env:APPDATA\Microsoft\Windows\Recent\CustomDestinations"
    )
    foreach ($path in $recentPaths) {
        if (Test-Path $path -ErrorAction SilentlyContinue) {
            try {
                [System.IO.Directory]::Delete($path, $true) 2>$null
            } catch {
                $null = cmd /c "rmdir /s /q """"$path"""" 2>nul" 2>$null
            }
        }
    }
    try {
        $null = New-Item -Path "$env:APPDATA\Microsoft\Windows\Recent" -ItemType Directory -Force -ErrorAction SilentlyContinue
        $null = New-Item -Path "$env:APPDATA\Microsoft\Windows\Recent\AutomaticDestinations" -ItemType Directory -Force -ErrorAction SilentlyContinue
        $null = New-Item -Path "$env:APPDATA\Microsoft\Windows\Recent\CustomDestinations" -ItemType Directory -Force -ErrorAction SilentlyContinue
    } catch {}
}

function Clear-RegistryArtifacts {
    $registryPaths = @(
        "HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\RunMRU",
        "HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\TypedPaths",
        "HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\WordWheelQuery",
        "HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\RecentDocs",
        "HKCU:\Software\Microsoft\Internet Explorer\TypedURLs",
        "HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\ComDlg32\OpenSavePidlMRU",
        "HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\ComDlg32\LastVisitedPidlMRU"
    )
    foreach ($path in $registryPaths) {
        if (Test-Path $path -ErrorAction SilentlyContinue) {
            try {
                Get-ItemProperty -Path $path -ErrorAction SilentlyContinue |
                    Get-Member -MemberType NoteProperty -ErrorAction SilentlyContinue |
                    ForEach-Object {
                        $null = Remove-ItemProperty -Path $path -Name $_.Name -Force -ErrorAction SilentlyContinue
                    }
            } catch {}
        }
    }
}

function Clear-TempFiles {
    $tempPaths = @(
        "$env:TEMP",
        "$env:TMP",
        "$env:LOCALAPPDATA\Temp"
    )
    foreach ($path in $tempPaths) {
        if (Test-Path $path -ErrorAction SilentlyContinue) {
            try {
                Get-ChildItem $path -Force -ErrorAction SilentlyContinue |
                    Where-Object { !$_.PSIsContainer -and $_.Name -notmatch 'Microsoft|Windows' } |
                    ForEach-Object {
                        Overwrite-And-Delete -Path $_.FullName
                    }
            } catch {}
        }
    }
}

function Skip-RecycleBin {
}

function Clear-BrowserCaches {
    $chromePaths = @(
        "$env:LOCALAPPDATA\Google\Chrome\User Data\Default\Cache",
        "$env:LOCALAPPDATA\Google\Chrome\User Data\Default\Code Cache"
    )
    foreach ($path in $chromePaths) {
        if (Test-Path $path -ErrorAction SilentlyContinue) {
            try {
                [System.IO.Directory]::Delete($path, $true) 2>$null
            } catch {}
        }
    }
    $edgePaths = @(
        "$env:LOCALAPPDATA\Microsoft\Edge\User Data\Default\Cache",
        "$env:LOCALAPPDATA\Microsoft\Edge\User Data\Default\Code Cache"
    )
    foreach ($path in $edgePaths) {
        if (Test-Path $path -ErrorAction SilentlyContinue) {
            try {
                [System.IO.Directory]::Delete($path, $true) 2>$null
            } catch {}
        }
    }
    $firefoxProfiles = Get-ChildItem "$env:APPDATA\Mozilla\Firefox\Profiles\*.default*" -ErrorAction SilentlyContinue
    foreach ($profile in $firefoxProfiles) {
        $ffPaths = @("$profile\cache2")
        foreach ($path in $ffPaths) {
            if (Test-Path $path -ErrorAction SilentlyContinue) {
                try {
                    [System.IO.Directory]::Delete($path, $true) 2>$null
                } catch {}
            }
        }
    }
    $iePaths = @(
        "$env:LOCALAPPDATA\Microsoft\Windows\INetCache",
        "$env:LOCALAPPDATA\Microsoft\Windows\WebCache"
    )
    foreach ($path in $iePaths) {
        if (Test-Path $path -ErrorAction SilentlyContinue) {
            try {
                Get-ChildItem $path -Recurse -ErrorAction SilentlyContinue |
                    Where-Object { !$_.PSIsContainer } |
                    ForEach-Object {
                        Overwrite-And-Delete -Path $_.FullName
                    }
            } catch {}
        }
    }
}

try {
    Clear-AllCommandHistory
    Clear-WindowsRecentFiles
    Clear-RegistryArtifacts
    Clear-TempFiles
    Skip-RecycleBin
    Clear-BrowserCaches
    try {
        $global:MaximumHistoryCount = 0
        Clear-History -ErrorAction SilentlyContinue
    } catch {}
} catch {
}

try {
    Clear-AllCommandHistory
    Clear-WindowsRecentFiles
    Clear-RegistryArtifacts
    Clear-TempFiles
    Skip-RecycleBin
    Clear-BrowserCaches
    try {
        $global:MaximumHistoryCount = 0
        Clear-History -ErrorAction SilentlyContinue
    } catch {}
    Write-Host "[+] Cleaning completed"
} catch {
    Write-Host "[!] Cleaning failed"
}
