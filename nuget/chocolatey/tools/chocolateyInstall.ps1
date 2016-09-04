$package = 'ConEmu'
$version = '16.09.04'
$sha256 = '5D0F8ECC6BAAE7AF1EF26CC9253803728EC50DA0B3CD218E5E8832BD5ECE6083'

$isSytem32Bit = (($Env:PROCESSOR_ARCHITECTURE -eq 'x86') -and `
  ($Env:PROCESSOR_ARCHITEW6432 -eq $null))

$os = if ($isSytem32Bit) { "x86" } else { "x64" }

$url = "https://github.com/Maximus5/ConEmu/releases/download/v$version/ConEmuSetup.$($version.replace('.','')).exe"

# MSI installer, but packed inside wrapper to select x86 or x64
# version. Therefore, treat it as EXE type.
$params = @{
  PackageName = $package;
  FileType = 'exe';
  SilentArgs = "/p:$os /quiet /norestart";
  Url = $url;
  Url64bit = $url;
  checksum      = $sha256
  checksumType  = 'sha256'
  checksum64    = $sha256
  checksumType64= 'sha256'
}
Install-ChocolateyPackage @params

# Done
