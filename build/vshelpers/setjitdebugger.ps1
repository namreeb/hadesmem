# From http://msdn.microsoft.com/en-us/library/jj919165.aspx
# Sometimes, you might need to debug the startup code for a program that is launched by another 
# process. Examples include services and custom setup actions. In these scenarios, you can have the 
# debugger launch and automatically attach when your application starts.
# 
#   Start the Registry Editor (regedit.exe).
# 
#   Navigate to the HKLM\Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options 
#   folder.
# 
#   Select the folder of the app that you want to start in the debugger.
# 
#   If the name of the app is not listed as a child folder, select Image File Execution Options 
#   and then choose New, Key on the context menu. Select the new key, choose Rename on the 
#   shortcut menu, and then enter the name of the app.
# 
#   On the context menu of the app folder, choose New, String Value.
# 
#   Change the name of the new value from New Value to debugger.
# 
#   On the context menu of the debugger entry, choose Modify.
# 
#   On the Edit String dialog box, type vsjitdebugger.exe in the Value data box.

param([string]$target,[bool]$setting)

$debugger = "vsjitdebugger.exe"

If( -not $setting )
{
	$debugger = ""
}

If( -not (Test-Path "HKLM:\Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\$target") )
{
	New-Item -Path "HKLM:\Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options" -Name $target
	Set-ItemProperty -Path "HKLM:\Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\$target" -Name debugger -Value $debugger -ErrorAction SilentlyContinue 
}
Else
{
	Set-ItemProperty -Path "HKLM:\Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\$target" -Name debugger -Value $debugger -ErrorAction SilentlyContinue 
}
Write-Host "Set HKLM:\Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\$target\debugger to '$debugger'."