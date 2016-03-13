$package = 'ConEmu'
$version = '16.03.13'


$isSytem32Bit = (($Env:PROCESSOR_ARCHITECTURE -eq 'x86') -and `
  ($Env:PROCESSOR_ARCHITEW6432 -eq $null))

$os = if ($isSytem32Bit) { "x86" } else { "x64" }

$displayName = "ConEmu $($version.replace('.','')).$os"

$installerRoot = 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Installer'
$productsRoot = "$installerRoot\UserData\S-1-5-18\Products"

Write-Host "Searching for installed msi-packet..."

# ConEmu packages do not have fixed GUIDs
$pkg = ""
Get-ChildItem $productsRoot |
  % { ($productsRoot + "\" + (Split-Path -Leaf $_.Name) + "\InstallProperties") } |
  ? { ($pkg -eq "") -And (Test-Path $_) } |
  ? { ((Get-ItemProperty $_).DisplayName -eq $displayName) } |
  % {
    $pkg = (Get-ItemProperty $_).LocalPackage
  }
if ($pkg -ne "") {
  # Write-Host "Found at $pkg"
  Write-Host "Uninstalling '$displayName' via 'msiexec /x $pkg'"
  Start-Process -Wait msiexec -ArgumentList @("/x", $pkg, "/qb-!")
}

# Done
