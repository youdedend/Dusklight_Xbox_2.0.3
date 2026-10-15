[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$InputDirectory,
    [Parameter(Mandatory = $true)][string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null

$Packages = @(Get-ChildItem -LiteralPath $InputDirectory -Recurse -File | Where-Object {
    $_.Extension -in @('.appx', '.msix') -and $_.FullName -notmatch '[\\/]Dependencies[\\/]'
})
if ($Packages.Count -ne 1) { throw "Expected one main unsigned package; found $($Packages.Count)." }
$ExpectedSourceSha256 = '11c4a8deebe4fe72405061900ad65ce1a5ba42a72ba3aff192cdfa764aae3150'
if ((Get-FileHash -LiteralPath $Packages[0].FullName -Algorithm SHA256).Hash.ToLowerInvariant() -ne $ExpectedSourceSha256) {
    throw 'Unsigned package hash does not match the verified build artifact.'
}


Add-Type -AssemblyName System.IO.Compression.FileSystem
$Archive = [System.IO.Compression.ZipFile]::OpenRead($Packages[0].FullName)
try {
    if ($Archive.GetEntry('AppxSignature.p7x')) { throw 'Input package is already signed; refusing to replace its signature.' }
    $Entry = $Archive.GetEntry('AppxManifest.xml')
    if (-not $Entry) { throw 'Package has no AppxManifest.xml.' }
    $Reader = [System.IO.StreamReader]::new($Entry.Open())
    try { [xml]$Manifest = $Reader.ReadToEnd() } finally { $Reader.Dispose() }
} finally { $Archive.Dispose() }
$Identity = $Manifest.SelectSingleNode('/*[local-name()="Package"]/*[local-name()="Identity"]')
$Publisher = $Identity.GetAttribute('Publisher')
$Version = $Identity.GetAttribute('Version')
if ($Identity.GetAttribute('Name') -ne 'Dusklight' -or $Version -ne '2.0.3.0' -or $Identity.GetAttribute('ProcessorArchitecture') -ne 'x64') {
    throw 'Input package is not the expected Dusklight 2.0.3.0 x64 candidate.'
}
if ($Publisher -ne 'CN=odolwa') { throw "Unexpected publisher: $Publisher" }

$SdkBin = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits/10/bin'
$Sdk = Get-ChildItem -LiteralPath $SdkBin -Directory | Where-Object { $_.Name -match '^\d+\.\d+\.\d+\.\d+$' } |
    Sort-Object { [version]$_.Name } -Descending |
    Where-Object { Test-Path (Join-Path $_.FullName 'x64/signtool.exe') } | Select-Object -First 1
if (-not $Sdk) { throw 'Windows SDK x64 SignTool was not found.' }
$SignTool = Join-Path $Sdk.FullName 'x64/signtool.exe'

$Certificate = $null
$RootTrustPath = $null
$PeopleTrustPath = $null
try {
    # The key is non-exportable and stays in this temporary runner's certificate store.
    $Certificate = New-SelfSignedCertificate -Type Custom -Subject $Publisher -KeyUsage DigitalSignature `
        -FriendlyName 'Dusklight UWP temporary test signing' -CertStoreLocation 'Cert:\CurrentUser\My' `
        -KeyAlgorithm RSA -KeyLength 2048 -HashAlgorithm SHA256 -KeyExportPolicy NonExportable `
        -NotAfter (Get-Date).AddYears(1) `
        -TextExtension @('2.5.29.37={text}1.3.6.1.5.5.7.3.3', '2.5.29.19={text}')
    $CerPath = Join-Path $OutputDirectory 'Dusklight-Test.cer'
    Export-Certificate -Cert $Certificate -FilePath $CerPath -Type CERT | Out-Null

    # A single MSIX uses the AppX package container. Copy before signing so the
    # supplied file has the .appx extension commonly used by Xbox Device Portal.
    $SignedPath = Join-Path $OutputDirectory 'Dusklight_2.0.3.0_x64_Test.appx'
    Copy-Item -LiteralPath $Packages[0].FullName -Destination $SignedPath
    & $SignTool sign /fd SHA256 /sha1 $Certificate.Thumbprint /s My /v $SignedPath 2>&1 | ForEach-Object { Write-Host $_ }
    if ($LASTEXITCODE -ne 0) { throw "SignTool signing failed: $LASTEXITCODE" }

    # Local trust is only for verification on this ephemeral runner; it does not
    # install or trust the certificate on the user's PC or Xbox.
    Import-Certificate -FilePath $CerPath -CertStoreLocation 'Cert:\CurrentUser\Root' | Out-Null
    $RootTrustPath = "Cert:\CurrentUser\Root\$($Certificate.Thumbprint)"
    Import-Certificate -FilePath $CerPath -CertStoreLocation 'Cert:\CurrentUser\TrustedPeople' | Out-Null
    $PeopleTrustPath = "Cert:\CurrentUser\TrustedPeople\$($Certificate.Thumbprint)"
    & $SignTool verify /pa /v $SignedPath 2>&1 | ForEach-Object { Write-Host $_ }
    if ($LASTEXITCODE -ne 0) { throw "SignTool verification failed: $LASTEXITCODE" }

    $SignedArchive = [System.IO.Compression.ZipFile]::OpenRead($SignedPath)
    try {
        if (-not $SignedArchive.GetEntry('AppxSignature.p7x')) { throw 'No embedded package signature was generated.' }
    } finally { $SignedArchive.Dispose() }

    # Microsoft framework dependencies keep Microsoft's original signatures.
    $Dependency = @(Get-ChildItem -LiteralPath $InputDirectory -Recurse -File | Where-Object {
        $_.FullName -match '[\\/]Dependencies[\\/]x64[\\/]' -and $_.Extension -eq '.appx'
    })
    foreach ($Item in $Dependency) {
        $Destination = Join-Path $OutputDirectory 'Dependencies/x64'
        New-Item -ItemType Directory -Force -Path $Destination | Out-Null
        Copy-Item -LiteralPath $Item.FullName -Destination (Join-Path $Destination $Item.Name)
    }

    $DeviceFamily = $Manifest.SelectSingleNode('/*[local-name()="Package"]/*[local-name()="Dependencies"]/*[local-name()="TargetDeviceFamily"]')
    $Metadata = [ordered]@{
        package = [System.IO.Path]::GetFileName($SignedPath)
        version = $Version
        publisher = $Publisher
        certificate_subject = $Certificate.Subject
        certificate_thumbprint = $Certificate.Thumbprint
        certificate_expires_utc = $Certificate.NotAfter.ToUniversalTime().ToString('o')
        minimum_os_version = $DeviceFamily.GetAttribute('MinVersion')
        sha256 = (Get-FileHash -LiteralPath $SignedPath -Algorithm SHA256).Hash.ToLowerInvariant()
        signing = 'self-signed test certificate; no timestamp'
        verification = 'SignTool /pa /v passed with this test certificate trusted on the temporary runner'
        private_key_exported = $false
        xbox_runtime_tested = $false
    }
    $Metadata | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $OutputDirectory 'signing-info.json') -Encoding utf8

    @'
# Dusklight 2.0.3.0 x64 — self-signed test package

Contents:
- Dusklight_2.0.3.0_x64_Test.appx: signed UWP candidate.
- Dusklight-Test.cer: PUBLIC certificate only; not a private key.
- Dependencies/x64/: Microsoft-signed framework dependency, unchanged.
- signing-info.json: hash, publisher, certificate details and minimum OS version.

Back up your existing saves first. Do not uninstall 1.1.1 just to try this update:
uninstalling can erase its local app data. This new test certificate does not
guarantee an in-place update of a package signed with your previous certificate.

Windows sideloading:
1. Inspect the certificate and verify its thumbprint against signing-info.json.
2. If you trust this test build, use the certificate import wizard to import the
   PUBLIC .cer into Local Machine > Trusted People (administrator approval).
3. Install the signed AppX, supplying the x64 VCLibs dependency if required.
   This does not require disabling Windows signature verification.

Xbox:
Use an Xbox in Developer Mode and its authenticated Device Portal to deploy the
AppX plus the x64 dependency if required. Certificate/trust handling can differ
by console OS. If deployment rejects the certificate, publisher or minimum OS
version, retain the existing installation and report the exact error.

This is a sideload test certificate, NOT Microsoft Store signing. Compilation
and package-signature verification succeeded, but Xbox gameplay, saves and
upgrade behavior are not yet tested. Code mods remain disabled in this build.

The certificate is newly generated and valid for about one year; there is no
trusted timestamp. Keep this signed package and public certificate together.
The private key was not exported and is deleted from the temporary runner, so
future builds cannot reuse this exact key. A stable update-signing identity
requires a securely retained certificate/private key under your control.
'@ | Set-Content -LiteralPath (Join-Path $OutputDirectory 'README.txt') -Encoding utf8

    Get-ChildItem -LiteralPath $OutputDirectory -Recurse -File | ForEach-Object { Write-Host $_.FullName }
    Write-Host "Public certificate thumbprint: $($Certificate.Thumbprint)"
} finally {
    foreach ($TrustPath in @($RootTrustPath, $PeopleTrustPath)) {
        if ($TrustPath -and (Test-Path $TrustPath)) { Remove-Item -LiteralPath $TrustPath -Force }
    }
    if ($Certificate -and (Test-Path "Cert:\CurrentUser\My\$($Certificate.Thumbprint)")) {
        Remove-Item -LiteralPath "Cert:\CurrentUser\My\$($Certificate.Thumbprint)" -DeleteKey -Force
    }
}
