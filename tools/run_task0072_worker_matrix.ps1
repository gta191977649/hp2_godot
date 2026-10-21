param(
 [Parameter(Mandatory=$true)][string]$ProjectPath,
 [Parameter(Mandatory=$true)][string]$OutputDirectory,
 [ValidateSet('parity','performance','soak')][string]$Phase='parity',
 [switch]$WithoutProfile
)
# Sequential GPU matrix. Do not run another graphical benchmark concurrently.
$ErrorActionPreference='Stop'
$portOut=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $portOut | Out-Null
$portErrors=@()
foreach($portMode in @('safe','separate')){
 foreach($portWorkers in @(0,2,4)){
  $portName="$portMode-$portWorkers"
  try {
   if($Phase -eq 'parity'){
    & "$PSScriptRoot/run_task0072_worker_parity.ps1" -ProjectPath $ProjectPath -OutputDirectory (Join-Path $portOut $portName) -PrepareWorkers $portWorkers -RenderThread $portMode
   } else {
    $portRoutes=if($Phase -eq 'soak'){@('drive_spin')}else{@('drive_spin','short_spin')}
    foreach($portRoute in $portRoutes){
     $portContext=if($portRoute -eq 'drive_spin'){'driving'}else{'viewer'}
     $portOptions=@{ProjectPath=$ProjectPath;OutputDirectory=(Join-Path $portOut "$portName-$portRoute");Context=$portContext;Route=$portRoute;Car='CORVETTE';Height=1695;Width=2880;PrepareWorkers=$portWorkers;RenderThread=$portMode;WarmFrames=120;Samples=600;Repeats=3}
     if($Phase -eq 'soak'){$portOptions.DurationSeconds=300;$portOptions.Repeats=1}
     if(-not $WithoutProfile){$portOptions.ProfileUi=$true}
     try { & "$PSScriptRoot/run_track_costs.ps1" @portOptions }
     catch { $portErrors+="$portName-$portRoute : $_";Write-Warning "Independent route failed; retaining raw results: $_" }
    }
   }
  }catch{
   $portErrors+="$portName : $_"
   Write-Warning "$portName failed; preserving capture and continuing independent configurations: $_"
  }
 }
}
@{task='TASK0072';phase=$Phase;without_profile=[bool]$WithoutProfile;errors=$portErrors;completed_without_errors=($portErrors.Count -eq 0)} | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $portOut 'matrix.json')
if($portErrors.Count){throw 'Worker matrix contains invalid/incomplete configurations; inspect matrix.json'}
