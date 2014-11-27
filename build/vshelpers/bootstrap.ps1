# Get the solution directory
$solutionDir = Split-Path $dte.Solution.FullName

# Look for Boost one level above the solution
$boostRoot = Split-Path $solutionDir 
$boostRoot = Join-Path $boostRoot boost* -resolve

# If b2.exe and bjam.exe aren't in the boost directory run bootstrap.bat
$b2Path = Join-Path $boostRoot b2.exe
$bjamPath = Join-Path $boostRoot bjam.exe
$bootstrapPath = Join-Path $boostRoot bootstrap.bat
If( (Test-Path "$b2Path") -and (Test-Path "$b2Path"))
{
  Write-Warning "Found b2.exe and bjam.exe. Bootstrap already done."
}
ElseIf( (Test-Path "$bootstrapPath") )
{
  Write-Warning "Running bootstrap.bat..."
  Push-Location $boostRoot
  .\bootstrap.bat
  Pop-Location
}
Else
{
	Write-Error "Could not find b2.exe, bjam.exe, or bootstrap.bat! Have you downloaded Boost?"
}
