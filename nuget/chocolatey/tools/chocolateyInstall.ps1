$package = 'ConEmu'
$version = '15.04.09'
$stage   = 'Preview'


$isSytem32Bit = (($Env:PROCESSOR_ARCHITECTURE -eq 'x86') -and `
  ($Env:PROCESSOR_ARCHITEW6432 -eq $null))

$os = if ($isSytem32Bit) { "x86" } else { "x64" }

$url = "https://downloads.sf.net/project/conemu/$stage/ConEmuSetup.$($version.replace('.','')).exe?use_mirror=autoselect"

# MSI installer, but packed inside wrapper to select x86 or x64
# version. Therefore, treat it as EXE type.
$params = @{
  PackageName = $package;
  FileType = 'exe';
  SilentArgs = "/p:$os /passive";
  Url = $url;
  Url64bit = $url;
}
Install-ChocolateyPackage @params

# Done
