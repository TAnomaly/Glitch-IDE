@echo off
echo Creating Glitch IDE Desktop Shortcut...

set "TargetPath=%~dp0GlitchIDE.exe"
set "ShortcutPath=%userprofile%\Desktop\Glitch IDE.lnk"

powershell "$WScriptShell = New-Object -ComObject WScript.Shell; $Shortcut = $WScriptShell.CreateShortcut('%ShortcutPath%'); $Shortcut.TargetPath = '%TargetPath%'; $Shortcut.WorkingDirectory = '%~dp0'; $Shortcut.Description = 'Glitch IDE - Modern Text Editor'; $Shortcut.Save()"

echo Desktop shortcut created: %ShortcutPath%
echo You can now double-click the shortcut on your desktop to run Glitch IDE!
pause 