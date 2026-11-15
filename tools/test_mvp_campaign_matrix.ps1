param(
    [ValidateSet('release','debug')][string]$Configuration = 'release',
    [ValidateRange(1,16)][int]$Jobs = 3
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$build = Join-Path $repo "build/godot-mvp-$Configuration"
$engine = Join-Path $repo 'local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe'
$project = Join-Path $build 'mvp-project'
$smoke = Join-Path $PSScriptRoot 'test_mvp_campaign_drive.gd'
$source = $env:HP2_PS2_ASSET_ROOT
if (-not $source) { throw 'Set HP2_PS2_ASSET_ROOT to a complete local game source.' }
$source = (Resolve-Path -LiteralPath $source).Path
$catalogueTool = Join-Path $build 'mvp_race_catalog.exe'
foreach ($required in @($engine, $catalogueTool, (Join-Path $project 'project.godot'))) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) { throw "Missing $required" }
}
$run = (New-Item -ItemType Directory -Path (Join-Path $repo ('local/campaign-matrix-' + [guid]::NewGuid()))).FullName
$catalogue = & $catalogueTool $source
if ($LASTEXITCODE) { throw 'The authored race catalogue could not be read.' }
$catalogue | Set-Content (Join-Path $run 'catalogue.csv')
$cases = foreach ($line in $catalogue) {
    $fields = $line.Split(',')
    if ($fields[0] -ne 'career' -or $fields[4] -ne '1') { continue }
    $tree = [int]$fields[1]
    [pscustomobject]@{
        Tree = @('championship','ultimate-racer')[$tree]
        GameType = 8 + $tree
        Event = [int]$fields[2]
        Track = [int]$fields[5]
        Reverse = [int]$fields[6]
        Attempt = 1
    }
}
if (-not $cases) { throw 'The authored catalogue contains no campaign entries.' }
$runCases = {
    $case = $_
    $name = "$($case.Tree)-$($case.Event)"
    $log = Join-Path $using:run "$name-$($case.Attempt).log"
    $launch = [System.Diagnostics.ProcessStartInfo]::new($using:engine)
    $launch.UseShellExecute = $false
    $launch.CreateNoWindow = $true
    $launch.Environment['HP2_PS2_ASSET_ROOT'] = $using:source
    $launch.Environment['HP2_MVP_TEST_PROFILE'] = '1'
    foreach ($argument in @('--headless','--path',$using:project,'--max-fps','120',
            '--log-file',$log,'--script',$using:smoke,'--',"--race-event=$($case.Tree):$($case.Event)")) {
        $launch.ArgumentList.Add($argument)
    }
    $launch.RedirectStandardOutput = $true
    $launch.RedirectStandardError = $true
    $process = [System.Diagnostics.Process]::Start($launch)
    try {
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        if (-not $process.WaitForExit(60000)) {
            $process.Kill($true)
            $process.WaitForExit()
        }
        $code = $process.ExitCode
        Set-Content -LiteralPath $log -Value ($stdout.Result + $stderr.Result)
    } finally {
        $process.Dispose()
    }
    $scene = "stage=scene_publish track=$($case.Track) .* reverse=$($case.Reverse) game_type=$($case.GameType) event=$($case.Event)$"
    $drive = "HP2_MVP_CAMPAIGN_DRIVE track=$($case.Track) event=$($case.Event) "
    [pscustomobject]@{
        Tree = $case.Tree
        GameType = $case.GameType
        Event = $case.Event
        Track = $case.Track
        Reverse = $case.Reverse
        Attempt = $case.Attempt
        Exit = $code
        Drove = $code -eq 0 -and (Select-String -Quiet $scene $log) -and (Select-String -Quiet $drive $log)
        Log = $log
    }
}
$results = @($cases | ForEach-Object -ThrottleLimit $Jobs -Parallel $runCases)
$retry = @($results | Where-Object { -not $_.Drove } | Select-Object Tree,GameType,Event,Track,Reverse,Attempt)
if ($retry) {
    $retry | ForEach-Object { $_.Attempt = 2 }
    $results += @($retry | ForEach-Object -ThrottleLimit 1 -Parallel $runCases)
}
$results | Sort-Object Tree,Event | Export-Csv -NoTypeInformation (Join-Path $run 'results.csv')
$passed = @($results | Where-Object Drove).Count
Write-Output "Drove $passed/$($cases.Count) authored campaign events. Results in $run"
if ($passed -ne $cases.Count) {
    $results | Group-Object Tree,Event | Where-Object { -not ($_.Group | Where-Object Drove) } |
        ForEach-Object { $_.Group[-1] } | Format-Table Tree,Event,Track,Exit,Log
    exit 1
}
