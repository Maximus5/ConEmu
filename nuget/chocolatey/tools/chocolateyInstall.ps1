$package = 'ConEmu'
$version = '15.03.16'

try {

  $isSytem32Bit = (($Env:PROCESSOR_ARCHITECTURE -eq 'x86') -and `
    ($Env:PROCESSOR_ARCHITEW6432 -eq $null))

  $os = if ($isSytem32Bit) { "x86" } else { "x64" }

  $url = "https://downloads.sf.net/project/conemu/Alpha/ConEmuSetup.$($version.replace('.','')).exe?use_mirror=autoselect"
  #$url = "https://github.com/Maximus5/ConEmu/releases/download/v$version/ConEmuSetup.$($version.replace('.','')).exe"
  #$url = "file:///C:\ConEmu-Deploy\ConEmuSetup.$($version.replace('.','')).exe"

  $chocTemp = Join-Path $Env:TEMP 'chocolatey'
  $tempFolder = Join-Path $chocTemp "ConEmu"
  $tempInstall = Join-Path $tempFolder "ConEmuSetup.$version.exe"

  # Check if our temp folder exists
  if ([System.IO.Directory]::Exists($tempFolder)) {
    Write-Host "Temp directory '$tempFolder' exists"
  } else {
    Write-Host "Creating temp directory '$tempFolder'"
    New-Item $tempFolder -itemtype directory
  }

  # Retrieve installer from server
  Write-Host "Downloading from $url to $tempInstall"
  $params = @{
    PackageName = $package;
    fileFullPath = $tempInstall;
    url = $url;
  }
  Get-ChocolateyWebFile @params
  Write-Host "Download from $url complete"

  # MSI installer, but packed inside wrapper to select x86 or x64
  # version. Therefore, treat it as EXE type.
  $params = @{
    PackageName = $package;
    FileType = 'exe';
    SilentArgs = "/p:$os /passive";
    File = $tempInstall;
  }
  Install-ChocolateyInstallPackage @params

  # Done
  Write-ChocolateySuccess $package
} catch {
  Write-ChocolateyFailure $package "$($_.Exception.Message)"
  throw
}
