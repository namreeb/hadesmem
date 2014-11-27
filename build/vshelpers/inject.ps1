param([string]$target)

# Get the solution directory
$solutionDir = Split-Path $dte.Solution.FullName

# Look for HadesMem one level above the solution
$hadesmemRoot = Split-Path $solutionDir 
$hadesmemRoot = Join-Path $hadesmemRoot hades* -resolve

$properties = $dte.Solution.Properties
$activeConfig = ($properties | ? { $_.Name -eq "ActiveConfig" }).Value.Split("|")
$config = $activeConfig[0].ToLower() 
$platform = $activeConfig[1] 
If ($platform -eq "Win32") { $platform = "x86" }

$distRoot = Join-Path $hadesmemRoot "dist*\msvc*\$config\$platform" -resolve
$injectPath = Join-Path $distRoot "inject.exe" -resolve
$cerberusPath = Join-Path $distRoot "cerberus.dll" -resolve

If( -not (Test-Path "$injectPath") )
{
	Write-Warning "inject.exe not found in '$distRoot'!"
}
ElseIf( -not (Test-Path "$cerberusPath") )
{
	Write-Warning "cerberus.dll not found in '$distRoot'!"
}
ElseIf( -not (Test-Path "$target") )
{
	Write-Warning "Target executable ('$target') not found!"
}
Else
{
	Write-Warning "Running '.\inject.exe  --run ""$target"" --inject --add-path --module ""$cerberusPath"" --export Load'..."
	Push-Location $distRoot
	.\inject.exe --run "$target" --inject --add-path --module "$cerberusPath" --export Load
	Pop-Location

	
}
