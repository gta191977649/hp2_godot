param(
    [Parameter(Mandatory=$true)][string]$ProjectPath,
    [Parameter(Mandatory=$true)][string]$OutputDirectory,
    [int[]]$Tracks=@(11),
    [ValidateSet('race','viewer','driving')][string]$Context='race',
    [ValidatePattern("^[A-Z0-9_]+$")][string]$Car="ELISE",
    [ValidateSet('all','default','p0f0a0','p0f0a1','p0f1a0','p0f1a1','p1f0a0','p1f0a1','p1f1a0','p1f1a1','sun_off','environment_off','dof_off','particles_off')][string]$Scenario='default',
    [ValidateSet('start','forest_fire','user_capture')][string]$Pose='start',
    [ValidateSet('legacy','fixed','dense','short','short_spin','rotate','drive','drive_spin')][string]$Route='legacy',
    [ValidateSet(0,90,180,270)][int[]]$Headings=@(0),
    [ValidateSet('gl_compatibility','mobile','forward_plus')][string]$RenderingMethod='forward_plus',
    [string]$RenderingDriver='vulkan',
    [ValidateSet(720,1080,1695)][int]$Height=1080,
    [ValidateRange(0,8192)][int]$Width=0,
    [ValidateRange(3,100000)][int]$WarmFrames=120,
    [ValidateRange(10,100000)][int]$Samples=600,
    [ValidateRange(0,3600)][double]$DurationSeconds=0,
    [ValidateRange(1,100)][int]$Repeats=3,
    [string[]]$Candidates=@(),
    [ValidateSet(0,2,4)][int]$PrepareWorkers=0,
    [ValidateSet("safe","separate")][string]$RenderThread="safe",
    [switch]$Fixed,
    [string]$Snapshot='',
    [switch]$Profile,
    [switch]$ProfileUi,
    [switch]$EnvironmentCapture
)
# Port diagnostic launcher. A minimized inherited console window invalidates
# GPU timing: create the actual Godot GUI process explicitly in normal mode.
$ErrorActionPreference='Stop'
if($Context -in @('viewer','driving') -and -not $PSBoundParameters.ContainsKey('Car')){$Car='CORVETTE'}
if($DurationSeconds -gt 0 -and $Context -ne 'driving'){throw 'DurationSeconds requires -Context driving'}
if($Context -eq 'driving' -and -not $PSBoundParameters.ContainsKey('Route')){$Route='drive'}
if($Context -eq 'driving' -and ($Route -notin @('drive','drive_spin') -or $Pose -ne 'start' -or $Snapshot -or $Fixed -or $Headings.Count -ne 1 -or $Headings[0] -ne 0)){throw 'Driving context owns the car/camera: use drive/drive_spin and start pose without snapshot/fixed/headings'}
if($Context -ne 'driving' -and $Route -in @('drive','drive_spin')){throw 'Drive routes require -Context driving'}
$taskRoot=Split-Path -Parent $PSScriptRoot
$taskProject=(Resolve-Path -LiteralPath $ProjectPath).Path
$taskOutput=[IO.Path]::GetFullPath($OutputDirectory)
if($EnvironmentCapture -and $Scenario -in @('all','environment_off')){throw 'Explicit capture pressure cannot be mixed with environment-off scenarios'}
if($Height -eq 1695 -and $Width -ne 2880){throw 'Height1695 requires -Width2880'}
if($Width -gt 0 -and $Width -lt 64){throw 'Explicit width must be64..8192'}
if($Fixed -and $Route -in @('dense','short','short_spin','rotate')){throw '-Fixed conflicts with dense/short routes'}
if($Headings.Count -eq 0 -or @($Headings | Select-Object -Unique).Count -ne $Headings.Count){throw 'Headings must be nonempty and unique'}
if($Pose -ne 'user_capture' -and ($Headings.Count -gt 1 -or $Headings[0] -ne 0)){throw 'Headings apply to user_capture pose only'}
if($Snapshot -and $Headings.Count -gt 1){throw 'Snapshot has a known basis; multiple synthetic headings would duplicate it'}
New-Item -ItemType Directory -Path $taskOutput -Force | Out-Null
$taskEngine=Join-Path $taskRoot 'local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64.exe'
$taskArgsBase=@('--render-thread',$RenderThread,'--disable-vsync','--windowed','--position','32,32','--rendering-method',$RenderingMethod,'--rendering-driver',$RenderingDriver,'--path',$taskProject,'--audio-driver','Dummy','--script','res://scripts/runtime/track_performance.gd','--','--track-performance-analysis','--track-optimization-profile=optimized',"--performance-scenario=$Scenario","--performance-pose=$Pose","--performance-height=$Height","--performance-route=$Route","--performance-warm=$WarmFrames","--performance-samples=$Samples","--performance-repeats=$Repeats")
$taskArgsBase+=@("--render-prepare-workers=$PrepareWorkers","--render-thread-requested=$RenderThread")
$taskArgsBase+='--performance-environment-capture='+([bool]$EnvironmentCapture).ToString().ToLowerInvariant()
if($Width -gt 0){$taskArgsBase+="--performance-width=$Width"}
$taskArgsBase+='--car='+$Car
$taskArgsBase+='--performance-context='+$Context
$taskArgsBase+='--performance-duration-seconds='+$DurationSeconds.ToString([Globalization.CultureInfo]::InvariantCulture)
foreach($taskTrack in $Tracks){$taskArgsBase+="--performance-track=$taskTrack"}
if($Fixed){$taskArgsBase+='--performance-fixed'}
if($Snapshot){$taskArgsBase+='--performance-snapshot='+ (Resolve-Path -LiteralPath $Snapshot).Path}
if($Profile -or $ProfileUi){
    $taskArgsBase=@('--gpu-profile','--profiling')+$taskArgsBase
    $taskArgsBase+='--profile-debug'
    if($ProfileUi){$taskArgsBase+='--performance-profile-ui'}else{$taskArgsBase+='--profile-debug-no-ui'}
}
foreach($taskCandidate in $Candidates){
    if($taskCandidate -notmatch '^--particle-backend=(?:native|godot_gpu)$' -and $taskCandidate -notmatch '^--(?:track-particle-(?:transport|state)-cache|track-post-consumer-cache|track-depth-discovery-cache|track-dynamic-plan-cache|track-residency-cache|track-capture-world-batches|track-vehicle-depth-staging|track-submit-math|profile-async-export|track-source-3d-skip|track-sun-reduction(?:-compare)?|track-sun-gpu-(?:resident|compare)|profile-compact-history|profile-ui-bulk-upload|sun-gpu-telemetry|track-opt-(?:opaque_queue|instancing|occlusion))=(?:true|false)$'){throw "Unsupported candidate argument: $taskCandidate"}
    $taskArgsBase+=$taskCandidate
}
# Sequential runs avoid simultaneous GPU competition. Separate directories preserve
# TASK0071's one-pose-per-document comparison keys and default output paths.
foreach($taskHeading in $Headings){
    $taskRunOutput=if($Headings.Count -gt 1){Join-Path $taskOutput ("heading-"+$taskHeading)}else{$taskOutput}
    New-Item -ItemType Directory -Path $taskRunOutput -Force | Out-Null
    if(Test-Path -LiteralPath (Join-Path $taskRunOutput 'run.json')){throw "Preserve existing results; choose a new OutputDirectory: $taskRunOutput"}
    $taskIdentity=Join-Path $taskRunOutput 'identity.json'
    & py -3 (Join-Path $PSScriptRoot 'task0077_baseline.py') identity --project $taskProject --engine $taskEngine --output $taskIdentity
    if($LASTEXITCODE -ne 0){throw 'Benchmark identity capture failed'}
    $taskArgs=$taskArgsBase+@("--performance-heading=$taskHeading",("--performance-output="+(Join-Path $taskRunOutput 'run.json')))
    $taskArgs+='--performance-identity='+$taskIdentity
    if($Profile -or $ProfileUi){$taskArgs+='--profile-debug-output='+ (Join-Path $taskRunOutput 'profile.json')}
    $taskQuoted=@($taskArgs | ForEach-Object {'"'+$_.Replace('"','\"')+'"'})
    $taskProcess=Start-Process -FilePath $taskEngine -ArgumentList $taskQuoted -WorkingDirectory $taskRoot -WindowStyle Hidden -RedirectStandardOutput (Join-Path $taskRunOutput 'run.log') -RedirectStandardError (Join-Path $taskRunOutput 'errors.log') -PassThru
    Write-Output "Track benchmark PID=$($taskProcess.Id), heading=$taskHeading, output=$taskRunOutput"
    $taskProcess.WaitForExit()
    & py -3 (Join-Path $PSScriptRoot 'task0077_baseline.py') verify $taskIdentity --output (Join-Path $taskRunOutput 'identity-check.json')
    if($LASTEXITCODE -ne 0){throw 'Benchmark input identity changed during execution'}
    Write-Output "Track benchmark exit=$($taskProcess.ExitCode)"
    $taskErrorText=Get-Content -LiteralPath (Join-Path $taskRunOutput 'errors.log') -Raw
    if($taskErrorText -match 'SCRIPT ERROR:|ERROR:'){throw "Godot error; benchmark invalid: $taskRunOutput"}
    $taskRunReport=Get-Content -LiteralPath (Join-Path $taskRunOutput 'run.json') -Raw | ConvertFrom-Json
    if(@($taskRunReport.tracks).Count -lt $Repeats*$Tracks.Count){throw "Missing benchmark repeats: $taskRunOutput"}
    foreach($taskRow in $taskRunReport.tracks){
        $taskSampleInvalid=if($DurationSeconds -gt 0){$taskRow.completed_samples -lt $Samples -or -not $taskRow.long_run_complete}else{$taskRow.completed_samples -ne $Samples}
        if($null -eq $taskRow -or -not $taskRow.timing_valid -or $taskSampleInvalid){throw "Incomplete benchmark row: $taskRunOutput"}
        if(($Profile -or $ProfileUi) -and (-not $taskRow.export_outside_measurement -or -not $taskRow.profile_exported)){throw "Profile export overlapped measurement or failed: $taskRunOutput"}
    }
    foreach($taskRow in $taskRunReport.tracks){
        if($taskRow.configuration.render_prepare_workers -ne $PrepareWorkers -or $taskRow.configuration.render_thread_actual -ne $RenderThread){throw "Requested/actual threading mismatch: $taskRunOutput"}
    }
    if($taskProcess.ExitCode -ne 0){Get-Content -LiteralPath (Join-Path $taskRunOutput 'errors.log') -Tail 12;exit $taskProcess.ExitCode}
}

