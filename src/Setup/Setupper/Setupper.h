// NO BOM! Compiled with old gcc!

#pragma once

// String resources (messages)

const wchar_t gsWWW[] = L"Project page: <a href=\"https://conemu.github.io/\">https://conemu.github.io/</a>";

const wchar_t msgConEmuInstaller[] = L"ConEmu %s installer";
const wchar_t msgRunSetupAsAdmin[] = L"Run installer as administrator";

const wchar_t msgUsageExample[] =
	L"Usage:\n"
	L"   ConEmuSetup [/p:x86[,adm] | /p:x64[,adm]] [<msi args>]\n"
	L"   ConEmuSetup [/e[:<extract path>]] [/p:x86 | /p:x64]\n"
	L"Example (run x64 auto update as administrator):\n"
	L"   ConEmuSetup /p:x64,adm /qr";

const wchar_t msgRebootRequired[] = L"You have chosen not to reboot the machine.\nReboot is required to get ConEmu instance working properly!\n%%s\nExitCode=%u";
const wchar_t msgInstallerFailed[] = L"Installer failed\n%s";
const wchar_t msgInstallerFailedEx[] = L"Installer failed\n%%s\nExitCode=%u";
const wchar_t msgExitCodeFailed[] = L"Installer failed (GetExitCodeProcess)\n%%s";
const wchar_t msgFindResourceFailed[] = L"FindResource(%i) failed";
const wchar_t msgLoadResourceFailed[] = L"LoadResource(%i) failed";
const wchar_t msgCreateFileFailed[] = L"CreateFile(%s) failed";
const wchar_t msgWriteFileFailed[] = L"WriteFile(%s) failed";
const wchar_t msgTempFolderFailed[] = L"Can't create temp folder\n%s";
const wchar_t msgExtractedSuccessfully[] = L"Installation files were extracted successfully\n%s";
const wchar_t msgChooseExtractVer[] = L"Choose version to extract";
const wchar_t msgExtractX86X64[] = L"%s %s\nExtract installation files to\n%s";
const wchar_t msgExtractConfirm[] = L"%s\n\nPress `Yes` to extract x64 version\nPress `No` to extract x86 version\n\n%s";
const wchar_t msgChooseInstallVer[] = L"Choose version to install";
const wchar_t msgInstallFolderIs[] = L"%s %s\n%s installation folder is\n%s";
const wchar_t msgPathCurrent[] = L"Current";
const wchar_t msgPathDefault[] = L"Default";
const wchar_t msgInstallConfirm[] = L"%s\n\nPress `Yes` to install x64 version\nPress `No` to install x86 version";
