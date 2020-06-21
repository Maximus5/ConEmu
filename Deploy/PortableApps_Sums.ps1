param([String]$file)

function CreateHash($path,$aname)
{
    $algo = [System.Security.Cryptography.HashAlgorithm]::Create($aname)
    $stream = New-Object System.IO.FileStream($path, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read)

    $md5StringBuilder = New-Object System.Text.StringBuilder
    $algo.ComputeHash($stream) | % { [void] $md5StringBuilder.Append($_.ToString("x2")) }

    $stream.Dispose()

    $md5StringBuilder.ToString()
}

$fullPath = Resolve-Path $file

$md5 = CreateHash -path $fullPath -aname "MD5"

$f = Get-Item $fullPath

Write-Host (Get-Item $file).Name (("{0:N1}" -f ($f.Length/(1024*1024)))+"MB "+$md5)
