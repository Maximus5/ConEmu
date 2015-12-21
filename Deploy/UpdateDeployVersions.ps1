param([string]$build="")

# This file will update PortableApps and Chocolatey versions

$Script_File_path = split-path -parent $MyInvocation.MyCommand.Definition
$PortableApps = Join-Path $Script_File_path "..\PortableApps\App\AppInfo\appinfo.ini"
$NuSpec = Join-Path $Script_File_path "..\nuget\chocolatey\ConEmu.nuspec"
$NuInstall = Join-Path $Script_File_path "..\nuget\chocolatey\tools\chocolateyInstall.ps1"
$NuUnInstall = Join-Path $Script_File_path "..\nuget\chocolatey\tools\chocolateyUninstall.ps1"
$wix = Join-Path $Script_File_path "..\src\Setup\Version.wxi"
$setupper = Join-Path $Script_File_path "..\src\Setup\Setupper\VersionI.h"
$WhatsNew = Join-Path $Script_File_path "..\Release\ConEmu\WhatsNew-ConEmu.txt"

if ($build -eq "") {
  $build = $env:CurVerBuild
}

if (($build -eq $null) -Or (-Not ($build -match "\b\d\d\d\d\d\d(\w*)\b"))) {
  Write-Host -ForegroundColor Red "Build was not passed as argument!"
  $host.SetShouldExit(101)
  exit 
}

$ver4n = 0; $ver4 = "";
if ($build.Length -gt 6) {
$ver4 = $build.SubString(6,1)
$ver4n = ([byte][char]$ver4) - ([byte][char]'a') + 1
}

$build_dot3 = "$($build.SubString(0,2)).$($build.SubString(2,2)).$($build.SubString(4,2))"
$build_year3 = "20$($build.SubString(0,2)).$($build.SubString(2,2).TrimStart('0')).$($build.SubString(4,2).TrimStart('0'))$ver4"
$build_dot4 = "$($build.SubString(0,2)).$($build.SubString(2,2).TrimStart('0')).$($build.SubString(4,2).TrimStart('0')).$ver4n"
$build_comma = "$($build.SubString(0,2)),$($build.SubString(2,2).TrimStart('0')),$($build.SubString(4,2).TrimStart('0')),$ver4n"

Write-Host -ForegroundColor Yellow "Updating files with build# $build"

Add-Type -PassThru '
// (C) 2015 ConEmu.Maximus5@gmail.com
using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
public class ProfileAPI {

    [DllImport("kernel32.dll")]
    public static extern bool WriteProfileSection(
        string lpAppName,
        string lpString);

    [DllImport("kernel32.dll")]
    public static extern bool WriteProfileString(
        string lpAppName,
        string lpKeyName,
        string lpString);

    [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool WritePrivateProfileString(
        string lpAppName,
        string lpKeyName,
        string lpString,
        string lpFileName);

    [DllImport("kernel32.dll")]
    public static extern uint GetPrivateProfileSectionNames(
        long lpReturnBuffer,
        uint nSize,
        string lpFileName);

    [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    public static extern uint GetPrivateProfileString(
        string lpAppName,
        string lpKeyName,
        string lpDefault,
        StringBuilder lpReturnedString,
        uint nSize,
        string lpFileName);
}
' | Out-Null


#
# PortableApps.com
#
Write-Host -ForegroundColor Green $PortableApps
[profileapi]::WritePrivateProfileString('Version','PackageVersion',$build_dot4,$PortableApps) | Out-Null
[profileapi]::WritePrivateProfileString('Version','DisplayVersion',$build,$PortableApps) | Out-Null




#
# Chocolatey.org
#
Write-Host -ForegroundColor Green $NuSpec
$xml = Get-Content -Raw $NuSpec
$m = ([regex]'<version>\d+\.\d+\.\d+\..<\/version>').Matches($xml)
if (($m -eq $null) -Or ($m.Count -ne 1)) {
  Write-Host -ForegroundColor Red "Proper <version>...</version> tag was not found in:`r`n$NuSpec"
  $host.SetShouldExit(101)
  exit 
}
$xml = $xml.Replace($m[0].Value, "<version>$build_dot4</version>").Trim()
Set-Content $NuSpec $xml -Encoding Ascii

@($NuInstall, $NuUnInstall) | % {
  Write-Host -ForegroundColor Green $_
  $txt = Get-Content -Raw $_
  $m = ([regex]"[$]version = '\d+\.\d+\.\d+'").Matches($txt)
  if (($m -eq $null) -Or ($m.Count -ne 1)) {
    Write-Host -ForegroundColor Red "Proper `$version define was not found in:`r`n$_"
    $host.SetShouldExit(101)
    exit 
  }
  $txt = $txt.Replace($m[0].Value, "`$version = '$build_dot3'").Trim()
  Set-Content $_ $txt -Encoding Ascii
}



#
# Installer
#
Write-Host -ForegroundColor Green $wix
$xml = "<?xml version=`"1.0`" encoding=`"utf-8`"?>
<Include>

  <?define Version = '`$(var.MajorVersion).$($build.SubString(0,3)).$($build.SubString(3,3))0' ?>
  <?define ConEmuVerS = '$build.`$(var.Platform)' ?>

</Include>"
Set-Content $wix $xml -Encoding Ascii

Write-Host -ForegroundColor Green $setupper
$txt = "#define CONEMUVERN $build_comma
#define CONEMUVERS `"$build`"
#define CONEMUVERL L`"$build`"
#define MSI86 `"../ConEmu.$build.x86.msi`"
#define MSI64 `"../ConEmu.$build.x64.msi`""
Set-Content $setupper $txt -Encoding Ascii





#
# Add Whats-New label
#
Write-Host -ForegroundColor Green $WhatsNew
$txt = Get-Content -Raw $WhatsNew -Encoding Utf8
if ($txt.Contains($build_year3)) {
  Write-Host "Whats-New already has $build_year3 label"
} else {
  $m = ([regex]"\b20\d\d\.\d+\.\d+\b").Match($txt)
  if (($m -eq $null) -Or (-Not $m.Success)) {
    Write-Host -ForegroundColor Red "Whats-New.txt parsing failed!"
    $host.SetShouldExit(101)
    exit 
  }
  $txt = $txt.Insert($m.Index, "$build_year3`r`n`r`n`r`n").Trim()
  Set-Content $WhatsNew $txt -Encoding Utf8
}
