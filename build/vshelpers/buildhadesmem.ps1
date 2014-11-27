# From http://stackoverflow.com/questions/2124753/how-i-can-use-powershell-with-the-visual-studio-command-prompt
function Set-VsCmd
{
    param(
        [parameter(Mandatory, HelpMessage="Enter VS version as 2010, 2012, or 2013")]
        [ValidateSet(2010,2012,2013)]
        [int]$version
    )
    $VS_VERSION = @{ 2010 = "10.0"; 2012 = "11.0"; 2013 = "12.0" }
    $targetDir = "c:\Program Files (x86)\Microsoft Visual Studio $($VS_VERSION[$version])\VC"
    if (!(Test-Path (Join-Path $targetDir "vcvarsall.bat"))) {
        "Error: Visual Studio $version not installed"
        return
    }
    pushd $targetDir
    cmd /c "vcvarsall.bat&set" |
    foreach {
      if ($_ -match "(.*?)=(.*)") {
        Set-Item -force -path "ENV:\$($matches[1])" -value "$($matches[2])"
      }
    }
    popd
    write-host "`nVisual Studio $version Command Prompt variables set." -ForegroundColor Yellow
}

# Get the solution directory
$solutionDir = Split-Path $dte.Solution.FullName

# Look for Boost one level above the solution
$boostRoot = Split-Path $solutionDir 
$boostRoot = Join-Path $boostRoot boost* -resolve
$Env:BOOST_ROOT=$boostRoot

# Look for HadesMem one level above the solution
$hadesmemRoot = Split-Path $solutionDir 
$hadesmemRoot = Join-Path $hadesmemRoot hades* -resolve

# Setup the Visual Studio build environment
Set-VsCmd 2013

# Find all the HadesMem build scripts and run them
Join-Path $hadesmemRoot "build\ant*\*.bat" -resolve | % { Split-Path $_ | Push-Location; & "$_"; Pop-Location}
Join-Path $hadesmemRoot "build\full\msvc\*.bat" -resolve | % { Split-Path $_ | Push-Location; & "$_"; Pop-Location}