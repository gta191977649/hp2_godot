param(
 [Parameter(Mandatory=$true)][string]$ProjectPath,
 [Parameter(Mandatory=$true)][string]$OutputDirectory,
 [ValidateSet(0,2,4)][int]$PrepareWorkers=0,
 [ValidateSet('safe','separate')][string]$RenderThread='safe',
 [ValidateRange(120,720)][int]$FrameLimit=720,
 [switch]$SubmitRepair,
 [switch]$LegacyReference
)
# Port-side actual driving/image fixture; sequential GPU execution required.
$ErrorActionPreference='Stop'
$portRoot=Split-Path -Parent $PSScriptRoot
$portProject=(Resolve-Path -LiteralPath $ProjectPath).Path
$portOutput=[IO.Path]::GetFullPath($OutputDirectory)
if($LegacyReference -and ($PrepareWorkers -ne 0 -or $RenderThread -ne 'safe')){throw 'Legacy reference must use0/Safe'}
New-Item -ItemType Directory -Force -Path $portOutput | Out-Null
$portEngine=Join-Path $portRoot 'local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64.exe'
$portArgs=@('--windowed','--position','32,32','--render-thread',$RenderThread,'--disable-vsync','--rendering-method','forward_plus','--rendering-driver','vulkan','--path',$portProject,'--audio-driver','Dummy','--script','res://scripts/runtime/task0072_driving_parity.gd','--',"--driving-parity-frame-limit=$FrameLimit",("--driving-parity-output="+$portOutput),'--track-sun-gpu-resident=true',"--render-prepare-workers=$PrepareWorkers","--render-thread-requested=$RenderThread")
if($SubmitRepair){$portArgs+=@('--track-submit-math=true','--track-vehicle-depth-staging=true','--track-depth-discovery-cache=true')}
$portQuoted=@($portArgs | ForEach-Object {'"'+$_.Replace('"','\"')+'"'})
$portProcess=Start-Process -FilePath $portEngine -ArgumentList $portQuoted -WorkingDirectory $portRoot -WindowStyle Hidden -RedirectStandardOutput (Join-Path $portOutput 'run.log') -RedirectStandardError (Join-Path $portOutput 'errors.log') -PassThru
Write-Output "Parity PID=$($portProcess.Id) workers=$PrepareWorkers render=$RenderThread"
if(-not $portProcess.WaitForExit(300000)){Stop-Process -Id $portProcess.Id -Force;throw "Parity exceeded300s including load/reload: $portOutput"}
$portErrors=Get-Content -LiteralPath (Join-Path $portOutput 'errors.log') -Raw
if($portProcess.ExitCode -ne 0 -or $portErrors -match 'SCRIPT ERROR:|ERROR:'){throw "Parity process failed: $portOutput"}
$portReport=Get-Content -LiteralPath (Join-Path $portOutput 'run.json') -Raw | ConvertFrom-Json
if(-not $portReport.finished -or $portReport.error -or @($portReport.timeline).Count -ne $FrameLimit+3){throw "Incomplete parity: $portOutput"}
foreach($portSession in $portReport.sessions){
 if(-not $LegacyReference -and ($portSession.controls.render_prepare_workers -ne $PrepareWorkers -or $portSession.controls.render_thread_actual -ne $RenderThread)){throw "Requested/actual thread configuration differs: $portOutput"}
}
Write-Output "Parity completed: $portOutput"
