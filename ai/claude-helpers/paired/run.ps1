# Launches the two-model console-RTS loop. The orchestrator (gamedev_loop.py) now OWNS the two
# live agent windows itself (it opens, injects into, trims and relaunches them), so this script
# only starts the orchestrator + a Game window and tears everything down when either stops.
#
#   powershell -ExecutionPolicy Bypass -File run.ps1       (or ./run from Git Bash)
# Every setting -- rounds, resume or fresh start, models, effort, tools, roles -- comes from
# config.json beside this script.
#
# 09-10-2026
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$game = Join-Path $here 'game'
$coop = Join-Path $game '.coop'
New-Item -ItemType Directory -Force -Path $game | Out-Null
New-Item -ItemType Directory -Force -Path $coop | Out-Null
Get-ChildItem -LiteralPath $coop -File -ErrorAction SilentlyContinue | Remove-Item -Force `
    -ErrorAction SilentlyContinue
$gameLive = Join-Path $game 'game.live'
if (-not (Test-Path $gameLive)) { New-Item -ItemType File -Path $gameLive | Out-Null }
$loopPy = Join-Path $env:USERPROFILE 'workspace\utils\ai\claude-helpers\gamedev_loop.py'

$gameCmd = "`$host.ui.RawUI.WindowTitle = 'Game'; Get-Content -LiteralPath '$gameLive' -Wait -Tail 2000"
$wgame = Start-Process powershell -PassThru -ArgumentList '-NoLogo', '-NoExit', '-NoProfile',
    '-Command', $gameCmd

$loopArgs = @($loopPy, '--dir', $game, '--config', (Join-Path $here 'config.json'))
$loop = Start-Process python -PassThru -WindowStyle Hidden -ArgumentList $loopArgs

Write-Host "Running with config.json. The orchestrator opens the two agent windows."
Write-Host "Context views: $game\glm.context.txt , $game\cld.context.txt"
Write-Host "Close the Game window (or an agent window) to stop everything."

# Stop when the orchestrator or the Game window exits; then kill whatever is left, including the
# agent windows the orchestrator recorded in .coop\*.winpid.
while (-not $loop.HasExited -and -not $wgame.HasExited) { Start-Sleep -Milliseconds 700 }
foreach ($f in 'impl.winpid', 'glm.winpid') {
    $p = Join-Path $coop $f
    if (Test-Path $p) {
        $wp = (Get-Content -LiteralPath $p -ErrorAction SilentlyContinue | Select-Object -First 1)
        if ($wp) { taskkill /PID $wp /T /F 2>$null | Out-Null }
    }
}
foreach ($pr in @($loop, $wgame)) { if (-not $pr.HasExited) { try { $pr.Kill() } catch {} } }
Write-Host "Stopped."
