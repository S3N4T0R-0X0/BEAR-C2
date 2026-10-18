function Take-Screenshot {
    try {
        Add-Type -AssemblyName System.Windows.Forms
        Add-Type -AssemblyName System.Drawing
        $screen = [System.Windows.Forms.SystemInformation]::VirtualScreen
        $width = $screen.Width
        $height = $screen.Height
        $left = $screen.X
        $top = $screen.Y
        $bitmap = New-Object System.Drawing.Bitmap($width, $height)
        $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
        $graphics.CopyFromScreen($left, $top, 0, 0, $bitmap.Size)
        $memoryStream = New-Object System.IO.MemoryStream
        $bitmap.Save($memoryStream, [System.Drawing.Imaging.ImageFormat]::Png)
        $base64 = [System.Convert]::ToBase64String($memoryStream.ToArray())
        $graphics.Dispose()
        $bitmap.Dispose()
        $memoryStream.Dispose()
        return "screenshot-$base64"
    }
    catch {
        return "[-] Failed to take screenshot: $($_.Exception.Message)"
    }
}
Take-Screenshot
