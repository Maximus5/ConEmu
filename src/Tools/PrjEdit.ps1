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
    if ($item.GetAttribute($attr) -gt $file) {
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
          if (-Not $cpp[$j].StartsWith(".")) { $cpp[$j] = ".\" + $cpp[$j] }
          Prj-AppendFile $igi $cpp[$j] "File" "RelativePath"
        }
        $cpp = @()
      } elseif (($hpp.length -gt 0) -And ($igi.Filter.Contains("hpp"))) {
        for ($j = 0; $j -lt $hpp.length; $j++) {
          if (-Not $hpp[$j].StartsWith(".")) { $hpp[$j] = ".\" + $hpp[$j] }
          Prj-AppendFile $igi $hpp[$j] "File" "RelativePath"
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

function Prj-List([String]$path)
{
  "Listing: $Script_Working_path"

  #$extlist = @("*.vcproj")
  $extlist = @("*.vcproj", "*.vcxproj")

  (Get-ChildItem -Path (Join-Path $path "*") -Include $extlist) | foreach {
    $xml_path = Join-Path $path $_.Name
      ("    prj: " + $_.Name)
      $x = Prj-XmlEdit $xml_path $script:add_cpp $script:add_hpp
      #$xml_path2 = $xml_path + ".new"
      #$x.Save($xml_path2)
      $x.Save($xml_path)
  }
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

if (($script:add_cpp.length -eq 0) -And ($script:add_hpp.length -eq 0)) {
  if ($force_rewrite) {
    Write-Host -Foregroundcolor Yellow "There is nothing to add, project will be just rewrited"
  } else {
    Write-Host -Foregroundcolor Red "There is nothing to add!"
    return
  }
}

Prj-List $Script_Working_path
