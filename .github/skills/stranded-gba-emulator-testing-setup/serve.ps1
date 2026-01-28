$port = 8080
$listener = New-Object System.Net.HttpListener
$listener.Prefixes.Add("http://localhost:$port/")
$listener.Start()

Write-Host "Server running at http://localhost:$port/"
Write-Host "Press Ctrl+C to stop"

while ($listener.IsListening) {
    $context = $listener.GetContext()
    $request = $context.Request
    $response = $context.Response
    
    $path = if ($request.Url.LocalPath -eq '/') { '/index.html' } else { $request.Url.LocalPath }
    $fullPath = Join-Path $PSScriptRoot $path.TrimStart('/')
    
    Write-Host "$($request.HttpMethod) $($request.Url.LocalPath)"
    
    if (Test-Path $fullPath -PathType Leaf) {
        $fileInfo = Get-Item $fullPath
        $ext = [System.IO.Path]::GetExtension($fullPath).ToLower()
        
        $contentType = switch ($ext) {
            '.html' { 'text/html' }
            '.css'  { 'text/css' }
            '.js'   { 'application/javascript' }
            '.json' { 'application/json' }
            '.gba'  { 'application/octet-stream' }
            default { 'application/octet-stream' }
        }
        
        $response.ContentType = $contentType
        $response.ContentLength64 = $fileInfo.Length
        
        if ($request.HttpMethod -ne 'HEAD') {
            $content = [System.IO.File]::ReadAllBytes($fullPath)
            $response.OutputStream.Write($content, 0, $content.Length)
        }
    } else {
        $response.StatusCode = 404
        if ($request.HttpMethod -ne 'HEAD') {
            $errorMsg = [System.Text.Encoding]::UTF8.GetBytes("404 Not Found")
            $response.OutputStream.Write($errorMsg, 0, $errorMsg.Length)
        }
    }
    
    $response.Close()
}

$listener.Stop()
