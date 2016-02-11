param([string]$mode="all")

$path = split-path -parent $MyInvocation.MyCommand.Definition

# https://github.com/Maximus5/ConEmu/raw/alpha/src/ConEmu/ConEmu.rc
$conemu_rc_file = ($path + "\..\src\ConEmu\ConEmu.rc")
# https://github.com/Maximus5/ConEmu/raw/alpha/src/ConEmu/ConEmu.rc2
$conemu_rc2_file = ($path + "\..\src\ConEmu\ConEmu.rc2")
# https://github.com/Maximus5/ConEmu/raw/alpha/src/ConEmu/resource.h
$resource_h_file = ($path + "\..\src\ConEmu\resource.h")
# https://github.com/Maximus5/ConEmu/raw/alpha/src/ConEmu/MenuIds.h
$menuids_h_file = ($path + "\..\src\ConEmu\MenuIds.h")

$target_l10n = ($path + "\..\Release\ConEmu\ConEmu.l10n")

$linedelta = 7

# Does the l10n file exist already?
if ([System.IO.File]::Exists($target_l10n)) {
  $script:json = Get-Content $target_l10n -Raw | ConvertFrom-JSON
} else {
  $script:json = $NULL
}

function AppendExistingLanguages()
{
  $lng = $script:json["languages"]
  $json.languages | ? { $_.id -ne "en" } | % {
    $script:l10n += "    ,"
    $script:l10n += "    {`"id`": `"$($_.id)`", `"name`": `"$($_.name)`" }"
  }
}


$script:l10n = @("{")
$script:l10n += "  `"languages`": ["
$script:l10n += "    {`"id`": `"en`", `"name`": `"English`" }"
if ($script:json -ne $null) {
  AppendExistingLanguages
}
$script:l10n += "  ]"

$script:res_id = @{}
$script:mnu_id = @{}
$script:ctrls = @{}

$script:ignore_ctrls = @(
  "tAppDistinctHolder", "tDefTermWikiLink",
  "stConEmuUrl", "tvSetupCategories", "stSetCommands2", "stHomePage", "stDisableConImeFast3", "stDisableConImeFast2",
  "lbActivityLog", "lbConEmuHotKeys", "IDI_ICON1", "IDI_ICON2", "IDI_ICON3"
)

function FindLine($l, $rcln, $cmp)
{
  #Write-Host "'"+$l+"'"
  #Write-Host "'"+$cmp+"'"
  #Write-Host "'"+$rcln+"'"
  $n = $cmp.length
  #Write-Host "'"+$n+"'"
  while ($l -lt $rcln.length) {
    if ($rcln[$l].length -ge $n) {
      $left = $rcln[$l].SubString(0,$n)
      if (($left -eq $cmp)) {
        break
      }
    }
    $l++
  }
  if ($l -ge $rcln.length) {
    Write-Host -ForegroundColor Red ("Was not found: С" + $cmp + "Т")
    return -1
  }
  return $l
}

function ParseLine($ln)
{
  $type = $ln.SubString(0,15).Trim()
  $title = ""
  $id = ""
  $x = -1; $y = -1; $width = -1; $height = -1

  # RTEXT|LTEXT|CTEXT   "Size:",IDC_STATIC,145,15,20,8
  # GROUPBOX     "Quake style",IDC_STATIC,4,201,319,29
  # CONTROL      "Close,after,paste ""C:\\"" /dir",cbHistoryDontClose,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,7,190,153,8
  # CONTROL      "Show && store current window size and position",cbUseCurrentSizePos, "Button",BS_AUTOCHECKBOX | WS_TABSTOP,11,13,258,8
  # CONTROL      "",IDC_ATTACHLIST,"SysListView32",LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | WS_BORDER | WS_TABSTOP,4,4,310,159
  # PUSHBUTTON   "Add default tasks...",cbAddDefaults,9,179,93,14
  # EDITTEXT     tCmdGroupName,109,12,81,12,ES_AUTOHSCROLL
  # COMBOBOX     lbExtendFontBoldIdx,65,9,26,30,CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL | WS_TABSTOP
  # LISTBOX      lbHistoryList,7,16,311,116,LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_TABSTOP

  $left = $ln.SubString(16).Trim()
  if ($left.SubString(0,3) -eq "`"`",") {
    $left = $left.SubString(2).Trim()
  } elseif ($left.SubString(0,1) -eq "`"") {
    # "Quake,style",IDC_STATIC...
    # "Show && store",cbUseCurrentSizePos...
    # "Close,after,paste ""C:\\"" /dir",cbHistoryDontClose...
    $i = $left.IndexOf("`"",1)
    while ($i -gt 0) {
      $i2 = $left.IndexOf("`"",$i+1)
      if ($i2 -eq ($i+1)) {
        $i = $left.IndexOf("`"",$i2+1)
      } else {
        break
      }
    }
    if ($i -le 1) {
      Write-Host -ForegroundColor Red ("Invalid string, bad token: "+$ln)
      return
    }
    $title = $left.SubString(1,$i-1).Replace("`"`"","\`"") #.TrimEnd(':').Replace("&&",[String][char]1).Replace("&","").Replace([String][char]1,"&").Replace("\\","\").Replace("`"`"","`"")
    $left = $left.SubString($i).Trim()
    #Write-Host $left
  }

  $arr = $left.Split(",")

  # RTEXT|LTEXT|CTEXT   "Size:",IDC_STATIC,145,15,20,8
  # GROUPBOX     "Quake style",IDC_STATIC,4,201,319,29
  # PUSHBUTTON   "Add default tasks...",cbAddDefaults,9,179,93,14
  if (($type -eq "RTEXT") -Or ($type -eq "LTEXT") -Or ($type -eq "CTEXT") -Or ($type -eq "GROUPBOX") -Or
      ($type -eq "PUSHBUTTON") -Or ($type -eq "DEFPUSHBUTTON")) {
    #Write-Host $type + $text + $arr
    $id = $arr[1]; $i = 2
    $x = [int]($arr[$i]); $y = [int]($arr[$i+1]); $width = [int]($arr[$i+2]); $height = [int]($arr[$i+3])
  }
  # CONTROL      "Close,after,paste ""C:\\"" /dir",cbHistoryDontClose,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,7,190,153,8
  # CONTROL      "Show && store current window size and position",cbUseCurrentSizePos, "Button",BS_AUTOCHECKBOX | WS_TABSTOP,11,13,258,8
  # CONTROL      "",IDC_ATTACHLIST,"SysListView32",LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | WS_BORDER | WS_TABSTOP,4,4,310,159
  elseif ($type -eq "CONTROL") {
    #Write-Host $type + $text + $arr
    $id = $arr[1]; $i = 4
    $x = [int]($arr[$i]); $y = [int]($arr[$i+1]); $width = [int]($arr[$i+2]); $height = [int]($arr[$i+3])
    #if ($id -eq "cbCmdTaskbarCommands") { Write-Host ("CONTROL: "+$id+" - '"+$arr[2]+"' - "+$arr[3]) }
    $class = ($arr[2]).Trim().Trim('"')
    if ($class -eq "SysListView32") {
      $type = "LVS_REPORT"
    }
    elseif ($class -eq "msctls_trackbar32") {
      $type = "TRACKBAR"
    }
    elseif ($class -eq "Button") {
      #if ($id -eq "cbCmdTaskbarCommands") { Write-Host ("Button: "+$id+" - "+$arr[3]) }
      if ((($arr[3]).Contains("CHECKBOX")) -Or (($arr[3]).Contains("AUTO3STATE"))) {
        $type = "CHECKBOX"
      } elseif (($arr[3]).IndexOf("RADIO") -gt 0) {
        $type = "RADIOBUTTON"
      }
    }
    #if ($id -eq "cbCmdTaskbarCommands") { Write-Host ("Type->" + $type) }
  }
  # EDITTEXT     tCmdGroupName,109,12,81,12,ES_AUTOHSCROLL
  # COMBOBOX     lbExtendFontBoldIdx,65,9,26,30,CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL | WS_TABSTOP
  # LISTBOX      lbHistoryList,7,16,311,116,LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_TABSTOP
  else {
    if ($type -eq "EDITTEXT") { $type = "EDIT" }
    elseif (($type -eq "COMBOBOX") -And ($arr.Length -gt 5)) {
      if ($arr[5].Contains("CBS_DROPDOWNLIST")) { $type = "DROPDOWNBOX" }
    }
    # Write-Host $type + $text + $arr
    $id = $arr[0]; $i = 1
    $x = [int]($arr[$i]); $y = [int]($arr[$i+1]); $width = [int]($arr[$i+2]); $height = [int]($arr[$i+3])
  }

  $y2 = $y
  if ($type -ne "GROUPBOX") {
    $y2 = ($y + [int]($height / 2))
  }

  #if ($id -eq "IDC_STATIC") {
  #  $id = ""
  #}

  return @{Type=$type;Title=$title;Id=$id;X=$x;Y=$y;Width=$width;Height=$height;Y2=$y2}
}

function ReadFileContent($file)
{
  if ([System.IO.File]::Exists($file)) {
    $src = Get-Content $file -Encoding UTF8
    $text = [System.String]::Join("`r`n",$src)
  } else {
    $text = ""
  }
  return $text
}

function WriteFileContent($file,$text)
{
  #Set-Content $file $text -Encoding UTF8
  $Utf8NoBom = new-object -TypeName System.Text.UTF8Encoding($FALSE)
  [System.IO.File]::WriteAllText($file, $text, $Utf8NoBom)
  #[System.IO.File]::WriteAllLines($file, $text)
}



function ParseDialog($rcln, $hints, $dlgid, $name)
{
  $l = FindLine 0 $rcln ($dlgid + " ")
  if ($l -le 0) { return }

  $b = FindLine $l $rcln "BEGIN"
  if ($b -le 0) { return }
  $e = FindLine $b $rcln "END"
  if ($e -le 0) { return }

  $supported = "|GROUPBOX|CONTROL|RTEXT|LTEXT|CTEXT|PUSHBUTTON|DEFPUSHBUTTON|EDITTEXT|COMBOBOX|LISTBOX|"
  $ignored = "|ICON|";

  $ln = ""
  $items_arg = @()

  #Write-Host -ForegroundColor Green ("Dialog: " + $name)
  $dlg_printed = $FALSE

  for ($l = ($b+1); $l -lt $e; $l++) {
    if ($rcln[$l].Length -le 22) {
      continue
    }
    if ($rcln[$l].SubString(0,8) -eq "        ") {
      if ($ln -ne "") {
        $ln += " " + $rcln[$l].Trim()
      }
    } else {
      if ($ln -ne "") {
        $h = ParseLine $ln
        $items_arg += $h
      }
      $ln = ""
      $ctrl = $rcln[$l].SubString(0,20).Trim()
      if ($supported.Contains("|"+$ctrl+"|")) {
        $ln = $rcln[$l].Trim()
      } elseif ($ignored.Contains("|"+$ctrl+"|")) {
        # skip this control
      } else {
        Write-Host -ForegroundColor Red ("Unsupported control type: "+$ctrl)
      }
    }
  }
  if ($ln -ne "") {
    $h = ParseLine $ln
    $items_arg += $h
  }

  #Write-Host ("  Controls: " + $items_arg.Count)
  Write-Progress -Activity "Dialog Controls" -Status "Controls: $($items_arg.Count)" -ParentId 1 -Id 2

  $items_arg | % {
    if (($_.Id -ne "IDC_STATIC") -And      # has valid ID
        -Not ($script:ignore_ctrls.Contains($_.Id)) -And
        ($_.Title.Trim() -ne "") -And      # non empty text
        ($_.Title -match "[a-zA-Z]{2,}"))  # with two letters at least
    {
      if ($script:ctrls.ContainsKey($_.Id)) {
        if ($script:ctrls[$_.Id] -ne $_.Title) {
          if (-Not $dlg_printed) {
            $dlg_printed = $TRUE
            Write-Host -ForegroundColor Red "  Dialog $($dlgid): $name"
          }
          Write-Host "    Duplicate control ID=$($_.Id)"
          Write-Host "      New: '$($_.Title)'"
          Write-Host "      Old: '$($script:ctrls[$_.Id])'"
        }
      } else {
        $script:ctrls.Add($_.Id, $_.Title)
      }
    }
  }

  return
}

function ParseResIds($resh)
{
  $script:res_id.Add("IDOK", 1)
  $script:res_id.Add("IDCANCEL", 2)
  $script:res_id.Add("IDABORT", 3)
  $script:res_id.Add("IDRETRY", 4)
  $script:res_id.Add("IDIGNORE", 5)
  $script:res_id.Add("IDYES", 6)
  $script:res_id.Add("IDNO", 7)
  $script:res_id.Add("IDCLOSE", 8)
  $script:res_id.Add("IDHELP", 9)
  $script:res_id.Add("IDTRYAGAIN", 10)
  $script:res_id.Add("IDCONTINUE", 11)

  for ($l = 0; $l -lt $resh.Length; $l++) {
    $ln = $resh[$l].Trim()
    if ($ln -match "#define\s+(\w+)\s+(\-?\d+)") {
      $id = [int]$matches[2]
      if ($script:ignore_ctrls.Contains($matches[1])) {
        ## Just skip this resource ID
      } elseif ($script:res_id.ContainsValue($id)) {
        $dup = ""; $script:res_id.Keys | % { if ($script:res_id[$_] -eq $id) { $dup = $_ } }
        Write-Host -ForegroundColor Red "res_id duplicate: $($id): '$($matches[1])' & '$($dup)'"
      } else {
        $script:res_id.Add($matches[1],$id)
      }
    }
  }
  return
}

function ParseResIdsHex($resh)
{
  for ($l = 0; $l -lt $resh.Length; $l++) {
    $ln = $resh[$l].Trim()
    #Write-Host $ln
    if ($ln -match "#define\s+(\w+)\s+(0x\w+)") {
      #Write-Host "$($matches[1]): $($matches[2])"
      $id = [int]$matches[2]
      if ($script:res_id.ContainsValue($id)) {
        $dup = ""; $script:res_id.Keys | % { if ($script:res_id[$_] -eq $id) { $dup = $_ } }
        Write-Host -ForegroundColor Red "res_id duplicate: $($id): '$($matches[1])' & '$($dup)'"
      }
      if ($script:mnu_id.ContainsValue($id)) {
        $dup = ""; $script:mnu_id.Keys | % { if ($script:mnu_id[$_] -eq $id) { $dup = $_ } }
        Write-Host -ForegroundColor Red "mnu_id duplicate: $($id): '$($matches[1])' & '$($dup)'"
      } else {
        $script:mnu_id.Add($matches[1],$id)
      }
    }
  }
  return
}

function ParseHints($rc2ln)
{
  $l = FindLine 0 $rc2ln "STRINGTABLE"
  if ($l -le 0) { return }
  $b = FindLine $l $rc2ln "BEGIN"
  if ($b -le 0) { return }
  $e = FindLine $b $rc2ln "END"
  if ($e -le 0) { return }

  $hints = @{}

  for ($l = ($b+1); $l -lt $e; $l++) {
    $ln = $rc2ln[$l].Trim()
    if ($ln.StartsWith("//")) {
      continue
    }
    $s = $ln.IndexOf(" ")
    if ($s -gt 1) {
      $name = $ln.SubString(0, $s)
      $text = $ln.SubString($s).Trim()
      $text = $text.SubString(1,$text.Length-2)

      #$s = $text.IndexOf("\")
      #while ($s -ge 0) {
      #  $v = $text.SubString($s,2)
      #  if ($v -eq "\r") { $r = "`r" }
      #  elseif ($v -eq "\n") { $r = "`n" }
      #  elseif ($v -eq "\t") { $r = "`t" }
      #  elseif ($v -eq "\\") { $r = "\" }
      #  else { $r = $e }

      #  $text = $text.Remove($s,2).Insert($s,$r)
      #  $s = $text.IndexOf("\",$s+$r.length)
      #}

      if ($text -ne "") {
        #$vi = @([int]$script:ids[$name])
        #$vi += [string]$name;
        #$vv = @{ id = $vi; "en" = $text; }
        #$vv = @{ iden = [int]$script:ids[$name]; name = $name; "en" = $text; }
        #$script:l10n["rsrc"] += $vv;
        $hints.Add($name,$text.Replace("`"`"","\`""))
      }
    }
  }

  return $hints
}

function AddValue($lng,$value)
{
  $lines = @()
  $m = ([regex]'(?<!\\)\\n(?=.+)').Matches($value)
  if (($m -ne $null) -And ($m.Count -gt 0)) {
    $lines += "      `"$lng`": [ `"$($value.Substring(0, $m[0].Index+$m[0].Length))`""
    for ($i = 0; $i -lt $m.Count; $i++) {
      $l1 = $m[$i].Index + $m[$i].Length
      if (($i+1) -lt $m.Count) {
        $l2 = $m[$i+1].Index + $m[$i+1].Length
        $fin = ""
      } else {
        $l2 = $value.Length
        $fin = " ],"
      }
      $lines += "            , `"$($value.Substring($l1, $l2 - $l1))`"$fin"
    }
  } else {
    $lines += "      `"$lng`": `"$value`","
  }
  return $lines
}

function WriteResources($ids)
{
  $ids.Keys | sort | % {
    $id = $ids[$_]
    $name = $_
    if ($hints.ContainsKey($name)) {
      $value = $hints[$name]
      if ($script:first) { $script:first = $FALSE } else { $script:l10n += "    ," }
      $script:l10n += "    `"$name`": {"
      $script:l10n += (AddValue "en" $value)
      $script:l10n += "      `"id`": $id }"
    }
  }
}

function WriteControls($ctrls,$ids)
{
  $ctrls.Keys | sort | % {
    $name = $_
    $value = $ctrls[$_]
    $id = $ids[$_]
    if ($script:first) { $script:first = $FALSE } else { $script:l10n += "    ," }
    $script:l10n += "    `"$name`": {"
    $script:l10n += (AddValue "en" $value)
    $script:l10n += "      `"id`": $id }"
  }
}

$dialogs = @()

# $dialogs += @{ id = "IDD_CMDPROMPT";     name = "Commands history and execution"; } ### is not used yet
$dialogs += @{ id = "IDD_FIND";            name = "Find text"; }
$dialogs += @{ id = "IDD_ATTACHDLG";       name = "Choose window or console application for attach"; }
$dialogs += @{ id = "IDD_SETTINGS";        name = "Settings"; }
$dialogs += @{ id = "IDD_RESTART";         name = "ConEmu"; }
$dialogs += @{ id = "IDD_MORE_CONFONT";    name = "Real console font"; }
# $dialogs += @{ id = "IDD_MORE_DOSBOX";   name = "DosBox settings"; }  ### is not used yet
$dialogs += @{ id = "IDD_FAST_CONFIG";     name = "ConEmu fast configuration"; }
$dialogs += @{ id = "IDD_ABOUT";           name = "About"; }
# $dialogs += @{ id = "IDD_ACTION";        name = "Choose action"; }    ### is not used yet
$dialogs += @{ id = "IDD_RENAMETAB";       name = "Rename tab"; }
$dialogs += @{ id = "IDD_HELP";            name = "Item description"; }
$dialogs += @{ id = "IDD_HOTKEY";          name = "Choose hotkey"; }
$dialogs += @{ id = "IDD_AFFINITY";        name = "Set active console processes affinity and priority"; }

$dialogs += @{ id = "IDD_SPG_MAIN";        name = "Main"; }
$dialogs += @{ id = "IDD_SPG_WNDSIZEPOS";  name = " Size & Pos"; }
$dialogs += @{ id = "IDD_SPG_SHOW";        name = " Appearance"; }
$dialogs += @{ id = "IDD_SPG_BACK";        name = " Background"; }
$dialogs += @{ id = "IDD_SPG_TABS";        name = " Tab bar"; }
$dialogs += @{ id = "IDD_SPG_CONFIRM";     name = " Confirm"; }
$dialogs += @{ id = "IDD_SPG_TASKBAR";     name = " Task bar"; }
$dialogs += @{ id = "IDD_SPG_UPDATE";      name = " Update"; }
$dialogs += @{ id = "IDD_SPG_STARTUP";     name = "Startup"; }
$dialogs += @{ id = "IDD_SPG_CMDTASKS";    name = " Tasks"; }
$dialogs += @{ id = "IDD_SPG_COMSPEC";     name = " ComSpec"; }
$dialogs += @{ id = "IDD_SPG_ENVIRONMENT"; name = " Environment"; }
$dialogs += @{ id = "IDD_SPG_FEATURE";     name = "Features"; }
$dialogs += @{ id = "IDD_SPG_CURSOR";      name = " Text cursor"; }
$dialogs += @{ id = "IDD_SPG_COLORS";      name = " Colors"; }
$dialogs += @{ id = "IDD_SPG_TRANSPARENT"; name = " Transparency"; }
$dialogs += @{ id = "IDD_SPG_STATUSBAR";   name = " Status bar"; }
$dialogs += @{ id = "IDD_SPG_APPDISTINCT"; name = " App distinct"; }
$dialogs += @{ id = "IDD_SPG_INTEGRATION"; name = "Integration"; }
$dialogs += @{ id = "IDD_SPG_DEFTERM";     name = " Default term"; }
$dialogs += @{ id = "IDD_SPG_KEYS";        name = "Keys & Macro"; }
$dialogs += @{ id = "IDD_SPG_CONTROL";     name = " Controls"; }
$dialogs += @{ id = "IDD_SPG_MARKCOPY";    name = " Mark/Copy"; }
$dialogs += @{ id = "IDD_SPG_PASTE";       name = " Paste"; }
$dialogs += @{ id = "IDD_SPG_HIGHLIGHT";   name = " Highlight"; }
$dialogs += @{ id = "IDD_SPG_FEATURE_FAR"; name = "Far Manager"; }
$dialogs += @{ id = "IDD_SPG_FARMACRO";    name = " Far macros"; }
$dialogs += @{ id = "IDD_SPG_VIEWS";       name = " Views"; }
$dialogs += @{ id = "IDD_SPG_INFO";        name = "Info"; }
$dialogs += @{ id = "IDD_SPG_DEBUG";       name = " Debug"; }
$dialogs += @{ id = "IDD_SPG_APPDISTINCT2";name = " <App distinct>"; }


# Query source files
Write-Host -NoNewLine ("Reading: " + $conemu_rc_file)
$rcln  = Get-Content $conemu_rc_file
Write-Host (" Lines: " + $rcln.Length)
Write-Host -NoNewLine ("Reading: " + $conemu_rc2_file)
$rc2ln = Get-Content $conemu_rc2_file
Write-Host (" Lines: " + $rc2ln.Length)

Write-Host -NoNewLine ("Reading: " + $resource_h_file)
$resh  = Get-Content $resource_h_file
Write-Host (" Lines: " + $resh.Length)
ParseResIds $resh

Write-Host -NoNewLine ("Reading: " + $menuids_h_file)
$menuh  = Get-Content $menuids_h_file
Write-Host (" Lines: " + $menuh.Length)
ParseResIdsHex $menuh

# Preparse hints and hotkeys
$script:hints   = ParseHints   $rc2ln
#Write-Host -ForegroundColor Red "`n`nHints list"

$script:l10n += "  ,"
$script:l10n += "  `"cmnhints`": {"
$script:first = $TRUE
WriteResources $script:res_id
$script:l10n += "  }"

$script:l10n += "  ,"
$script:l10n += "  `"mnuhints`": {"
$script:first = $TRUE
WriteResources $script:mnu_id
$script:l10n += "  }"

#$script:l10n += "    { }"

# Parse sources and write wiki/md pages
$iDlgNo = 0
$dialogs | % {
  Write-Progress -Activity "Dialogs" -PercentComplete ($i * 100 / $dialogs.Count) -Status "$($_.id): $($_.name.Trim())" -Id 1
  ParseDialog $rcln $script:hints $_.id $_.name.Trim()
  $iDlgNo++
}
Write-Progress -Activity "Dialogs" -PercentComplete 100 -Completed -Id 1

$script:l10n += "  ,"
$script:l10n += "  `"controls`": {"
$script:first = $TRUE
WriteControls $script:ctrls $script:res_id
$script:l10n += "  }"

$script:l10n += "}"
Set-Content $target_l10n $script:l10n -Encoding UTF8
