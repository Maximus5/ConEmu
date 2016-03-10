param([string]$mode="auto",[string]$id="",[string]$str="")

$path = split-path -parent $MyInvocation.MyCommand.Definition

# Dialog resources
$conemu_rc_file = ($path + "\..\src\ConEmu\ConEmu.rc")
# gsDataHints: dialog hints, menu hints, hotkey descriptions
$hints_h_file = ($path + "\..\src\ConEmu\LngDataHints.h")
# gsDataRsrcs: just string resources (lng_XXXX)
$rsrcs_h_file = ($path + "\..\src\ConEmu\LngDataRsrcs.h")
# ID-s for dialogs and hotkeys
$resource_h_file = ($path + "\..\src\ConEmu\resource.h")
# ID-s for menu items
$menuids_h_file = ($path + "\..\src\ConEmu\MenuIds.h")
# ID-s for gsDataRsrcs
$rsrsids_h_file = ($path + "\..\src\ConEmu\LngDataEnum.h")

$target_l10n = ($path + "\..\Release\ConEmu\ConEmu.l10n")

$script:ignore_ctrls = @(
  "tAppDistinctHolder", "tDefTermWikiLink",
  "stConEmuUrl", "tvSetupCategories", "stSetCommands2", "stHomePage", "stDisableConImeFast3", "stDisableConImeFast2",
  "lbActivityLog", "lbConEmuHotKeys", "IDI_ICON1", "IDI_ICON2", "IDI_ICON3", "stConEmuAbout"
)

$last_gen_ids_note = "// last auto-gen identifier"
$last_gen_str_note = "{ /* empty trailing item for patch convenience */ }"




function AppendExistingLanguages()
{
  $json.languages | ? { $_.id -ne "en" } | % {
    $script:l10n += "    ,"
    $script:l10n += "    {`"id`": `"$($_.id)`", `"name`": `"$($_.name)`" }"
  }
}


function InitializeJsonData()
{
  # Does the l10n file exist already?
  if ([System.IO.File]::Exists($target_l10n)) {
    $script:json = Get-Content $target_l10n -Raw | ConvertFrom-JSON
  } else {
    $script:json = $NULL
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
  $script:str_id = @{}
  $script:ctrls = @{}
}

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
    Write-Host -ForegroundColor Red ("Was not found: ``" + $cmp + "``")
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



function ParseDialog($rcln, $dlgid, $name)
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

  #Write-Progress -Activity "Dialog Controls" -Completed -Id 2

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

function ParseResIdsEnum($resh)
{
  for ($l = 0; $l -lt $resh.Length; $l++) {
    $ln = $resh[$l].Trim()
    if ($ln -match "\s*(lng_\w+)\s*\=\s*(\d+)\,*") {
      #Write-Host "m1=$($matches[1]); m2=$($matches[2]);"
      $id = [int]$matches[2]
      if ($script:str_id.ContainsValue($id)) {
        $dup = ""; $script:str_id.Keys | % { if ($script:str_id[$_] -eq $id) { $dup = $_ } }
        Write-Host -ForegroundColor Red "res_id duplicate: $($id): '$($matches[1])' & '$($dup)'"
      } else {
        $script:str_id.Add($matches[1],$id)
      }
    }
  }
  return
}

function ParseLngData($LngData)
{
  $b = FindLine 0 $LngData "static LngPredefined"
  if ($b -le 0) { return }
  $e = $LngData.Count
  if ($e -le 0) { return }

  $hints = @{}

  for ($l = ($b+1); $l -lt $e; $l++) {
    $ln = $LngData[$l].Trim()
    if ($ln.StartsWith("//")) {
      continue
    }
    $m = ([regex]'\{\s*(\w+)\,\s*L\"(.+)\"\s*\}').Match($ln)
    if ($m -ne $null) {
      $name = $m.Groups[1].Value
      $text = $m.Groups[2].Value

      if ($text -ne "") {
        $hints.Add($name,$text)
      }
    }
  }

  return $hints
}

# If string comes from ConvertFrom-JSON - it is ‘de-escaped’
function EscapeJson($str)
{
  if ($str -eq $null) {
    return $null
  }
  return $str.Replace("\", "\\").Replace("`t","\t").Replace("`n","\n").Replace("`r","\r")
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

function AppendExistingTranslations([string]$section,[string]$name,[string]$en_value)
{
  $js = $script:json."$section"
  if ($js -ne $null) {
    $jres = $js."$name"
    if ($jres -ne $null) {
      # If English resource was changed - mark other translations as deprecated
      $depr_en = ""; $old_en = "";
      $jlng = EscapeJson $jres."en"
      if ($jlng -ne $null) {
        $old_en = [System.String]::Join("",$jlng).Replace("`r","\r").Replace("`n","\n").Replace("`t","\t").Replace("`"","\`"")
      }
      if ($old_en -ne $en_value) {
        $depr_en = "_"
        Write-Host "Deprecation: $name;`n  old: '$old_en'`n  new: '$en_value'"
      }

      # Append existing translations"
      $json.languages | ? { $_.id -ne "en" } | % {
        $depr = $depr_en
        $jlng = EscapeJson $jres."$($_.id)"
        # Perhaps resource was already deprecated?
        if ($jlng -eq $null) {
          $jlng = EscapeJson $jres."_$($_.id)"
          if ($jlng -ne $null) {
            $depr = "_"
            Write-Host "Already deprecated: $name / _$_.id"
          }
        }
        # Append translation if found
        if ($jlng -ne $null) {
          # Write-Host -ForegroundColor Yellow "    [$($jlng.GetType())] ``$jlng``"
          $str = [System.String]::Join("",$jlng).Replace("`r","\r").Replace("`n","\n").Replace("`t","\t").Replace("`"","\`"")
          if ($str -ne "") {
            $script:l10n += (AddValue "$depr$($_.id)" $str)
          }
        }
      }
    }
  }
}

function WriteResources([string]$section,$ids,$hints)
{
  Write-Host "Section: $section"
  $script:l10n += "  ,"
  $script:l10n += "  `"$section`": {"
  $script:first = $TRUE

  $ids.Keys | sort | % {
    $id = $ids[$_]
    $name = $_
    if ($hints.ContainsKey($name)) {
      $value = $hints[$name]
      if ($script:first) { $script:first = $FALSE } else { $script:l10n += "    ," }
      $script:l10n += "    `"$name`": {"
      $script:l10n += (AddValue "en" $value)
      if ($script:json -ne $NULL) {
        AppendExistingTranslations $section $name $value
      }
      $script:l10n += "      `"id`": $id }"
    }
  }

  $script:l10n += "  }"
}

function WriteControls([string]$section,$ctrls,$ids)
{
  Write-Host "Section: $section"
  $script:l10n += "  ,"
  $script:l10n += "  `"$section`": {"
  $script:first = $TRUE

  $ctrls.Keys | sort | % {
    $name = $_
    $value = $ctrls[$_]
    $id = $ids[$_]
    if ($script:first) { $script:first = $FALSE } else { $script:l10n += "    ," }
    $script:l10n += "    `"$name`": {"
    $script:l10n += (AddValue "en" $value)
    if ($script:json -ne $NULL) {
      AppendExistingTranslations "controls" $name $value
    }
    $script:l10n += "      `"id`": $id }"
  }

  $script:l10n += "  }"
}

function InitDialogList()
{
  $script:dialogs = @()

  # $script:dialogs += @{ id = "IDD_CMDPROMPT";     name = "Commands history and execution"; } ### is not used yet
  $script:dialogs += @{ id = "IDD_FIND";            name = "Find text"; }
  $script:dialogs += @{ id = "IDD_ATTACHDLG";       name = "Choose window or console application for attach"; }
  $script:dialogs += @{ id = "IDD_SETTINGS";        name = "Settings"; }
  $script:dialogs += @{ id = "IDD_RESTART";         name = "ConEmu"; }
  $script:dialogs += @{ id = "IDD_MORE_CONFONT";    name = "Real console font"; }
  # $script:dialogs += @{ id = "IDD_MORE_DOSBOX";   name = "DosBox settings"; }  ### is not used yet
  $script:dialogs += @{ id = "IDD_FAST_CONFIG";     name = "ConEmu fast configuration"; }
  $script:dialogs += @{ id = "IDD_ABOUT";           name = "About"; }
  # $script:dialogs += @{ id = "IDD_ACTION";        name = "Choose action"; }    ### is not used yet
  $script:dialogs += @{ id = "IDD_RENAMETAB";       name = "Rename tab"; }
  $script:dialogs += @{ id = "IDD_HELP";            name = "Item description"; }
  $script:dialogs += @{ id = "IDD_HOTKEY";          name = "Choose hotkey"; }
  $script:dialogs += @{ id = "IDD_AFFINITY";        name = "Set active console processes affinity and priority"; }

  $script:dialogs += @{ id = "IDD_SPG_MAIN";        name = "Main"; }
  $script:dialogs += @{ id = "IDD_SPG_WNDSIZEPOS";  name = " Size & Pos"; }
  $script:dialogs += @{ id = "IDD_SPG_SHOW";        name = " Appearance"; }
  $script:dialogs += @{ id = "IDD_SPG_BACK";        name = " Background"; }
  $script:dialogs += @{ id = "IDD_SPG_TABS";        name = " Tab bar"; }
  $script:dialogs += @{ id = "IDD_SPG_CONFIRM";     name = " Confirm"; }
  $script:dialogs += @{ id = "IDD_SPG_TASKBAR";     name = " Task bar"; }
  $script:dialogs += @{ id = "IDD_SPG_UPDATE";      name = " Update"; }
  $script:dialogs += @{ id = "IDD_SPG_STARTUP";     name = "Startup"; }
  $script:dialogs += @{ id = "IDD_SPG_CMDTASKS";    name = " Tasks"; }
  $script:dialogs += @{ id = "IDD_SPG_COMSPEC";     name = " ComSpec"; }
  $script:dialogs += @{ id = "IDD_SPG_ENVIRONMENT"; name = " Environment"; }
  $script:dialogs += @{ id = "IDD_SPG_FEATURE";     name = "Features"; }
  $script:dialogs += @{ id = "IDD_SPG_CURSOR";      name = " Text cursor"; }
  $script:dialogs += @{ id = "IDD_SPG_COLORS";      name = " Colors"; }
  $script:dialogs += @{ id = "IDD_SPG_TRANSPARENT"; name = " Transparency"; }
  $script:dialogs += @{ id = "IDD_SPG_STATUSBAR";   name = " Status bar"; }
  $script:dialogs += @{ id = "IDD_SPG_APPDISTINCT"; name = " App distinct"; }
  $script:dialogs += @{ id = "IDD_SPG_INTEGRATION"; name = "Integration"; }
  $script:dialogs += @{ id = "IDD_SPG_DEFTERM";     name = " Default term"; }
  $script:dialogs += @{ id = "IDD_SPG_KEYS";        name = "Keys & Macro"; }
  $script:dialogs += @{ id = "IDD_SPG_CONTROL";     name = " Controls"; }
  $script:dialogs += @{ id = "IDD_SPG_MARKCOPY";    name = " Mark/Copy"; }
  $script:dialogs += @{ id = "IDD_SPG_PASTE";       name = " Paste"; }
  $script:dialogs += @{ id = "IDD_SPG_HIGHLIGHT";   name = " Highlight"; }
  $script:dialogs += @{ id = "IDD_SPG_FEATURE_FAR"; name = "Far Manager"; }
  $script:dialogs += @{ id = "IDD_SPG_FARMACRO";    name = " Far macros"; }
  $script:dialogs += @{ id = "IDD_SPG_VIEWS";       name = " Views"; }
  $script:dialogs += @{ id = "IDD_SPG_INFO";        name = "Info"; }
  $script:dialogs += @{ id = "IDD_SPG_DEBUG";       name = " Debug"; }
  $script:dialogs += @{ id = "IDD_SPG_APPDISTINCT2";name = " <App distinct>"; }
}


function UpdateConEmuL10N()
{
  # $script:json & $script:l10n
  InitializeJsonData

  # Query source files

  Write-Host -NoNewLine ("Reading: " + $conemu_rc_file)
  $rcln  = Get-Content $conemu_rc_file
  Write-Host (" Lines: " + $rcln.Length)

  Write-Host -NoNewLine ("Reading: " + $hints_h_file)
  $rchints = Get-Content $hints_h_file
  Write-Host (" Lines: " + $rchints.Length)

  Write-Host -NoNewLine ("Reading: " + $rsrcs_h_file)
  $strdata = Get-Content $rsrcs_h_file
  Write-Host (" Lines: " + $strdata.Length)

  Write-Host -NoNewLine ("Reading: " + $resource_h_file)
  $resh  = Get-Content $resource_h_file
  Write-Host (" Lines: " + $resh.Length)
  ParseResIds $resh

  Write-Host -NoNewLine ("Reading: " + $menuids_h_file)
  $menuh  = Get-Content $menuids_h_file
  Write-Host (" Lines: " + $menuh.Length)
  ParseResIdsHex $menuh

  Write-Host -NoNewLine ("Reading: " + $rsrsids_h_file)
  $rsrch  = Get-Content $rsrsids_h_file
  Write-Host (" Lines: " + $rsrch.Length)
  ParseResIdsEnum $rsrch


  # Preparse hints and hotkeys
  $script:hints = ParseLngData $rchints
  # Prepare string resources
  $script:rsrcs = ParseLngData $strdata
  #Write-Host -ForegroundColor Red "`n`nHints list"

  #######################################################

  WriteResources "cmnhints" $script:res_id $script:hints

  WriteResources "mnuhints" $script:mnu_id $script:hints

  WriteResources "strings"  $script:str_id $script:rsrcs

  ####### Parse sources and write wiki/md pages #########

  Write-Host "Parsing dialog resources"
  InitDialogList
  $iDlgNo = 0
  $script:dialogs | % {
    Write-Progress -Activity "Dialogs" -PercentComplete ($iDlgNo * 100 / $script:dialogs.Count) -Status "$($_.id): $($_.name.Trim())" -Id 1
    ParseDialog $rcln $_.id $_.name.Trim()
    $iDlgNo++
  }
  Write-Progress -Activity "Dialog Controls" -Completed -Id 2
  Write-Progress -Activity "Dialogs" -PercentComplete 100 -Completed -Id 1

  #######################################################

  WriteControls "controls" $script:ctrls $script:res_id

  #######################################################

  $script:l10n += "}"

  Write-Host "Updating: $target_l10n"
  Set-Content $target_l10n $script:l10n -Encoding UTF8
}

function NewLngResource([string]$id,[string]$str,[bool]$loop=$FALSE)
{
  $script:str_id = @{}

  #Write-Host -NoNewLine ("Reading: " + $rsrsids_h_file)
  $rsrch  = Get-Content $rsrsids_h_file
  #Write-Host (" Lines: " + $rsrch.Length)
  ParseResIdsEnum $rsrch

  #Write-Host -NoNewLine ("Reading: " + $rsrcs_h_file)
  $strdata = Get-Content $rsrcs_h_file
  #Write-Host (" Lines: " + $strdata.Length)

  # Prepare string resources
  $script:rsrcs = ParseLngData $strdata

  if ($script:rsrcs.ContainsKey($id)) {
    Write-Host -ForegroundColor Red "Key '$id' already exists: $($script:rsrcs[$id])"
    if (-Not ($loop)) {
      $host.SetShouldExit(101)
      exit
    }
    return
  }

  #############
  $script:dst_ids = @()
  $script:dst_str = @()

  # enum LngResources
  $iNextId = ($script:str_id.Values | measure-object -Maximum).Maximum
  if (($iNextId -eq $null) -Or ($iNextId -eq 0)) {
    Write-Host -ForegroundColor Red "lng_NextId was not found!"
    if (-Not ($loop)) {
      $host.SetShouldExit(101)
      exit
    }
    return
  }
  $iNextId ++
  for ($i = 0; $i -lt $rsrch.Count; $i++) {
    if ($rsrch[$i].Trim() -eq $last_gen_ids_note) {
      break
    }
    $script:dst_ids += $rsrch[$i]
  }
  $script:dst_ids += ("`t" + $id.PadRight(30) + "= " + [string]$iNextId + ",")
  $script:dst_ids += "`t$last_gen_ids_note"
  $script:dst_ids += "`tlng_NextId"
  $script:dst_ids += "};"

  #$script:dst_ids

  # static LngPredefined gsDataRsrcs[]
  for ($i = 0; $i -lt $strdata.Count; $i++) {
    $script:dst_str += $strdata[$i]
    if ($strdata[$i].StartsWith("static LngPredefined")) {
      break
    }
  }
  #
  $script:rsrcs.Add($id, $str)
  $script:rsrcs.Keys | sort | % {
    $script:dst_str += ("`t{ " + ($_+",").PadRight(30) + "L`"" + $script:rsrcs[$_] + "`" },")
  }
  $script:dst_str += "`t$last_gen_str_note"
  $script:dst_str += "};"

  #$script:dst_str

  Set-Content $rsrsids_h_file $script:dst_ids -Encoding UTF8
  Set-Content $rsrcs_h_file   $script:dst_str -Encoding UTF8

  Add-Type -AssemblyName System.Windows.Forms
  if ($str.Length -le 64) {
    $clip = "CLngRc::getRsrc($id/*`"$str`"*/)"
  } else {
    $clip = "CLngRc::getRsrc($id)"
  }
  [Windows.Forms.Clipboard]::SetText($clip);

  Write-Host -ForegroundColor Yellow "Resource created:"
  Write-Host -ForegroundColor Green "  $($id)=$($iNextId)"
  Write-Host -ForegroundColor Yellow "Ready to paste:"
  Write-Host -ForegroundColor Green "  $clip"
  Write-Host -ForegroundColor Yellow "Don't forget to update ConEmu.l10n with rc2json.cmd!"

  return
}

function NewLngResourceLoop()
{
  while ($TRUE) {
    Write-Host -ForeGroundColor Yellow -NoNewLine "ResourceID: "
    Write-Host -ForeGroundColor Gray -NoNewLine "lng_"
    $id = Read-Host
    if ($id -eq "") { return }
    $id = "lng_$id"
    if (-Not ($id -match "^\w+$")) {
      Write-Host -ForegroundColor Red "  Invalid ResourceID: '$id'"
      continue
    }
    Write-Host -ForeGroundColor Yellow -NoNewLine "    String: "
    $str = Read-Host
    if ($str -eq "") {
      Write-Host -ForegroundColor Red "  Empty string skipped"
      continue
    }

    NewLngResource $id $str $TRUE
  }
}



#################################################
##############  Main entry point  ###############
#################################################
if ($mode -eq "auto") {
  UpdateConEmuL10N
  Write-Host "All done"
} elseif ($mode -eq "add") {
  if (($id -eq "") -Or ($id -eq "-")) {
    NewLngResourceLoop
  } else {
    if ($str -eq "") { $str = $env:ce_add_str }
    NewLngResource $id $str
  }
}
