#param([String]$workmode="edit",[String]$add_cpp="",[String]$add_hpp="")

$script:add_cpp = @()
$script:add_hpp = @()

$script:cpp_filter = @(".cpp", ".c")
$script:hpp_filter = @(".hpp", ".h")

$Script_Working_path = (Get-Location).Path
$script:namespace = ""
$script:ns = "ns:"

function Prj-AppendFile($root, $file, $node, $attr)
{
  if ($script:ns -eq "") {
    $cur = $igi.SelectNodes($node)
  } else {
    $cur = $igi.SelectNodes(($script:ns+$node), $ns)
  }

  $before = $null

  for ($i = 0; $i -lt $cur.Count; $i++) {
    $item = $cur[$i]
    $cur_attr = $item.GetAttribute($attr)
    # Do not add it twice
    if ($cur_attr -eq $file) {
      return
    }
    if ($cur_attr -gt $file) {
      $before = $item
      break
    }
  }

  if ($script:ns -eq "") {
    $s = $x.CreateElement($node)
  } else {
    $s = $x.CreateElement($node, $script:namespace)
  }

  $v = $s.SetAttribute($attr, $file)
  if ($before -eq $null) {
    $v = $root.AppendChild($s)
  } else {
    $v = $root.InsertBefore($s, $before)
  }
}

function Prj-XmlEdit([String]$project, [Array]$cpp, [Array]$hpp)
{
  $x = [xml](Get-Content $project)

  if ($x.DocumentElement.NamespaceURI -eq "") {
    # VC 9.0 format
    $script:namespace = ""
    $script:ns = ""
    $ig = $x.SelectNodes("//VisualStudioProject/Files/Filter")

    for ($i = 0; $i -lt $ig.Count; $i++) {
      # List files?
      $igi = $ig[$i]
      # cpp
      if (($cpp.length -gt 0) -And ($igi.Filter.Contains("cpp"))) {
        for ($j = 0; $j -lt $cpp.length; $j++) {
          if (-Not $cpp[$j].StartsWith(".")) { $add = ".\" + $cpp[$j] } else { $add = $cpp[$j] }
          Prj-AppendFile $igi $add "File" "RelativePath"
        }
        $cpp = @()
      } elseif (($hpp.length -gt 0) -And ($igi.Filter.Contains("hpp"))) {
        for ($j = 0; $j -lt $hpp.length; $j++) {
          if (-Not $hpp[$j].StartsWith(".")) { $add = ".\" + $hpp[$j] } else { $add = $hpp[$j] }
          Prj-AppendFile $igi $add "File" "RelativePath"
        }
        $hpp = @()
      }
    }

  } else {
    # VC 10.0+ format
    $ns = New-Object System.Xml.XmlNamespaceManager($x.NameTable)
    $script:namespace = $x.DocumentElement.NamespaceURI
    $script:ns = "ns:"
    $ns.AddNamespace("ns", $script:namespace)
    
    # $x.SelectSingleNode("//ns:Project", $ns)
    $ig = $x.SelectNodes("//ns:Project/ns:ItemGroup", $ns)

    for ($i = 0; $i -lt $ig.Count; $i++) {
      # List files?
      $igi = $ig[$i]
      # cpp
      if (($cpp.length -gt 0) -And ($igi.SelectSingleNode("ns:ClCompile", $ns) -ne $null)) {
        for ($j = 0; $j -lt $cpp.length; $j++) {
          Prj-AppendFile $igi $cpp[$j] "ClCompile" "Include"
        }
        $cpp = @()
      } elseif (($hpp.length -gt 0) -And ($igi.SelectSingleNode("ns:ClInclude", $ns) -ne $null)) {
        for ($j = 0; $j -lt $hpp.length; $j++) {
          Prj-AppendFile $igi $hpp[$j] "ClInclude" "Include"
        }
        $hpp = @()
      }
    }
  }

  return $x
}

function Prj-GccMake([String]$project, [Array]$cpp, [Array]$hpp)
{
  if ($cpp.length -eq 0) { return }

  $make = Get-Content $project
  $ready = @()
  $changed = $false
  $step = 0

  foreach ($s in $make)
  {
    if ($step -eq 0) {
      if ($s.StartsWith("SRCS =")) {
        $step = 1
        $make_add = @()
        foreach ($n in $cpp) { $make_add += $n }
      }
      $ready += $s
    } elseif ($step -eq 1) {
      if ($s -eq "") {
        $step = 2
      } else {
        for ($i = 0; $i -lt $make_add.length; $i++) {
          $n = $make_add[$i]
          if ($n -eq "") { continue }
          if ($n.StartsWith(".")) { $pad = 31 } else { $pad = 21 }
          $add = ("`t`t" + $n.Replace("\","/"))
          if ($s.StartsWith($add)) {
            # already exists
            $make_add[$i] = ""
            continue
          }
          if ($s -gt $add) {
            $ready += ($add.PadRight($pad) + "\")
            $make_add[$i] = ""
            $changed = $true
          }
        }
      }
      $ready += $s
    } else {
      $ready += $s
    }
  }

  if ($changed) {
    Set-Content $project $ready
  }

  return
}

function Prj-VcMake([String]$project, [Array]$cpp, [Array]$hpp)
{
  if ($cpp.length -eq 0) { return }

  $make = Get-Content $project
  $ready = @()
  $changed = $false
  $step = 0

  foreach ($s in $make)
  {
    if ($step -eq 0) {
      if ($s.StartsWith("LINK_OBJS =")) {
        $step = 1
        $make_obj = @()
        foreach ($n in $cpp) { $make_obj += $n }
      }
      $ready += $s
    } elseif ($step -eq 1) {
      # $(INTDIR)\AboutDlg.obj \ ...
      if ($s -eq "") {
        $step = 2
      } else {
        for ($i = 0; $i -lt $make_obj.length; $i++) {
          $n = $make_obj[$i]
          if ($n -eq "") { continue }
          $add = ("`$(INTDIR)\" + [System.IO.Path]::GetFileNameWithoutExtension($n) + ".obj \")
          if ($s -eq $add) {
            # already exists
            $make_obj[$i] = ""
            continue
          }
          if ($s -gt $add) {
            $ready += $add
            $make_obj[$i] = ""
            $changed = $true
          }
        }
      }
      $ready += $s
    } elseif ($step -eq 2) {
      if ($s -eq "#LINK_OBJS begin") {
        $step = 3
        $make_obj = @()
        $make_src = @()
        foreach ($n in $cpp) {
          if ($n -eq "") { continue }
          $obj = ("`$(INTDIR)\" + [System.IO.Path]::GetFileNameWithoutExtension($n) + ".obj: ")
          $add = $n.Replace("\", "/")
          foreach ($h in $hpp) {
            $add += (" " + $h.Replace("\", "/"))
          }
          $make_obj += $obj
          $make_src += ($obj + $add)
        }
      }
      $ready += $s
    } elseif ($step -eq 3) {
      if ($s -eq "#LINK_OBJS end") {
        $step = 4
        $ready += $s
      } elseif ($s -ne "") {
        # $(INTDIR)\AboutDlg.obj: AboutDlg.cpp AboutDlg.h About.h ../ConEmuCD/ConsoleHelp.h
        $jjj = 0
        
        for ($i = 0; $i -lt $make_obj.length; $i++) {
          $obj = $make_obj[$i]
          if ($obj -eq "") { continue }
          $add = $make_src[$i]

          if ($s.StartsWith($obj)) {
            # already exists, just update src and headers
            $s = $add
            $make_obj[$i] = ""
            continue
          }
          if ($s -gt $obj) {
            $ready += $add
            $ready += ""
            $make_obj[$i] = ""
            $changed = $true
          }
        }

        $ready += $s
      } else {
        $ready += $s
      }
      #$ready += $s
    } else {
      $ready += $s
    }
  }

  if ($changed) {
    Set-Content $project $ready
  }

  return
}

function Prj-CheckName([String]$file)
{
  $file = $file.Replace("/","\")
  $ext = [System.IO.Path]::GetExtension($file)
  if ($ext -ne "") {
    if ($script:cpp_filter -contains $ext) {
      $script:add_cpp += $file
    } elseif ($script:hpp_filter -contains $ext) {
      $script:add_hpp += $file
    }
  }
}

#$xml_path = "T:\VCProject\Maximus5\ConEmu\src\ConEmu\ConEmu09.vcproj"
#Prj-VcMake "T:\VCProject\Maximus5\ConEmu\src\ConEmu\makefile_vc" @("DpiAware.cpp") @("DpiAware.h")
#$x = Prj-XmlEdit $xml_path @("DpiAware.cpp") @("DpiAware.h")
#$x.Save($xml_path)
#return

$force_rewrite = $false

# parse arguments (file names passed)
for ($i = 0; $i -lt $args.length; $i++)
{
  if ($args[$i] -is [System.String]) {
    if (($args[$i] -eq "force") -Or ($args[$i] -eq "-force")) {
      $force_rewrite = $true
    } else {
      Prj-CheckName $args[$i]
    }
  } elseif ($args[$i] -is [System.Array]) {
    $arr = $args[$i]
    for ($j = 0; $j -lt $arr.length; $j++) {
      if ($arr[$j] -is [System.String]) {
        Prj-CheckName $arr[$j]
      }
    }
  }
}

#if (($script:add_cpp.length -eq 0) -And ($script:add_hpp.length -eq 0)) {
#  $script:add_cpp = @("DpiAware.cpp")
#  $script:add_hpp = @("DpiAware.h")
#}

if (($script:add_cpp.length -eq 0) -And ($script:add_hpp.length -eq 0)) {
  if ($force_rewrite) {
    Write-Host -Foregroundcolor Yellow "There is nothing to add, project will be just rewrited"
  } else {
    Write-Host -Foregroundcolor Red "There is nothing to add!"
    return
  }
}

function Prj-List([String]$path)
{
  "Listing: $Script_Working_path"

  #$extlist = @("*.vcproj")
  #$extlist = @("makefile_vc")
  $extlist = @("*.vcproj", "*.vcxproj", "makefile_vc", "makefile_gcc")

  (Get-ChildItem -Path (Join-Path $path "*") -Include $extlist) | foreach {
    $xml_path = Join-Path $path $_.Name
    ("    prj: " + $_.Name)
    $ext = [System.IO.Path]::GetExtension($_.Name)
    if (($ext -eq ".vcproj") -Or ($ext -eq ".vcxproj")) {
      $x = Prj-XmlEdit $xml_path $script:add_cpp $script:add_hpp
      $x.Save($xml_path)
      #$xml_path2 = $xml_path + ".new"
      #$x.Save($xml_path2)
    } elseif ($_.Name -eq "makefile_gcc") {
      Prj-GccMake $xml_path $script:add_cpp $script:add_hpp
    } elseif ($_.Name -eq "makefile_vc") {
      Prj-VcMake $xml_path $script:add_cpp $script:add_hpp
    }
  }
}

Prj-List $Script_Working_path
