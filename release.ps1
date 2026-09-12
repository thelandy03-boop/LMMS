param (
    [Parameter(Mandatory=$true)]
    [string]$Version # Ej: v1.3.1
)

$ErrorActionPreference = "Stop"
$repoDir = "C:\Users\grezu\Downloads\Landy_Proyectos\Apps\LMMS\lmms-estudio"
$buildDir = "$repoDir\build"
$stagingDir = "$repoDir\staging_release"
$zipPath = "$repoDir\LMMS_Win64_$Version.zip"

Write-Host "=== INICIANDO PUBLICACIÓN AUTOMÁTICA DE RELEASE $Version ===" -ForegroundColor Cyan

if (Test-Path $stagingDir) { Remove-Item -Path $stagingDir -Recurse -Force }
if (Test-Path $zipPath) { Remove-Item -Path $zipPath -Force }

Write-Host "`n[1/5] Compilando LMMS en Release..." -ForegroundColor Yellow
Set-Location $buildDir
cmake --build . --config Release --parallel

Write-Host "`n[2/5] Desplegando archivos a Staging..." -ForegroundColor Yellow
cmake --install . --config Release --prefix $stagingDir

Copy-Item "$repoDir\cmake\nsis\project.ico" "$stagingDir\project.ico" -Force

Write-Host "`n[3/5] Comprimiendo paquete ZIP..." -ForegroundColor Yellow
Compress-Archive -Path "$stagingDir\*" -DestinationPath $zipPath -Force

Write-Host "`n[4/5] Creando Tag $Version en Git..." -ForegroundColor Yellow
Set-Location $repoDir
git tag -a $Version -m "Release $Version"
git push origin $Version

Write-Host "`n[5/5] Subiendo Release a GitHub..." -ForegroundColor Yellow
if (Get-Command gh -ErrorAction SilentlyContinue) {
    gh release create $Version $zipPath --title "LMMS $Version" --notes "Actualización automática de binarios para Windows x64."
    Write-Host "`n¡RELEASE $Version PUBLICADA CON ÉXITO EN GITHUB!" -ForegroundColor Green
} else {
    Write-Host "`n[AVISO] Se creó el Tag y el ZIP ($zipPath), pero GitHub CLI ('gh') no está instalado." -ForegroundColor Yellow
    Write-Host "Para subir automáticamente el ZIP a GitHub, instala 'gh' corriendo: winget install GitHub.cli" -ForegroundColor Cyan
}
