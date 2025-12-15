$trackerPath = "backend\build\tracker.exe"
$sourcePath = "backend\main.cpp"
$buildDir = "backend\build"

Write-Host "Starting Development Mode (Hot Reload)" -ForegroundColor Green
Write-Host "Watching $sourcePath for changes..." -ForegroundColor Gray

# Initial Run
Write-Host "Initial Build..."
cmake --build $buildDir --config Release --target tracker
if ($LASTEXITCODE -eq 0) {cd
    Stop-Process -Name "tracker" -ErrorAction SilentlyContinue
    Start-Process $trackerPath -NoNewWindow
}

$lastWrite = (Get-Item $sourcePath).LastWriteTime

while ($true) {
    Start-Sleep -Seconds 1
    $currentWrite = (Get-Item $sourcePath).LastWriteTime
    
    if ($currentWrite -ne $lastWrite) {
        Write-Host "`nDetected change in main.cpp. Rebuilding..." -ForegroundColor Yellow
        $lastWrite = $currentWrite
        
        # Kill existing process
        Stop-Process -Name "tracker" -ErrorAction SilentlyContinue
        
        # Rebuild
        cmake --build $buildDir --config Release --target tracker
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "Build Successful. Restarting Server..." -ForegroundColor Green
            Start-Process $trackerPath -NoNewWindow
        } else {
            Write-Host "Build Failed. Waiting for fix..." -ForegroundColor Red
        }
    }
}
