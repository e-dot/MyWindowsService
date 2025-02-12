param([string]$BatchName = "")

$myProcessToKill = ""
try {
  $myProcessToKill = (Get-WmiObject Win32_Process -Filter "name = 'cmd.exe'" | where {$_.CommandLine -like "*$BatchName"})
}
catch {}
if ($myProcessToKill) {
  $myProcessPID = $myProcessToKill.ProcessId
  echo "Kill process $myProcessPID and its sub-processes..."
  # Use taskkill with option /T to kill sub-processes
  TASKKILL.EXE /F /T /PID $myProcessPID
} else {
  echo "$BatchName is not running."
}
