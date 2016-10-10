param([string]$build="")

# This file will update PortableApps and Chocolatey versions

$Script_File_path = split-path -parent $MyInvocation.MyCommand.Definition
$NuSpec = Join-Path $Script_File_path "..\nuget\chocolatey\ConEmu.nuspec"
$NuInstall = Join-Path $Script_File_path "..\nuget\chocolatey\tools\chocolateyInstall.ps1"
$NuUnInstall = Join-Path $Script_File_path "..\nuget\chocolatey\tools\chocolateyUninstall.ps1"
$ConEmuCoreNuget = Join-Path $Script_File_path "..\nuget\ConEmu.Core\ConEmu.Core.nuspec"

if ($build -eq "") {
  $build = $env:CurVerBuild
}

if (($build -eq $null) -Or (-Not ($build -match "\b\d\d\d\d\d\d(\w*)\b"))) {
  Write-Host -ForegroundColor Red "Build was not passed as argument!"
  $host.SetShouldExit(101)
  exit 
}

$ConEmuSetup = Join-Path $Script_File_path "..\..\ConEmu-Deploy\Setup\ConEmuSetup.$build.exe"

# PowerShell v4 function
$sha = (Get-FileHash $ConEmuSetup -Algorithm sha256).Hash
# External tool
# $sha = & \Chocolatey\lib\checksum\tools\checksum.exe -t sha256 -f "ConEmuSetup.160724.exe"
if (($sha -eq $null) -Or ($sha -eq "")) {
  Write-Host -ForegroundColor Red "Sha256 calculation failed for '$ConEmuSetup'"
  $host.SetShouldExit(101)
  exit 
}


$ver4n = 0; $ver4 = "";
if ($build.Length -gt 6) {
$ver4 = $build.SubString(6,1)
$ver4n = ([byte][char]$ver4) - ([byte][char]'a') + 1
}

$build_dot3 = "$($build.SubString(0,2)).$($build.SubString(2,2)).$($build.SubString(4,2))$ver4"
$build_year3 = "20$($build.SubString(0,2)).$($build.SubString(2,2).TrimStart('0')).$($build.SubString(4,2).TrimStart('0'))$ver4"
$build_dot4 = "$($build.SubString(0,2)).$($build.SubString(2,2).TrimStart('0')).$($build.SubString(4,2).TrimStart('0')).$ver4n"
$build_comma = "$($build.SubString(0,2)),$($build.SubString(2,2).TrimStart('0')),$($build.SubString(4,2).TrimStart('0')),$ver4n"

Write-Host -ForegroundColor Yellow "Updating files with build# $build"


#
# Helper to update NuSpec-s
#
function NuSpec-SetBuild([string]$XmlFile) {
  Write-Host -ForegroundColor Green $XmlFile
  $xml = Get-Content -Raw $XmlFile -Encoding UTF8
  $m = ([regex]'<version>\d+\.\d+\.\d+\..<\/version>').Matches($xml)
  if (($m -eq $null) -Or ($m.Count -ne 1)) {
    Write-Host -ForegroundColor Red "Proper <version>...</version> tag was not found in:`r`n$XmlFile"
    $host.SetShouldExit(101)
    return $FALSE
  }
  $xml = $xml.Replace($m[0].Value, "<version>$build_dot4</version>").Trim()
  Set-Content $XmlFile $xml -Encoding UTF8
  return $TRUE
}



#
# Chocolatey.org : chocolatey\ConEmu.nuspec
#
if (-Not (NuSpec-SetBuild $NuSpec)) {
  exit 
}


# chocolatey\tools\chocolateyInstall.ps1
# chocolatey\tools\chocolateyUninstall.ps1
@($NuInstall, $NuUnInstall) | % {
  Write-Host -ForegroundColor Green $_
  $txt = Get-Content -Raw $_

  # Example: $version = '16.07.24'
  $m = ([regex]"[$]version = '\d+\.\d+\.\d+\w?'").Matches($txt)
  if (($m -eq $null) -Or ($m.Count -ne 1)) {
    Write-Host -ForegroundColor Red "Proper `$version define was not found in:`r`n$_"
    $host.SetShouldExit(101)
    exit 
  }
  $txt = $txt.Replace($m[0].Value, "`$version = '$build_dot3'").Trim()

  # Example: $sha256 = '79F2895D3D339688FD9D8992CD38A680631CB36AD472F98A9D3D3C2B3EEB22B0'
  if ($_ -eq $NuInstall) {
    $m = ([regex]"[$]sha256 = '[a-zA-Z0-9]+'").Matches($txt)
    if (($m -eq $null) -Or ($m.Count -ne 1)) {
      Write-Host -ForegroundColor Red "Proper `$sha256 define was not found in:`r`n$_"
      $host.SetShouldExit(101)
      exit
    }
    $txt = $txt.Replace($m[0].Value, "`$sha256 = '$sha'").Trim()
  }

  Set-Content $_ $txt -Encoding Ascii
}


#
# Nuget.org : ConEmu.Core\ConEmu.Core.nuspec
#
if (-Not (NuSpec-SetBuild $ConEmuCoreNuget)) {
  exit 
}
