param([string]$mode="auto",[string]$id="",[string]$str="",[switch]$force)

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
# Hotkeys
$conemu_hotkeys_file = Join-Path $path "..\src\ConEmu\HotkeyList.cpp"
# Status bar columns
$conemu_status_file = Join-Path $path "..\src\ConEmu\Status.cpp"

$target_l10n = ($path + "\..\Release\ConEmu\ConEmu.l10n")
$target_yaml_path = ($path + "\..\src\l10n\")

$conemu_page_automsg = "*This page was generated automatically from ConEmu sources*"
$conemu_page_hotkeymark = "{% comment %} LIST OF HOTKEYS {% endcomment %}"

$dest_md = ($path + "\..\..\ConEmu-GitHub-io\ConEmu.github.io\en\")

$conemu_hotkeys_md = Join-Path $dest_md "KeyboardShortcuts.md"

$md_img_path = "/img/"

$linedelta = 7

$script:ignore_ctrls = @(
  "tAppDistinctHolder", "tDefTermWikiLink", "stPalettePreviewFast",
  "stConEmuUrl", "tvSetupCategories", "stSetCommands2", "stHomePage", "stDisableConImeFast3", "stDisableConImeFast2",
  "lbActivityLog", "lbConEmuHotKeys", "IDI_ICON1", "IDI_ICON2", "IDI_ICON3", "stConEmuAbout", "IDD_RESTART"
)

$last_gen_ids_note = "// last auto-gen identifier"
$last_gen_str_note = "{ /* empty trailing item for patch convenience */ }"

$script:dlg_not_found = $FALSE



function AppendExistingLanguages()
{
  $script:json.languages | ? { $_.id -ne "en" } | % {
    $script:l10n += "    ,"
    $script:l10n += "    {`"id`": `"$($_.id)`", `"name`": `"$($_.name)`" }"
    $script:yaml.Add($_.id, @{})
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

  $script:yaml = @{}
  $script:yaml.Add("en", @{})

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
    Write-Host -ForegroundColor Red ("Was not found: ``" + $cmp.Trim() + "``")
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

function StringIsUrl($text)
{
  if (($text.StartsWith("http://") -Or $text.StartsWith("https://")) -And
      ($text.IndexOfAny(" `r`n`t") -eq -1))
  {
    return $TRUE
  }
  return $FALSE
}



function ParseDialogData($rcln, $dlgid, $name)
{
  $l = FindLine 0 $rcln ($dlgid + " ")
  if ($l -le 0) {
    $script:dlg_not_found = $TRUE
    return $null
  }

  $b = FindLine $l $rcln "BEGIN"
  if ($b -le 0) { return $null }
  $e = FindLine $b $rcln "END"
  if ($e -le 0) { return $null }

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
    if (-Not ($script:ignore_ctrls.Contains($h.Id))) {
      $items_arg += $h
    }
  }

  return $items_arg
}

function ParseDialog($rcln, $dlgid, $name)
{
  $items_arg = ParseDialogData $rcln $dlgid $name

  #Write-Host ("  Controls: " + $items_arg.Count)
  Write-Progress -Activity "Dialog Controls" -Status "Controls: $($items_arg.Count)" -ParentId 1 -Id 2

  $items_arg | % {
    if (($_.Id -ne "IDC_STATIC") -And      # has valid ID
        -Not ($script:ignore_ctrls.Contains($_.Id)) -And
        ($_.Title.Trim() -ne "") -And      # non empty text
        -Not (StringIsUrl $_.Title) -And   # not a just a URL
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

# If string comes from ConvertFrom-JSON - it is â€˜de-escapedâ€™
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
        Write-Host -ForegroundColor Yellow "Deprecation: $name;`n  old: '$old_en'`n  new: '$en_value'"
      }

      # Append existing translations"
      $script:json.languages | ? { $_.id -ne "en" } | % {
        $depr = $depr_en
        $jlng = EscapeJson $jres."$($_.id)"
        # Perhaps resource was already deprecated?
        if ($jlng -eq $null) {
          $jlng = EscapeJson $jres."_$($_.id)"
          if ($jlng -ne $null) {
            $depr = "_"
            Write-Host -ForegroundColor Yellow "Already deprecated: $name / _$($_.id)"
          }
        }
        # Append translation if found
        if ($jlng -ne $null) {
          # Write-Host -ForegroundColor Yellow "    [$($jlng.GetType())] ``$jlng``"
          $str = [System.String]::Join("",$jlng).Replace("`r","\r").Replace("`n","\n").Replace("`t","\t").Replace("`"","\`"")
          if ($str -ne "") {
            $script:l10n += (AddValue "$depr$($_.id)" $str)
            if (-not $script:yaml.ContainsKey($_.id)) {
              Write-Host -ForegroundColor Red "Language not found: $($_.id)"
              $script:yaml.Add($_.id, @{})
            }
            if (-not $script:yaml[$_.id].ContainsKey($section)) {
              Write-Host -ForegroundColor Red "Section not found: $($_.id):$section"
              $script:yaml[$_.id].Add($section, @{})
            }
            $script:yaml[$_.id][$section].Add($name, $str)
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
  $script:json.languages | % {
    $script:yaml[$_.id].Add($section, @{})
  }

  $ids.Keys | sort | % {
    $id = $ids[$_]
    $name = $_
    if ($hints.ContainsKey($name)) {
      $value = $hints[$name]
      if ($script:first) { $script:first = $FALSE } else { $script:l10n += "    ," }
      $script:l10n += "    `"$name`": {"
      $script:l10n += (AddValue "en" $value)
      $script:yaml["en"][$section].Add($name, $value)
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
  $script:dialogs += @{ id = "IDD_FIND";            name = "Find text"; file = $null; }
  $script:dialogs += @{ id = "IDD_ATTACHDLG";       name = "Choose window or console application for attach"; file = $null; }
  $script:dialogs += @{ id = "IDD_SETTINGS";        name = "Settings"; file = $null; }
  $script:dialogs += @{ id = "IDD_RESTART";         name = "ConEmu"; file = $null; }
  $script:dialogs += @{ id = "IDD_MORE_CONFONT";    name = "Real console font"; file = $null; }
  # $script:dialogs += @{ id = "IDD_MORE_DOSBOX";   name = "DosBox settings"; }  ### is not used yet
  $script:dialogs += @{ id = "IDD_FAST_CONFIG";     name = "ConEmu fast configuration"; file = $null; }
  $script:dialogs += @{ id = "IDD_ABOUT";           name = "About"; file = $null; }
  # $script:dialogs += @{ id = "IDD_ACTION";        name = "Choose action"; }    ### is not used yet
  $script:dialogs += @{ id = "IDD_RENAMETAB";       name = "Rename tab"; file = $null; }
  $script:dialogs += @{ id = "IDD_HELP";            name = "Item description"; file = $null; }
  $script:dialogs += @{ id = "IDD_HOTKEY";          name = "Choose hotkey"; file = $null; }
  $script:dialogs += @{ id = "IDD_AFFINITY";        name = "Set active console processes affinity and priority"; file = $null; }

  $script:dialogs += @{ id = "IDD_SPG_GENERAL";     name = "General"; file = "Settings-Fast"; }
  $script:dialogs += @{ id = "IDD_SPG_FONTS";       name = " Fonts"; file = "Settings-Main"; }
  $script:dialogs += @{ id = "IDD_SPG_SIZEPOS";     name = " Size & Pos"; file = "Settings-SizePos"; }
  $script:dialogs += @{ id = "IDD_SPG_APPEAR";      name = " Appearance"; file = "Settings-Appearance"; }
  $script:dialogs += @{ id = "IDD_SPG_QUAKE";       name = " Quake style"; file = "Settings-Quake"; } # NEW
  $script:dialogs += @{ id = "IDD_SPG_BACKGR";      name = " Background"; file = "Settings-Background"; }
  $script:dialogs += @{ id = "IDD_SPG_TABS";        name = " Tab bar"; file = "Settings-TabBar"; }
  $script:dialogs += @{ id = "IDD_SPG_CONFIRM";     name = " Confirm"; file = "Settings-Confirm"; }
  $script:dialogs += @{ id = "IDD_SPG_TASKBAR";     name = " Task bar"; file = "Settings-TaskBar"; }
  $script:dialogs += @{ id = "IDD_SPG_UPDATE";      name = " Update"; file = "Settings-Update"; }
  $script:dialogs += @{ id = "IDD_SPG_STARTUP";     name = "Startup"; file = "Settings-Startup"; }
  $script:dialogs += @{ id = "IDD_SPG_TASKS";       name = " Tasks"; file = "Settings-Tasks"; }
  $script:dialogs += @{ id = "IDD_SPG_ENVIRONMENT"; name = " Environment"; file = "Settings-Environment"; }
  $script:dialogs += @{ id = "IDD_SPG_FEATURES";    name = "Features"; file = "Settings-Features"; }
  $script:dialogs += @{ id = "IDD_SPG_CURSOR";      name = " Text cursor"; file = "Settings-TextCursor"; }
  $script:dialogs += @{ id = "IDD_SPG_COLORS";      name = " Colors"; file = "Settings-Colors,Settings-Colors2"; }
  $script:dialogs += @{ id = "IDD_SPG_TRANSPARENT"; name = " Transparency"; file = "Settings-Transparency"; } # manual?
  $script:dialogs += @{ id = "IDD_SPG_STATUSBAR";   name = " Status bar"; file = "Settings-StatusBar"; }
  $script:dialogs += @{ id = "IDD_SPG_APPDISTINCT"; name = " App distinct"; file = "Settings-AppDistinct,Settings-AppDistinct2"; }
  $script:dialogs += @{ id = "IDD_SPG_INTEGRATION"; name = "Integration"; file ="Settings-Integration"; } # manual?
  $script:dialogs += @{ id = "IDD_SPG_DEFTERM";     name = " Default term"; file = "Settings-DefTerm"; }
  $script:dialogs += @{ id = "IDD_SPG_COMSPEC";     name = " ComSpec"; file = "Settings-Comspec"; }
  $script:dialogs += @{ id = "IDD_SPG_CHILDGUI";    name = " Children GUI"; file = "Settings-ChildGui"; } # NEW
  $script:dialogs += @{ id = "IDD_SPG_ANSI";        name = " ANSI execution"; file = "Settings-ANSI"; } # NEW
  $script:dialogs += @{ id = "IDD_SPG_KEYS";        name = "Keys & Macro"; file = "Settings-Hotkeys,Settings-Hotkeys2"; }
  $script:dialogs += @{ id = "IDD_SPG_KEYBOARD";    name = " Keyboard"; file = "Settings-Keyboard"; } # NEW
  $script:dialogs += @{ id = "IDD_SPG_MOUSE";       name = " Mouse"; file = "Settings-Mouse"; } # NEW
  $script:dialogs += @{ id = "IDD_SPG_MARKCOPY";    name = " Mark/Copy"; file = "Settings-MarkCopy"; }
  $script:dialogs += @{ id = "IDD_SPG_PASTE";       name = " Paste"; file = "Settings-Paste"; }
  $script:dialogs += @{ id = "IDD_SPG_HIGHLIGHT";   name = " Highlight"; file = "Settings-Highlight"; }
  $script:dialogs += @{ id = "IDD_SPG_FEATURE_FAR"; name = "Far Manager"; file = "Settings-Far"; }
  $script:dialogs += @{ id = "IDD_SPG_FARMACRO";    name = " Far macros"; file = "Settings-Far-Macros"; }
  $script:dialogs += @{ id = "IDD_SPG_VIEWS";       name = " Views"; file = "Settings-Far-View"; }
  $script:dialogs += @{ id = "IDD_SPG_INFO";        name = "Info"; file = "Settings-Info"; }
  $script:dialogs += @{ id = "IDD_SPG_DEBUG";       name = " Debug"; file = "Settings-Debug"; }
  $script:dialogs += @{ id = "IDD_SPG_APPDISTINCT2";name = " <App distinct>"; file = $null; } # ONLY rc2json
}


function WriteL10nYaml()
{
  $script:yaml.Keys | % {
    $yaml_file = Join-Path $target_yaml_path "ConEmu_$($_).yaml"
    Write-Host "Updating: $yaml_file"
    $lang = $script:yaml[$_]
    $data = @()
    $lang.Keys | % {
      $section = $lang[$_]
      $data += $_+":"  # cmnhints:
      $section.Keys | % {
        $data += "  "+$_+": `"" + $section[$_] + "`""
      }
    }
    Set-Content $yaml_file $data -Encoding UTF8
  }
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
  $script:dlg_not_found = $FALSE
  $script:dialogs | % {
    Write-Progress -Activity "Dialogs" -PercentComplete ($iDlgNo * 100 / $script:dialogs.Count) -Status "$($_.id): $($_.name.Trim())" -Id 1
    ParseDialog $rcln $_.id $_.name.Trim()
    $iDlgNo++
  }
  Write-Progress -Activity "Dialog Controls" -Completed -Id 2
  Write-Progress -Activity "Dialogs" -PercentComplete 100 -Completed -Id 1
  if ($script:dlg_not_found) {
    Write-Host -ForegroundColor Red ("Some dialogs were not found in ``" + (Split-Path $conemu_rc_file -Leaf) + "```nExiting!")
    $host.SetShouldExit(101)
    exit
  }

  #######################################################

  WriteControls "controls" $script:ctrls $script:res_id

  #######################################################

  $script:l10n += "}"

  Write-Host "Updating: $target_l10n"
  Set-Content $target_l10n $script:l10n -Encoding UTF8

  # WriteL10nYaml
}

# $loop is $TRUE when called from NewLngResourceLoop
function NewLngResourceHelper([string]$id,[string]$str,[bool]$loop=$FALSE,[string]$file="",[string]$header="")
{
  $script:str_id = @{}

  if ($header -ne "") { $id_pad = 30 } else { $id_pad = 25 }

  if ($header -ne "") {
    #Write-Host -NoNewLine ("Reading: " + $header)
    $rsrch  = Get-Content $header
    #Write-Host (" Lines: " + $rsrch.Length)
    ParseResIdsEnum $rsrch
  }

  #Write-Host -NoNewLine ("Reading: " + $file)
  $strdata = Get-Content $file
  #Write-Host (" Lines: " + $strdata.Length)

  # Prepare string resources
  $script:rsrcs = ParseLngData $strdata

  $action = "add"

  if ($script:rsrcs.ContainsKey($id)) {
    if ($script:rsrcs[$id] -eq $str) { $color = "Green" } else { $color = "Red" }
    Write-Host -ForegroundColor $color "Key '$id' already exists: $($script:rsrcs[$id])"
    if ($force) {
      $rewrite = "Y"
    } else {
      Write-Host -NoNewLine -ForegroundColor White "Rewrite files? [y/N] "
      $rewrite = Read-Host
    }
    if ($rewrite -eq "Y") {
      $action = "set"
    } else {
      if (-Not ($loop)) {
        $host.SetShouldExit(101)
        exit
      }
      return
    }
  }

  #############
  $script:dst_ids = @()
  $script:dst_str = @()

  if ($header -ne "") {
    # enum LngResources
    if ($action -eq "set") {
      $iNextId = $action[$id]
    } else {
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
    }
    for ($i = 0; $i -lt $rsrch.Count; $i++) {
      if ($rsrch[$i].Trim() -eq $last_gen_ids_note) {
        break
      }
      $script:dst_ids += $rsrch[$i]
    }
    if ($action -eq "add") {
      $script:dst_ids += ("`t" + $id.PadRight(30) + "= " + [string]$iNextId + ",")
    }
    $script:dst_ids += "`t$last_gen_ids_note"
    $script:dst_ids += "`tlng_NextId"
    $script:dst_ids += "};"
  }

  #$script:dst_ids

  # static LngPredefined gsDataRsrcs[]
  for ($i = 0; $i -lt $strdata.Count; $i++) {
    $script:dst_str += $strdata[$i]
    if ($strdata[$i].StartsWith("static LngPredefined")) {
      break
    }
  }
  #
  if ($action -eq "set") {
    $script:rsrcs[$id] = $str
  } else {
    $script:rsrcs.Add($id, $str)
  }

  $list = New-Object System.Collections.ArrayList
  $list.AddRange($script:rsrcs.Keys)
  $list.Sort([System.StringComparer]::Ordinal);
  $list | % {
    $script:dst_str += ("`t{ " + ($_+",").PadRight($id_pad) + "L`"" + $script:rsrcs[$_] + "`" },")
  }
  $script:dst_str += "`t$last_gen_str_note"
  $script:dst_str += "};"

  #$script:dst_str

  if ($header -ne "") {
    Set-Content $header $script:dst_ids -Encoding UTF8
  }
  Set-Content $file $script:dst_str -Encoding UTF8

  if ($header -ne "") {
    Add-Type -AssemblyName System.Windows.Forms
    if ($str.Length -le 64) {
      $clip = "CLngRc::getRsrc($id/*`"$str`"*/)"
    } else {
      $clip = "CLngRc::getRsrc($id)"
    }
    [Windows.Forms.Clipboard]::SetText($clip);
  }

  if ($action -eq "add") {
    Write-Host -ForegroundColor Yellow "Resource created:"
  } else {
    Write-Host -ForegroundColor Yellow "Resource updated:"
  }
  if ($header -ne "") {
    Write-Host -ForegroundColor Green "  $($id)=$($iNextId)"
    Write-Host -ForegroundColor Yellow "Ready to paste:"
    Write-Host -ForegroundColor Green "  $clip"
  } else {
    Write-Host -ForegroundColor Green "  $($id)=`"$str`""
  }
  Write-Host -ForegroundColor Yellow "Don't forget to update ConEmu.l10n with rc2json.cmd!"

  return
}

# $loop is $TRUE when called from NewLngResourceLoop
function NewLngResource([string]$id,[string]$str,[bool]$loop=$FALSE)
{
  NewLngResourceHelper -id $id -str $str -loop $FALSE -file $rsrcs_h_file -header $rsrsids_h_file

  return

  #$script:str_id = @{}

  ##Write-Host -NoNewLine ("Reading: " + $rsrsids_h_file)
  #$rsrch  = Get-Content $rsrsids_h_file
  ##Write-Host (" Lines: " + $rsrch.Length)
  #ParseResIdsEnum $rsrch

  ##Write-Host -NoNewLine ("Reading: " + $rsrcs_h_file)
  #$strdata = Get-Content $rsrcs_h_file
  ##Write-Host (" Lines: " + $strdata.Length)

  ## Prepare string resources
  #$script:rsrcs = ParseLngData $strdata

  #if ($script:rsrcs.ContainsKey($id)) {
  #  Write-Host -ForegroundColor Red "Key '$id' already exists: $($script:rsrcs[$id])"
  #  if (-Not ($loop)) {
  #    $host.SetShouldExit(101)
  #    exit
  #  }
  #  return
  #}

  ##############
  #$script:dst_ids = @()
  #$script:dst_str = @()

  ## enum LngResources
  #$iNextId = ($script:str_id.Values | measure-object -Maximum).Maximum
  #if (($iNextId -eq $null) -Or ($iNextId -eq 0)) {
  #  Write-Host -ForegroundColor Red "lng_NextId was not found!"
  #  if (-Not ($loop)) {
  #    $host.SetShouldExit(101)
  #    exit
  #  }
  #  return
  #}
  #$iNextId ++
  #for ($i = 0; $i -lt $rsrch.Count; $i++) {
  #  if ($rsrch[$i].Trim() -eq $last_gen_ids_note) {
  #    break
  #  }
  #  $script:dst_ids += $rsrch[$i]
  #}
  #$script:dst_ids += ("`t" + $id.PadRight(30) + "= " + [string]$iNextId + ",")
  #$script:dst_ids += "`t$last_gen_ids_note"
  #$script:dst_ids += "`tlng_NextId"
  #$script:dst_ids += "};"

  ##$script:dst_ids

  ## static LngPredefined gsDataRsrcs[]
  #for ($i = 0; $i -lt $strdata.Count; $i++) {
  #  $script:dst_str += $strdata[$i]
  #  if ($strdata[$i].StartsWith("static LngPredefined")) {
  #    break
  #  }
  #}
  ##
  #$script:rsrcs.Add($id, $str)
  #$script:rsrcs.Keys | sort | % {
  #  $script:dst_str += ("`t{ " + ($_+",").PadRight(30) + "L`"" + $script:rsrcs[$_] + "`" },")
  #}
  #$script:dst_str += "`t$last_gen_str_note"
  #$script:dst_str += "};"

  ##$script:dst_str

  #Set-Content $rsrsids_h_file $script:dst_ids -Encoding UTF8
  #Set-Content $rsrcs_h_file   $script:dst_str -Encoding UTF8

  #Add-Type -AssemblyName System.Windows.Forms
  #if ($str.Length -le 64) {
  #  $clip = "CLngRc::getRsrc($id/*`"$str`"*/)"
  #} else {
  #  $clip = "CLngRc::getRsrc($id)"
  #}
  #[Windows.Forms.Clipboard]::SetText($clip);

  #Write-Host -ForegroundColor Yellow "Resource created:"
  #Write-Host -ForegroundColor Green "  $($id)=$($iNextId)"
  #Write-Host -ForegroundColor Yellow "Ready to paste:"
  #Write-Host -ForegroundColor Green "  $clip"
  #Write-Host -ForegroundColor Yellow "Don't forget to update ConEmu.l10n with rc2json.cmd!"

  #return
}

function NewCtrlHint([string]$id,[string]$str)
{
  $script:res_id = @{}
  #Write-Host -NoNewLine ("Reading: " + $resource_h_file)
  $resh  = Get-Content $resource_h_file
  #Write-Host (" Lines: " + $resh.Length)
  ParseResIds $resh

  if (-Not ($script:res_id.ContainsKey($id))) {
    Write-Host -ForegroundColor Red "ResourceID '$id' not found in 'resource.h'!"
    $host.SetShouldExit(101)
    exit
  }

  NewLngResourceHelper -id $id -str $str -loop $FALSE -file $hints_h_file -header ""
}

function NewLngResourceLoop()
{
  $script:rsrcs = ParseLngData (Get-Content $rsrcs_h_file)

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
    if ($script:rsrcs.ContainsKey($id)) {
      Write-Host -ForeGroundColor Green -NoNewLine "   Current: "
      Write-Host -ForeGroundColor Gray -NoNewLine "``"
      Write-Host -ForeGroundColor Yellow -NoNewLine $script:rsrcs[$id]
      Write-Host -ForeGroundColor Gray "``"
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



################################################
##############  Wiki Generation  ###############
################################################

function InitKeyNames()
{
  $script:KeysFriendly = @()
  $script:KeysFriendly += @{ Key = "VK_LWIN" ; Name = "Win" }
  $script:KeysFriendly += @{ Key = "VK_APPS" ; Name = "Apps" }
  $script:KeysFriendly += @{ Key = "VK_CONTROL" ; Name = "Ctrl" }
  $script:KeysFriendly += @{ Key = "VK_LCONTROL" ; Name = "RCtrl" }
  $script:KeysFriendly += @{ Key = "VK_RCONTROL" ; Name = "LCtrl" }
  $script:KeysFriendly += @{ Key = "VK_MENU" ; Name = "Alt" }
  $script:KeysFriendly += @{ Key = "VK_LMENU" ; Name = "RAlt" }
  $script:KeysFriendly += @{ Key = "VK_RMENU" ; Name = "LAlt" }
  $script:KeysFriendly += @{ Key = "VK_SHIFT" ; Name = "Shift" }
  $script:KeysFriendly += @{ Key = "VK_LSHIFT" ; Name = "RShift" }
  $script:KeysFriendly += @{ Key = "VK_RSHIFT" ; Name = "LShift" }
  $script:KeysFriendly += @{ Key = "VK_OEM_3/*~*/" ; Name = "'~'" }
  $script:KeysFriendly += @{ Key = "192/*VK_tilde*/" ; Name = "'~'" }
  $script:KeysFriendly += @{ Key = "VK_UP" ; Name = "UpArrow" }
  $script:KeysFriendly += @{ Key = "VK_DOWN" ; Name = "DownArrow" }
  $script:KeysFriendly += @{ Key = "VK_LEFT" ; Name = "LeftArrow" }
  $script:KeysFriendly += @{ Key = "VK_RIGHT" ; Name = "RightArrow" }
  $script:KeysFriendly += @{ Key = "VK_SPACE" ; Name = "Space" }
  $script:KeysFriendly += @{ Key = "VK_RETURN" ; Name = "Enter" }
  $script:KeysFriendly += @{ Key = "VK_PAUSE" ; Name = "Break" } # will be replaced with "Pause" if single
  $script:KeysFriendly += @{ Key = "VK_CANCEL" ; Name = "Break" }
  $script:KeysFriendly += @{ Key = "VK_LBUTTON" ; Name = "LeftMouseButton" }
  $script:KeysFriendly += @{ Key = "VK_RBUTTON" ; Name = "RightMouseButton" }
  $script:KeysFriendly += @{ Key = "VK_MBUTTON" ; Name = "MiddleMouseButton" }
  $script:KeysFriendly += @{ Key = "VK_ESCAPE" ; Name = "Esc" }
  $script:KeysFriendly += @{ Key = "VK_WHEEL_DOWN" ; Name = "WheelDown" }
  $script:KeysFriendly += @{ Key = "VK_WHEEL_UP" ; Name = "WheelUp" }
  $script:KeysFriendly += @{ Key = "VK_INSERT" ; Name = "Ins" }
  $script:KeysFriendly += @{ Key = "VK_TAB" ; Name = "Tab" }
  $script:KeysFriendly += @{ Key = "VK_HOME" ; Name = "Home" }
  $script:KeysFriendly += @{ Key = "VK_END" ; Name = "End" }
  $script:KeysFriendly += @{ Key = "0xbd/* -_ */" ; Name = "'-_'" }
  $script:KeysFriendly += @{ Key = "0xbb/* =+ */" ; Name = "'+='" }
  $script:KeysFriendly += @{ Key = "VK_DELETE" ; Name = "Delete" }
  $script:KeysFriendly += @{ Key = "VK_PRIOR" ; Name = "PageUp" }
  $script:KeysFriendly += @{ Key = "VK_NEXT" ; Name = "PageDown" }
  $script:KeysFriendly += @{ Key = "CEHOTKEY_ARRHOSTKEY" ; Name = "Win(configurable)" }
}

function FriendlyKeys($token)
{
  #Write-Host $token
  if (($token -eq "0") -Or ($token -eq "")) { return "*NoDefault*" }
  $mk = "MakeHotKey("
  if ($token.StartsWith($mk)) {
    $token = $token.SubString($mk.Length).Trim().TrimEnd(")").Replace(",","|")
  }
  for ($i = 0; $i -lt $script:KeysFriendly.Length; $i++) {
    $k = $script:KeysFriendly[$i]["Key"]
    $n = $script:KeysFriendly[$i]["Name"]
    #Write-Host ($k + " - " + $n)
    $token = $token.Replace($k,$n)
  }
  $token = $token.Replace("VK_","")
  #$token = $token.Replace("VK_LWIN","Win").Replace("VK_CONTROL","Ctrl").Replace("VK_LCONTROL","LCtrl").Replace("VK_RCONTROL","RCtrl")
  #$token = $token.Replace("VK_MENU","Alt").Replace("VK_LMENU","LAlt").Replace("VK_RMENU","RAlt")
  #$token = $token.Replace("VK_SHIFT","Shift").Replace("VK_LSHIFT","LShift").Replace("VK_RSHIFT","RShift")
  #$token = $token.Replace("VK_APPS","Apps").Replace("VK_LSHIFT","LShift").Replace("VK_RSHIFT","RShift")
  $i = $token.IndexOf("|")
  if ($i -gt 0) {
    $token = ($token.SubString($i+1)+"|"+$token.SubString(0,$i))
  }
  $i = $token.IndexOf(",")
  if ($i -gt 0) {
    $key = $token.SubString(0, $i).Trim()
    $mod = $token.SubString($i+1).Trim()
  } else {
    $key = $token.Trim()
    $mod = ""
  }
  if (($key.StartsWith("'")) -And ($key.EndsWith("'"))) {
    if ($key.Length -eq 3) {
      $key = $key.SubString(1,1)
    } else {
      $key = "‘"+$key.Trim("'")+"’"
    }
  }
  # ready
  if ($mod -ne "") { $ready = ($mod.Replace(",","+") + "+" + $key) } else { $ready = $key }
  # Show "Pause" instead of single "Break"
  if ($ready -eq "Break") { $ready = "Pause" }
  return $ready
}

function SplitHotkey($line)
{
  # 0             1         2     3               4                          5      6                       7
  # vkMultiClose, chk_User, NULL, L"Multi.Close", CConEmuCtrl::key_GuiMacro, false, L"Close(0)")->SetHotKey(VK_DELETE,VK_LWIN);
  $tokens = @()
  for ($i = 0; $i -le 8; $i++) { $tokens += @("") }

  #Write-Host -ForegroundColor DarkYellow "--`n$line"

  # First - cut of hotkeys themself
  $l = $line.IndexOf(")->SetHotKey(")
  if ($l -gt 0) {
    $keys = $line.SubString($l + ")->SetHotKey(".Length)
    $l2 = $keys.LastIndexOf(")")
    if ($l2 -gt 0) {
      $keys = $keys.Remove($l2)
    }
    $tokens[7] = $keys
    $line = $line.Remove($l)
    #Write-Host $line
  }
  # Now we are ready to parse the line
  for ($i = 0; ($i -le 5) -And ($line -ne ""); $i++) {
    $l = $line.IndexOf(",")
    if ($l -gt 0) {
      $tokens[$i] = $line.SubString(0, $l).Trim()
      $line = $line.Remove(0, $l+1).Trim()
    } else {
      $tokens[$i] = $line.Trim()
      $line = ""
    }
  }
  if (($line -ne "") -And ($line.StartsWith("L`""))) {
    $macro = $line.SubString(1).TrimEnd(' );')
    if ($macro.EndsWith("*/")) {
      $l = $macro.IndexOf("/*")
      if ($l -gt 0) {
        $macro = $macro.Substring(0, $l).TrimEnd()
      } else {
        $macro = ""
      }
    }
    #Write-Host -ForegroundColor DarkYellow "Macro: $macro"
    $tokens[6] = $macro
  }
  if (($tokens[3] -ne "") -And ($tokens[3].StartsWith("L`""))) {
    $tokens[3] = $tokens[3].Remove(0,1) #.Trim("`"")
    if ($tokens[3] -eq "`"`"") { $tokens[3] = "" }
  }
  
  #Write-Host ($tokens[0] + "; " + $tokens[3] + "; " + $tokens[1] + "; " + $tokens[7] + "; " + $tokens[3] + "; " + $tokens[6] + ";")

  $hk = @{}
  $hk.Add("Id",$tokens[0])
  $hk.Add("Type",$tokens[1].SubString(4))
  $hk.Add("Save",$tokens[3].Replace("`"","``"))
  $hk.Add("Hotkey",(FriendlyKeys $tokens[7]))
  $hk.Add("Macro",$tokens[6].Replace("`"","``"))

  #Write-Host -ForegroundColor Yellow (":: " + $hk.Id + " :: " + $hk.Hotkey + " :: " + $hk.Macro + " ::")

  #if ($hk.ContainsKey("Disable")) { $d = "Possible" } else { $d = "" }
  #if ($hk.ContainsKey("Hotkey")) { $k = $hk["Hotkey"] } else { $k = "" }
  #Write-Host ($hk["Id"].PadRight(25) + "`t" + $d.PadRight(10) + "`t" + $k.PadRight(30) + "`t" + $hk["Macro"])

  return $hk
}

function ParseHotkeys($hkln)
{
  #Write-Host $hkln
  $hk_lines = $hkln `
    | Where { $_.StartsWith("`tAdd`(vk") -Or $_.StartsWith("`t`t->SetHotKey") } `
    | Where { $_.IndexOf("vkGuiMacro") -eq -1 } `
    | Where { $_.IndexOf("vkConsole_") -eq -1 } # NEED TO BE ADDED MANUALLY !!!

  $script:hotkeys = @()

  #Write-Host -ForegroundColor DarkYellow "--`nParsing hotkeys..."

  for ($i = 0; $i -lt $hk_lines.Count; $i++) {
    $s = $hk_lines[$i]

    #Write-Host -ForegroundColor DarkYellow "--`n$s"

    $l = $s.LastIndexOf("//")
    if ($l -gt 0) { $s = $s.Remove($l).Trim() }

    $l = $s.LastIndexOf(")")
    if ($l -gt 0) {
      $line = $s.Trim().Remove(0,4)
      #Write-Host -ForegroundColor DarkYellow "--`n$line|"
      if ((($i+1) -lt $hk_lines.Count) -And ($hk_lines[$i+1].StartsWith("`t`t->SetHotKey"))) {
        $i++
        $s = $hk_lines[$i].Remove(0,2)

        $l = $s.LastIndexOf("//")
        if ($l -gt 0) { $s = $s.Remove($l).Trim() }

        $line += $s
      }
      #$line
      $hk = SplitHotkey $line

      #Write-Host "---"
      #$hk
      #Write-Host "***"

      $script:hotkeys += $hk
    }
  }

  return
}

function EscMd($txt)
{
  return $txt.Replace("&&","(and)").Replace("&","").Replace("*","``*``").TrimEnd(":")
}

function ReplaceRN($txt)
{
  return $txt.Replace("\`"","`"").
    Replace("\\r","<r>").Replace("\\n","<n>").
    Replace("\r\n"," ").Replace("\n"," ").
    Replace("<r>","\r").Replace("<n>","\n")
    #.Replace("`r`n"," ").Replace("`r"," ").Replace("`n"," ")
}

function WriteWiki($items_arg, $hints, $hotkeys, $dlgid, $name, $flname)
{
  $script:items = $items_arg

  $file_base = ($dest_md + $flname[0].Replace("-",""))
  $file_md = "$file_base.md"
  $file_html = "$file_base.html"

  if ([System.IO.File]::Exists($file_html)) {
    $src_content = ReadFileContent $file_html
  } elseif ([System.IO.File]::Exists($file_md)) {
    $src_content = ReadFileContent $file_md
  } else {
    Write-Host -ForegroundColor Red "Target file was not prepared (absent): $file_md"
    return
  }
  if ($src_content -eq "") {
    Write-Host -ForegroundColor Red "Target file was not prepared (empty): $file_md"
    return
  }
  $yaml_end = $src_content.IndexOf("`n---",3)
  if (($yaml_end -le 0) -Or ($yaml_end -eq $NULL)) {
    Write-Host -ForegroundColor Red "Target file was not prepared (no yaml, length=$($src_content.Length)): $file_md"
    #Write-Host ($src_content.SubString(0, 200))
    return
  }
  
  $file_table = ($dest_md + "\..\..project\tables\" + $flname[0].Replace("-","") + ".md")

  $yaml_header = $src_content.SubString(0,$yaml_end+4)

  $md_automsg = "$conemu_page_automsg`r`n{% comment %} $dlgid {% endcomment %}`r`n`r`n"

  # <img src="http://conemu-maximus5.googlecode.com/svn/files/ConEmuAnsi.png" title="ANSI X3.64 and Xterm 256 colors in ConEmu">

  $script:md = ("$yaml_header" + "`r`n`r`n" + "# Settings: $name" + "`r`n`r`n" + $md_automsg)

  $script:table = ("# Settings: " + $name + "`r`n")
  $script:table = $md_automsg


  # Screenshots
  for ($i = 0; $i -lt $flname.Length; $i++) {
    $img = ("![ConEmu Settings: $name]($md_img_path$($flname[$i]).png)`r`n`r`n")
    $script:md += $img
    $script:table += $img
  }

  $script:table += "| CtrlType | Text | ID | Position | Description |`r`n"
  $script:table += "|:---|:---|:---|:---|:---|`r`n"

  $script:ctrl_type = ""
  $script:ctrl_name = ""
  $script:ctrl_id = ""
  $script:ctrl_alt = ""
  $script:ctrl_desc = ""
  $script:list = @()
  $script:descs = @()
  $script:radio = $FALSE
  $script:track = $FALSE
  $script:dirty = $FALSE
  $script:wasgroup = $FALSE

  function HeaderFromId($ctrlid)
  {
    if (($ctrlid -ne "") -And ($ctrlid -ne "IDC_STATIC")) {
      if ($script:res_id.Contains($ctrlid)) {
        return "  {#id$($script:res_id[$ctrlid])}"
      } else {
        return "  {#id$ctrlid}"
      }
    }
    return ""
  }

  function DumpWiki()
  {
    if ($script:dirty) {
      $label = ""
      $alt = ""
      $desc = ""

      $add_md   = ""

      if ($script:radio) {

        if ($script:ctrl_alt -ne "") {
          $add_md   += ("**" + (EscMd $script:ctrl_alt) + "**`r`n`r`n") # Bold
        }

        if ($script:list.Length -gt 0) {
          $add_md   += "`r`n"
        }
        for ($i = 0; $i -lt $script:list.Length; $i++) {
          $add_md   += ("* **" + (EscMd $script:list[$i]) + "**") # Bold
          if ($script:descs[$i] -ne "") {
            $add_md   += (" " + (EscMd $script:descs[$i]))
          }
          $add_md   += "`r`n"
        }

      } else {

        $desc = $script:ctrl_desc

        if (($script:ctrl_type.Contains("TEXT")) -And ($desc -eq ""))  {
          $desc = $script:ctrl_alt
        }
        elseif ($script:ctrl_name -ne "") {
          $label = ReplaceRN $script:ctrl_name
          if ($script:ctrl_alt -ne "") { $alt = ReplaceRN $script:ctrl_alt }
        }
        elseif ($script:ctrl_alt -ne "") {
          $label = ReplaceRN $script:ctrl_alt
        }
        else {
          if ($script:ctrl_type -eq "EDIT") {
            $script:ctrl_name = "Edit box"
          }
        }

        if ($label -ne "") {
          if ($script:wasgroup) {
            StartGroup $label $script:ctrl_id
          } else {
            $add_md   += ("#### " + (EscMd $label)) # H4
            if ($script:ctrl_id -ne "") {
              $add_md += (HeaderFromId $script:ctrl_id)
            }
            $add_md   += "`r`n"
          }
        }

        if ($alt -ne "") {
          $add_md   += ("*" + (EscMd $alt) + "*  `r`n") # Italic
        }

        if (($desc -ne "") -And ($desc -ne $label) -And ($desc -ne $alt)) {
          $add_md   += (EscMd $desc)
        }

      }

      if ($add_md -ne "") {
        # Write-Host -ForegroundColor Gray $add_md
        $script:md += $add_md
      }
    }

    $script:md   += "`r`n`r`n"

    $script:ctrl_type = ""
    $script:ctrl_name = ""
    $script:ctrl_id = ""
    $script:ctrl_alt = ""
    $script:ctrl_desc = ""
    $script:list = @()
    $script:descs = @()
    $script:radio = $FALSE
    $script:track = $FALSE
    $script:dirty = $FALSE
    $script:wasgroup = $FALSE
  }

  function StartGroup($grptxt,$grpid="")
  {
    if ($grptxt -eq "") { $grptxt = "Group" }
    ### github md
    $script:md += ("## "+ (EscMd $grptxt))
    if ($grpid -ne "") { $script:md += (HeaderFromId $grpid) }
    $script:md += "`r`n`r`n"
    # clean
    $script:wasgroup = $FALSE
  }

  function GetHint($item_Id)
  {
    $hint = ""
    if (($item_Id -ne "") -And ($hints.ContainsKey($item_Id))) {
      $hint = ReplaceRN $hints[$item_Id]
    }
    return $hint
  }

  $script:wasgroup = $FALSE

  function AddTable()
  {
    # For debugging (information separate md file)
    $script:table += ("| " + $script:items[$i].Type + " | " + $script:items[$i].Title + " | " + `
      $script:items[$i].Id + " | (" + $script:items[$i].Y2 + ":" + [int]($script:items[$i].Y2/$linedelta) + ") " + `
      $script:items[$i].x+","+$script:items[$i].y+","+$script:items[$i].Width+","+$script:items[$i].Height + `
      " | " + (GetHint $script:items[$i].Id) + `
      " |`r`n")
  }

  function ParseGroupbox($sgTitle,$iFrom,$xFrom,$xTo,$yTo)
  {
    #Write-Host ("  GroupBox: " + $sgTitle + " iFrom=" + $iFrom + " x={" + $xFrom + ".." + $xTo + "} yTo=" + $yTo)
    $iDbg = 0

    for ($i = $iFrom; $i -lt $script:items.length; $i++) {

      if (($script:items[$i].x -lt $xFrom) -Or (($script:items[$i].x + $script:items[$i].Width) -gt $xTo) -Or (($script:items[$i].y + $script:items[$i].Height) -gt $yTo)) {
        continue # outside of processed GroupBox
      }

      #Write-Host ("    Item: " + $script:items[$i].Id + " " + $script:items[$i].Type + " " + $script:items[$i].Title)

      if ($script:items[$i].Type -eq "GROUPBOX") {
        # If any is pending...
        DumpWiki
        # Process current group
        $script:wasgroup = ($script:items[$i].Title.Trim() -eq "")
        if ($script:wasgroup) {
          # That is last element???
          if (($i+1) -ge $script:items.length) { $script:wasgroup = $FALSE }
          # If next element is BELOW group Y
          elseif (($script:items[$i].y) -lt ($script:items[$i+1].y)) { $script:wasgroup = $FALSE }
        }
        # Start new heading
        if (-Not $script:wasgroup) {
          StartGroup $script:items[$i].Title $script:items[$i].Id
        }

        # For debugging (information separate wiki file)
        AddTable

        $script:items[$i].Id = ""

        ParseGroupbox $script:items[$i].Title ($i+1) $script:items[$i].x ($script:items[$i].x + $script:items[$i].Width) ($script:items[$i].y + $script:items[$i].Height)

        continue
      }

      if ($script:items[$i].Id -eq "") {
        continue # already processed
      }

      if (($script:ctrl_type.Contains("TEXT")) -And ($script:items[$i].Type.Contains("TEXT"))) {
        DumpWiki
      } elseif (($script:radio) -And (-Not $script:items[$i].Type.Contains("RADIO"))) {
        DumpWiki
      }

      # For debugging (information separate wiki file)
      AddTable

      if ($script:items[$i].Type.Contains("TEXT")) {
        $txt = $script:items[$i].Title.Trim()
        if (($txt -eq "") -Or ($txt.CompareTo("x") -eq 0)) {
          continue
        }
        $hint = (GetHint $script:items[$i].Id)
        if ($hint -ne "") {
          Write-Host -ForegroundColor Red ("Hint skipped: " + $hint)
        }
        DumpWiki
        # Static text
        $script:dirty = $TRUE
        $script:ctrl_type = $script:items[$i].Type
        $script:ctrl_alt = $script:items[$i].Title
      }
      elseif ($script:items[$i].Type -eq "RADIOBUTTON") {
        if ((-Not $script:ctrl_type.Contains("TEXT")) -And ($script:ctrl_type -ne "RADIOBUTTON")) {
          DumpWiki
        }
        # Radio buttons
        $script:dirty = $TRUE
        $script:radio = $TRUE
        $script:ctrl_type = $script:items[$i].Type
        $script:list += $script:items[$i].Title
        $script:descs += (GetHint $script:items[$i].Id)
      }
      elseif ($script:items[$i].Type -eq "CHECKBOX") {
        if (($script:dirty) -And (-Not $script:ctrl_type.Contains("TEXT"))) {
          DumpWiki
        }
        # Check box
        $script:dirty = $TRUE
        $script:ctrl_type = $script:items[$i].Type
        $script:ctrl_name = $script:items[$i].Title
        $script:ctrl_id = $script:items[$i].Id
        $script:ctrl_desc = (GetHint $script:items[$i].Id)
      }
      elseif ($script:items[$i].Type.Contains("BOX")) {
        if (-Not $script:ctrl_type.Contains("TEXT")) {
          DumpWiki
        }
        # Combo or list box
        $script:dirty = $TRUE
        $script:ctrl_type = $script:items[$i].Type
        $script:ctrl_name = $script:items[$i].Title
        $script:ctrl_id = $script:items[$i].Id
        $script:ctrl_desc = (GetHint $script:items[$i].Id)
        if ($script:items[$i].Type -eq "DROPDOWNBOX") {
          # TODO : ListBox items (retrieve from sources) ?
        }
      }
      elseif ($script:items[$i].Type -eq "TRACKBAR") {
        if (-Not $script:ctrl_type.Contains("TEXT")) {
          DumpWiki
        }
        # Trackbar (may be followed by text)
        # Transparent .. Opaque
        # Darkening: ... [255]
        $script:dirty = $TRUE
        $script:ctrl_type = $script:items[$i].Type
        $script:track = $TRUE
        $script:ctrl_desc = (GetHint $script:items[$i].Id)
      }
      elseif ($script:items[$i].Type -eq "EDIT") {
        if (($script:dirty) -And (-Not $script:ctrl_type.Contains("TEXT"))) {
          DumpWiki
        }
        $script:dirty = $TRUE
        $script:ctrl_type = $script:items[$i].Type
        #if ($script:ctrl_name -eq "") { $script:ctrl_name = "Edit box" }
        $script:ctrl_desc = (GetHint $script:items[$i].Id)
      }
      elseif ($script:items[$i].Type.Contains("BUTTON")) {
        DumpWiki
        if ($script:items[$i].Title.Trim().Trim('.').Length -ge 1) {
          $script:dirty = $TRUE
          $script:ctrl_type = $script:items[$i].Type
          $script:ctrl_name = $script:items[$i].Title
          $script:ctrl_id = $script:items[$i].Id
          $script:ctrl_desc = (GetHint $script:items[$i].Id)
        }
      }
      else {
        if (-Not $script:ctrl_type.Contains("TEXT")) {
          DumpWiki
        }
        $hint = (GetHint $script:items[$i].Id)
        if ($hint -ne "") {
          $script:dirty = $TRUE
          $script:ctrl_type = $script:items[$i].Type
          $script:ctrl_desc = $hint
        }
      }

      $script:items[$i].Id = ""
    }

    DumpWiki
  }

  ParseGroupbox "" 0 0 9999 9999

  # Informational
  Set-Content -Path $file_table -Value $script:table -Encoding UTF8

  # github md (!NO BOM!)
  # [System.IO.File]::WriteAllLines($file_md, $script:md)
  $file_md
  $old_cont = ReadFileContent $file_md
  if (($old_cont -ne "") -And -Not ($old_cont.Contains($conemu_page_automsg))) {
    Write-Host "  File was changed manually, writing automatic content to:"
    $file_md = $file_md.SubString(0, $file_md.Length-3) + "-auto.md"
    Write-Host "  $file_md"
  }
  #Set-Content -Path $file_md -Value $script:md -Encoding UTF8
  WriteFileContent $file_md $script:md
}

function WriteHotkeys($hints, $hotkeys, $flname)
{
  function GetHint($item_Id)
  {
    $hint = ""
    if (($item_Id -ne "") -And ($hints.ContainsKey($item_Id))) {
      $hint = ReplaceRN $hints[$item_Id]
    }
    return $hint
  }

  ### github md
  $apps = "``Apps`` key is a key between RWin and RCtrl."
  $script:keys_md   = ("$conemu_page_hotkeymark`r`n`r`n" + "*Note* "+$apps+"`r`n`r`n" +
    "| **Hotkey** | **SaveName/GuiMacro** | **Description** |`r`n" +
    "|:---|:---|:---|:---|`r`n")

  #Write-Host $hotkeys

  Write-Host "Hotkey count: $($hotkeys.Length)"
  for ($i = 0; $i -lt $hotkeys.Length; $i++) {
    $hk = $hotkeys[$i]
    if ($hk.Id.StartsWith("vkPicView")) { continue }
    #Write-Host "HK[$i]---------------"
    #$hk
    if ($($hk.Save) -ne "") { $svgm = $($hk.Save).TrimEnd(")") } else { $svgm = "``-``" }
    if ($($hk.Macro) -ne "") { $svgm += " <br/> $($hk.Macro)" }
    $script:keys_md += ("| $($hk.Hotkey) | $svgm | " + (GetHint $hk.Id) + " |`r`n")
  }

  $flname
  $old_cont = ReadFileContent $flname
  $l = $old_cont.IndexOf($conemu_page_hotkeymark)
  if ($l -le 0) {
    Write-Host "  File was changed manually, required marker not found"
    return
  }
  $new_cont = $old_cont.SubString(0, $l) + $script:keys_md
  
  WriteFileContent $flname $new_cont
}

function ParseDialogWiki($rcln, $hints, $hotkeys, $dlgid, $name, $flname)
{
  $items_arg = ParseDialogData $rcln $dlgid $name

  Write-Host ("  Controls: " + $items_arg.Count)

  WriteWiki ($items_arg | sort {((1000*[int]($_.Y2/$linedelta))+[int]$_.X)}) $hints $hotkeys $dlgid $name $flname.Split(",")
}

function GenerateWikiFiles()
{
  InitializeJsonData

  InitDialogList

  InitKeyNames

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

  Write-Host -NoNewLine ("Reading: " + $conemu_hotkeys_file)
  $hkln  = Get-Content $conemu_hotkeys_file
  if ($hkln -eq $null) { return }
  Write-Host (" Lines: " + $hkln.Length)


  # Preparse hints and hotkeys
  $script:hints = ParseLngData $rchints
  # Prepare string resources
  $script:rsrcs = ParseLngData $strdata
  #Write-Host -ForegroundColor Red "`n`nHints list"

  ParseHotkeys $hkln
  #Write-Host ("Count: " + $script:hotkeys.Length)
  WriteHotkeys $script:hints $script:hotkeys $conemu_hotkeys_md

  #Write-Host -ForegroundColor Red "`n`nHotkeys list"
  #$hotkeys

  # Parse sources and write wiki/md pages
  for ($p = 0; $p -lt $script:dialogs.length; $p++) {
    $dlg = $script:dialogs[$p]
    if (($dlg.file -ne $null) -And ($dlg.file -ne "")) {
      ParseDialogWiki $rcln $script:hints $script:hotkeys $dlg.id $dlg.name.Trim() $dlg.file
    }
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
    if ($id.StartsWith("lng_")) {
      NewLngResource $id $str
    } else {
      NewCtrlHint $id $str
    }
  }
} elseif ($mode -eq "wiki") {
  GenerateWikiFiles
}
