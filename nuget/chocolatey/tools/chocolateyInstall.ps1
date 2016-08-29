$package = 'ConEmu'
$version = '16.08.28'
$sha256 = '29E903203D0308E6621F9C0CFC587FAC5FCCFDCE71A1228EBF8AD02F1EB63F74'

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
