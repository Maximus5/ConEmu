param([String]$workmode="edit",[String]$add_cpp="",[String]$add_hpp="")

$workmode
$add_cpp
$add_hpp

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

function Prj-XmlEdit([String]$project, [String]$cpp="", [String]$hpp="")
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
      if (($cpp -ne "") -And ($igi.Filter.Contains("cpp"))) {
        if (-Not $cpp.StartsWith(".")) { $cpp = ".\" + $cpp }
        Prj-AppendFile $igi $cpp "File" "RelativePath"
        $cpp = ""
      } elseif (($hpp -ne "") -And ($igi.Filter.Contains("hpp"))) {
        if (-Not $hpp.StartsWith(".")) { $hpp = ".\" + $hpp }
        Prj-AppendFile $igi $hpp "File" "RelativePath"
        $hpp = ""
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
      if (($cpp -ne "") -And ($igi.SelectSingleNode("ns:ClCompile", $ns) -ne $null)) {
        Prj-AppendFile $igi $cpp "ClCompile" "Include"
        $cpp = ""
      } elseif (($hpp -ne "") -And ($igi.SelectSingleNode("ns:ClInclude", $ns) -ne $null)) {
        Prj-AppendFile $igi $hpp "ClInclude" "Include"
        $hpp = ""
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
    #Prj-LoadXml 
    if ($workmode -eq "edit") {
      ("    prj: " + $_.Name)
      $x = Prj-XmlEdit $xml_path $add_cpp $add_hpp
      #$xml_path2 = $xml_path + ".new"
      #$x.Save($xml_path2)
      $x.Save($xml_path)
    }
  }
}

Prj-List $Script_Working_path
