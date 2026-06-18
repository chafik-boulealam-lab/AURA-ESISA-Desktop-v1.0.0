$src = "C:\msys64\mingw64\bin"
$dest = Join-Path $PSScriptRoot "bin\bin"
if (-not (Test-Path $dest)) { $dest = Join-Path $PSScriptRoot "bin" }

$dlls = @(
    "libatk-1.0-0.dll","libcairo-2.dll","libcairo-gobject-2.dll","libcjson-1.dll","libcurl-4.dll",
    "libexpat-1.dll","libffi-8.dll","libfontconfig-1.dll","libfreetype-6.dll","libgdk-3-0.dll",
    "libgdk_pixbuf-2.0-0.dll","libgio-2.0-0.dll","libglib-2.0-0.dll","libgmodule-2.0-0.dll",
    "libgobject-2.0-0.dll","libgraphite2.dll","libgtk-3-0.dll","libharfbuzz-0.dll","libiconv-2.dll",
    "libintl-8.dll","libpango-1.0-0.dll","libpangocairo-1.0-0.dll","libpangowin32-1.0-0.dll",
    "libpcre2-8-0.dll","libpixman-1-0.dll","libpng16-16.dll","libsqlite3-0.dll","libwinpthread-1.dll",
    "zlib1.dll","libgcc_s_seh-1.dll","libstdc++-6.dll","libbrotlidec.dll","libbrotlicommon.dll",
    "libidn2-0.dll","libnghttp2-14.dll","libnghttp3-9.dll","libngtcp2-16.dll","libngtcp2_crypto_ossl-0.dll",
    "libpsl-5.dll","libssh2-1.dll","libepoxy-0.dll","libfribidi-0.dll","libzstd.dll","libcrypto-3-x64.dll",
    "libssl-3-x64.dll","libunistring-5.dll","libjpeg-8.dll","libtiff-6.dll","libthai-0.dll",
    "libpangoft2-1.0-0.dll","libbz2-1.dll","libjbig-0.dll","libLerc.dll","libdeflate.dll","liblzma-5.dll",
    "libwebp-7.dll","libdatrie-1.dll","libsharpyuv-0.dll"
)

$copied = 0
foreach ($d in $dlls) {
    $p = Join-Path $src $d
    if (Test-Path $p) {
        Copy-Item $p $dest -Force
        $copied++
    }
}
Write-Host "DLLs copies: $copied -> $dest"