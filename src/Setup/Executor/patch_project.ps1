
$ErrorActionPreference = "Stop"

$changed = $False
$projectName = "Executor.vcxproj"
$projectFile = Join-Path (Get-Location).Path $projectName
$xml = [xml](Get-Content $projectFile)

$ns = New-Object System.Xml.XmlNamespaceManager($xml.NameTable)
$ns.AddNamespace("ns", $xml.DocumentElement.NamespaceURI)

$uacNode = $xml.Project.ItemDefinitionGroup.Link.SelectSingleNode("ns:UACExecutionLevel", $ns)

if ($uacNode -eq $null) {
  Write-Host "No <UACExecutionLevel> node in the project $projectName"
  $uacNode = $xml.CreateElement("UACExecutionLevel", $ns.LookupNamespace("ns"))
  $uacNode.set_InnerText("RequireAdministrator")
  $newNode = $xml.Project.ItemDefinitionGroup.Link.AppendChild($uacNode)
  $changed = $True
} else {
  Write-Host "The file $projectName already have <UACExecutionLevel> node"
  if ($uacNode.InnerText -ne "RequireAdministrator") {
    Write-Host "Changing <UACExecutionLevel> to RequireAdministrator"
    $uacNode.set_InnerText("RequireAdministrator")
    $changed = $True
  }
}

if ($changed) {
  $xml.Save($projectFile)
  Write-Host "The file $projectName was updated"
}
