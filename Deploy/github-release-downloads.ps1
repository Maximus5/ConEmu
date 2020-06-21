$rels = (conemuc -download https://api.github.com/repos/Maximus5/ConEmu/releases - 2>$null)
$rel = ConvertFrom-JSON $rels
$rel | foreach { Write-Host -NoNewLine "$($_.name): "; $total=0; $_.Assets | foreach { $total+=$_.download_count; Write-Host -NoNewLine " $($_.download_count)"}; Write-Host ";   Total $total"; }; Write-Host "Done"
