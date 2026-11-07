param(
    [ValidateSet('build', 'menuconfig', 'flash', 'monitor', 'size')]
    [string]$Action = 'build',
    [string]$Port
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
Push-Location $projectRoot
try {
    $idfArgs = @('-B', 'build-cyd', '-D', 'SDKCONFIG=sdkconfig.cyd',
                 '-D', 'SDKCONFIG_DEFAULTS=sdkconfig.defaults.cyd', '-D', 'IDF_TARGET=esp32')
    if ($Port) { $idfArgs += @('-p', $Port) }
    $idfArgs += $Action
    & idf.py @idfArgs
    if ($LASTEXITCODE -ne 0) { throw "idf.py failed with exit code $LASTEXITCODE" }
} finally {
    Pop-Location
}
