param(
  [string]$Workspace = "C:\Users\Administrator\Documents\Smart Oil Press Management\Smart-Oil-Press-Management-2A10"
)

$db = Join-Path $Workspace 'out\build\Debug\compile_commands.json'
$rsp = Join-Path $Workspace 'out\build\Debug\CMakeFiles\smartoil.dir\includes_CXX.rsp'
$out = Join-Path $Workspace '.vscode\compile_commands.json'
$needle = '@CMakeFiles/smartoil.dir/includes_CXX.rsp'

if (!(Test-Path $db)) { throw "Missing: $db" }
if (!(Test-Path $rsp)) { throw "Missing: $rsp" }

$rspText = (Get-Content -Raw $rsp).Trim()
$items = Get-Content -Raw $db | ConvertFrom-Json
foreach ($i in $items) {
  $i.command = $i.command.Replace($needle, $rspText)
}

$items | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $out
Write-Host "Updated $out"
