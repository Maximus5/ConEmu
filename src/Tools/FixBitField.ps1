param([string]$file)

# New versions of GIMP saves 32-bit bitmaps as BI_BITFIELDS
# Moreover, it has non-standard size of bitmap header structure
# That is not intended for use with WinApi ::LoadImage

$bytes = [System.IO.File]::ReadAllBytes($file)
# 'BM'
if (($bytes[0] -ne 66) -or ($bytes[1] -ne 77)) {
  Write-Host -ForegroundColor Red "Not a bitmap"
  $host.SetShouldExit(101)
  return
}

$update = $FALSE

if ($bytes[0x1E] -eq 0) {
  Write-Host -ForegroundColor Green "Already BI_RGB"

  if ($bytes[0x0E] -ne 0x28) {
    Write-Host -ForegroundColor Yellow "Size of the header was 0x38"
    $bytes[0x0E] = 0x28
    # Update the file
    $update = $TRUE
  }

} elseif ($bytes[0x1E] -eq 3) {
  Write-Host -ForegroundColor Yellow "Found BI_BITFIELDS encoding"

  $Offset = $bytes[10] + 256 * ($bytes[11] + 256 * ($bytes[12] + 256 * ($bytes[13])))
  Write-Host ("Bits offset is " + [string]$Offset)

  while ($Offset -lt $bytes.Count) {
    $bb = @($bytes[$Offset+1], $bytes[$Offset+2], $bytes[$Offset+3], $bytes[$Offset+0])
    for ($i = 0; $i -lt 4; $i++) {
      $bytes[$Offset+$i] = $bb[$i]
    }

    $Offset += 4
  }

  # Set to BI_RGB
  $bytes[0x1E] = 0

  if ($bytes[0x0E] -ne 0x28) {
    Write-Host -ForegroundColor Yellow "Size of the header was 0x38"
    $bytes[0x0E] = 0x28
  }

  # Update the file
  $update = $TRUE

} else {
  Write-Host -ForegroundColor Red "Not a bitmap"
  $host.SetShouldExit(101)
  return
}

if ($update) {
  [System.IO.File]::WriteAllBytes($file, $bytes)
  Write-Host "File was updated: $file"
}
