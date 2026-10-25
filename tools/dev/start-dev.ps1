# Dev launcher: checks dependencies, links the built mod into the game, starts the server and the game.
# Usage: start-dev.cmd [-Build] [-NoServer] [-NoGame] [-GamePath <path to Cyberpunk2077.exe>] [-Ip 127.0.0.1] [-Port 11778]
param(
    [switch]$Build,
    [switch]$NoServer,
    [switch]$NoGame,
    [string]$GamePath,
    [string]$Ip = '127.0.0.1',
    [int]$Port = 11778
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$dist = Join-Path $repo 'dist\launcher'
$localConfig = Join-Path $repo 'dist\dev-settings.json'

function Info($msg) { Write-Host "[*] $msg" -ForegroundColor Cyan }
function Ok($msg)   { Write-Host "[+] $msg" -ForegroundColor Green }
function Warn($msg) { Write-Host "[!] $msg" -ForegroundColor Yellow }
function Fail($msg) { Write-Host "[x] $msg" -ForegroundColor Red; Read-Host 'Нажмите Enter, чтобы закрыть'; exit 1 }

function Load-Settings {
    if (Test-Path $localConfig) {
        try { return Get-Content $localConfig -Raw | ConvertFrom-Json } catch { }
    }
    return [pscustomobject]@{}
}

function Save-Settings($settings) {
    New-Item -ItemType Directory -Force (Split-Path $localConfig) | Out-Null
    $settings | ConvertTo-Json | Set-Content -Encoding UTF8 $localConfig
}

function Set-Setting($settings, $name, $value) {
    $settings | Add-Member -NotePropertyName $name -NotePropertyValue $value -Force
}

# --- 1. Find the game -------------------------------------------------------

function Find-GameInSteam {
    $steam = (Get-ItemProperty -Path 'HKCU:\Software\Valve\Steam' -ErrorAction SilentlyContinue).SteamPath
    if (-not $steam) { return $null }

    $libraries = @($steam)
    $vdf = Join-Path $steam 'steamapps\libraryfolders.vdf'
    if (Test-Path $vdf) {
        foreach ($m in [regex]::Matches((Get-Content $vdf -Raw), '"path"\s+"([^"]+)"')) {
            $libraries += $m.Groups[1].Value.Replace('\\', '\')
        }
    }

    foreach ($lib in $libraries | Select-Object -Unique) {
        $exe = Join-Path $lib 'steamapps\common\Cyberpunk 2077\bin\x64\Cyberpunk2077.exe'
        if (Test-Path $exe) { return $exe }
    }
    return $null
}

$settings = Load-Settings

if (-not $GamePath) { $GamePath = $settings.GamePath }
if (-not $GamePath -or -not (Test-Path $GamePath)) { $GamePath = Find-GameInSteam }
if (-not $GamePath -or -not (Test-Path $GamePath)) {
    Fail "Игра не найдена. Запустите так: start-dev.cmd -GamePath `"<папка игры>\bin\x64\Cyberpunk2077.exe`""
}
Set-Setting $settings 'GamePath' $GamePath
Save-Settings $settings

$gameRoot = (Resolve-Path (Join-Path (Split-Path $GamePath) '..\..')).Path
$gameVersion = (Get-Item $GamePath).VersionInfo.ProductVersion
Ok "Игра: $gameRoot (версия $gameVersion)"

# --- 2. Check dependencies --------------------------------------------------

# Minimum versions for patch 2.5, see CLAUDE.md
$deps = @(
    @{ Name = 'RED4ext';      File = 'red4ext\RED4ext.dll';                         Min = '1.31.0' },
    @{ Name = 'RED4ext (загрузчик)'; File = 'bin\x64\winmm.dll';                     Min = $null },
    @{ Name = 'Redscript';    File = 'engine\tools\scc.exe';                        Min = '0.5.32' },
    @{ Name = 'ArchiveXL';    File = 'red4ext\plugins\ArchiveXL\ArchiveXL.dll';     Min = '1.28.0' },
    @{ Name = 'TweakXL';      File = 'red4ext\plugins\TweakXL\TweakXL.dll';         Min = '1.12.0' },
    @{ Name = 'Codeware';     File = 'red4ext\plugins\Codeware\Codeware.dll';       Min = '1.21.0' },
    @{ Name = 'Input Loader'; File = 'red4ext\plugins\input_loader\input_loader.dll'; Min = '0.2.3' }
)

function Get-FileVersion($path) {
    $info = (Get-Item $path).VersionInfo
    foreach ($raw in @($info.ProductVersion, $info.FileVersion)) {
        if ($raw -match '(\d+)\.(\d+)\.(\d+)') { return [version]"$($Matches[1]).$($Matches[2]).$($Matches[3])" }
    }
    return $null
}

$missing = @()
foreach ($dep in $deps) {
    $path = Join-Path $gameRoot $dep.File
    if (-not (Test-Path $path)) {
        $missing += $dep.Name
        Warn "$($dep.Name): НЕ найден ($($dep.File))"
        continue
    }
    if (-not $dep.Min) { Ok "$($dep.Name): есть"; continue }

    $version = Get-FileVersion $path
    if (-not $version) {
        Warn "$($dep.Name): есть, версию определить не удалось (нужна $($dep.Min)+)"
    } elseif ($version -lt [version]$dep.Min) {
        Warn "$($dep.Name): версия $version, нужна $($dep.Min)+ (обновите)"
    } else {
        Ok "$($dep.Name): $version"
    }
}
if ($missing.Count -gt 0) {
    Fail "Не хватает модов: $($missing -join ', '). Распакуйте их архивы в $gameRoot и запустите снова."
}

# --- 3. Build (optional) and link the mod -----------------------------------

if ($Build) {
    Info 'Сборка и раскладка клиента (xmake)...'
    Push-Location $repo
    try {
        $env:Path = [Environment]::GetEnvironmentVariable('Path', 'Machine') + ';' + [Environment]::GetEnvironmentVariable('Path', 'User')
        & xmake build -y Client
        if ($LASTEXITCODE -ne 0) { Fail 'Сборка упала, смотрите вывод выше.' }
        & xmake install -y -o dist Client redscript Archives Inputs Tweaks Server.Native Server.Scripting Server.Loader
        if ($LASTEXITCODE -ne 0) { Fail 'xmake install упал, смотрите вывод выше.' }
    } finally { Pop-Location }
}

$modSource = Join-Path $dist 'mod'
if (-not (Test-Path (Join-Path $modSource 'CyberpunkMP.dll'))) {
    Fail "Нет собранного мода в $modSource. Запустите start-dev.cmd -Build"
}

$modTarget = Join-Path $gameRoot 'red4ext\plugins\zzzCyberpunkMP'
if (Test-Path $modTarget) {
    $item = Get-Item $modTarget -Force
    if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
        # Our previous link, safe to replace
        cmd /c rmdir "$modTarget" | Out-Null
    } else {
        Fail "$modTarget - обычная папка, а не ссылка. Удалите или переименуйте её сами (скрипт не трогает чужие файлы) и запустите снова."
    }
}
cmd /c mklink /J "$modTarget" "$modSource" | Out-Null
if (-not (Test-Path (Join-Path $modTarget 'CyberpunkMP.dll'))) { Fail "Не удалось создать ссылку $modTarget" }
Ok "Мод подключён: $modTarget -> $modSource"

# --- 4. Server ---------------------------------------------------------------

if (-not $NoServer) {
    $serverExe = Join-Path $dist 'server\Server.Loader.exe'
    if (-not (Test-Path $serverExe)) { Fail "Нет сервера в $serverExe. Запустите start-dev.cmd -Build" }

    if (Get-Process -Name 'Server.Loader' -ErrorAction SilentlyContinue) {
        Ok 'Сервер уже запущен'
    } else {
        if (-not $settings.AdminPassword) {
            Set-Setting $settings 'AdminPassword' ([guid]::NewGuid().ToString('N').Substring(0, 12))
            Save-Settings $settings
        }
        $env:CYBERPUNKMP_ADMIN_USERNAME = 'admin'
        $env:CYBERPUNKMP_ADMIN_PASSWORD = $settings.AdminPassword
        Start-Process -FilePath $serverExe -WorkingDirectory (Split-Path $serverExe)
        Ok "Сервер запущен в отдельном окне (админка: admin / $($settings.AdminPassword))"
        Start-Sleep -Seconds 3
    }
}

# --- 5. Game -----------------------------------------------------------------

if (-not $NoGame) {
    if (Get-Process -Name 'Cyberpunk2077' -ErrorAction SilentlyContinue) {
        Fail 'Игра уже запущена. Закройте её и запустите скрипт снова.'
    }
    if (-not (Get-Process -Name 'steam' -ErrorAction SilentlyContinue)) {
        Warn 'Steam не запущен. Если игра перезапустится через Steam, аргументы --online/--ip потеряются.'
    }
    $gameArgs = @('--online', "--ip=$Ip", "--port=$Port")
    Start-Process -FilePath $GamePath -ArgumentList $gameArgs -WorkingDirectory (Split-Path $GamePath)
    Ok "Игра запущена: $($gameArgs -join ' ')"
}

Write-Host ''
Write-Host 'Логи: red4ext\logs, red4ext\plugins\zzzCyberpunkMP\CyberpunkMP.log, r6\logs\redscript_rCURRENT.log' -ForegroundColor DarkGray
Start-Sleep -Seconds 5
