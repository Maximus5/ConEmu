$package = 'ConEmu'
$version = '19.10.12'


$isSytem32Bit = (($Env:PROCESSOR_ARCHITECTURE -eq 'x86') -and `
  ($Env:PROCESSOR_ARCHITEW6432 -eq $null))

$os = if ($isSytem32Bit) { "x86" } else { "x64" }

$displayName = "ConEmu $($version.replace('.','')).$os"

$installerRoot = 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Installer'
$productsRoot = "$installerRoot\UserData\S-1-5-18\Products"


# To avoid unexpected PC reboot, prohibit ConEmu uninstall from ConEmu
$is_conemu = $FALSE
try {
  & ConEmuC.exe -IsConEmu
  $is_conemu = ($LASTEXITCODE -eq 1)
} catch {
}
if ($is_conemu) {
  throw "Can't detect proper ConEmu environment variables! Please restart console!"
}


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
