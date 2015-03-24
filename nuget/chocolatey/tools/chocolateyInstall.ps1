$package = 'ConEmu'
$version = '15.03.16'


$isSytem32Bit = (($Env:PROCESSOR_ARCHITECTURE -eq 'x86') -and `
  ($Env:PROCESSOR_ARCHITEW6432 -eq $null))

$os = if ($isSytem32Bit) { "x86" } else { "x64" }

$url = "https://downloads.sf.net/project/conemu/Alpha/ConEmuSetup.$($version.replace('.','')).exe?use_mirror=autoselect"
#$url = "https://github.com/Maximus5/ConEmu/releases/download/v$version/ConEmuSetup.$($version.replace('.','')).exe"
#$url = "file:///C:\ConEmu-Deploy\ConEmuSetup.$($version.replace('.','')).exe"

# MSI installer, but packed inside wrapper to select x86 or x64
# version. Therefore, treat it as EXE type.
$params = @{
  PackageName = $package;
  FileType = 'exe';
  SilentArgs = "/p:$os /passive";
  File = $tempInstall;
}
Install-ChocolateyPackage @params

# Done
