$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$editor = 'D:\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$project = Join-Path $projectRoot 'Project_NF.uproject'
$logRoot = Join-Path $projectRoot 'Saved\Logs'
$runTag = Get-Date -Format 'yyyyMMdd-HHmmss'
$serverLog = Join-Path $logRoot "MonsterDedicated-$runTag.log"
$shared = @('-nullrhi', '-unattended', '-nop4', '-nosound', '-nosteam')
$serverArgs = @($project, '/Game/ProjectNF/Persnal/GyeongHwan/TestLevel_GH', '-server', '-port=17877') + $shared + @('-ExecCmds="NF.Monster.VerifySpawn 9 60"', "-abslog=$serverLog")
$server = Start-Process -FilePath $editor -ArgumentList $serverArgs -WindowStyle Hidden -PassThru
$deadline = (Get-Date).AddSeconds(30)
do {
    Start-Sleep -Milliseconds 500
    $ready = (Test-Path $serverLog) -and (Select-String -Path $serverLog -Pattern 'Game Engine Initialized' -Quiet)
} until ($ready -or $server.HasExited -or (Get-Date) -gt $deadline)
if (!$ready) { throw "Server did not initialize. Inspect $serverLog" }
$clients = @()
foreach ($index in 1..2) {
    $clientLog = Join-Path $logRoot "MonsterClient$index-$runTag.log"
    $clientArgs = @($project, '127.0.0.1:17877', '-game') + $shared + @('-ExecCmds="NF.Monster.VerifySpawn 9 35"', "-abslog=$clientLog")
    $clients += Start-Process -FilePath $editor -ArgumentList $clientArgs -WindowStyle Hidden -PassThru
    Write-Output "Client $index log: $clientLog"
}
Write-Output "Server log: $serverLog"
($clients + $server) | Wait-Process -Timeout 100
Get-ChildItem $logRoot -Filter "Monster*-$runTag.log" | ForEach-Object {
    $results = @(Select-String -Path $_.FullName -Pattern 'MonsterSpawnVerify.*result=')
    $results
    if ($results.Count -eq 0 -or $results[-1].Line -notmatch 'result=PASS') {
        throw "Monster verification failed: $($_.FullName)"
    }
}
