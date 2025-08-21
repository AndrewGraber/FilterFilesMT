param(
    [string]$ExePath = ".\Build\Release\FilterFilesMT.exe",
    [switch]$Verbose
)

# Ensure Unicode filenames display correctly
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$PSDefaultParameterValues['*:Encoding'] = 'utf8'

# Counters (script scope so functions can update them)
$script:PassCount = 0
$script:FailCount = 0

# Root for all test data
$TestRoot = Join-Path $PSScriptRoot "TestData"

# -----------------------------
# Helpers
# -----------------------------
function Write-Ok($m){ Write-Host "[PASS] $m" -ForegroundColor Green }
function Write-Bad($m){ Write-Host "[FAIL] $m" -ForegroundColor Red }
function Write-Info($m){ Write-Host "[INFO] $m" -ForegroundColor Cyan }
function Write-Diff($label,$list){
    Write-Host $label -ForegroundColor Yellow
    if (-not $list -or $list.Count -eq 0){ Write-Host "  (none)"; return }
    foreach($i in $list){ Write-Host "  $i" }
}

function New-CleanDir($path){
    if (Test-Path $path){ Remove-Item -Recurse -Force $path }
    New-Item -ItemType Directory -Force -Path $path | Out-Null
}

function New-TestDir($name, [string[]]$ignoreLines){
    $dir = Join-Path $TestRoot $name
    New-CleanDir $dir

    # Write .backupignore at the ROOT we're going to pass to the exe
    if ($ignoreLines){
        $file = Join-Path $dir ".backupignore"
        # comments/empty lines preserved as-is
        [IO.File]::WriteAllLines($file, $ignoreLines, [Text.Encoding]::UTF8)
    }
    return $dir
}

function Add-Dir($root, $rel){
    $p = Join-Path $root $rel
    New-Item -ItemType Directory -Force -Path $p | Out-Null
    return ([IO.Path]::GetFullPath($p))
}

function Add-File($root, $rel, [switch]$Hidden) {
    $p = Join-Path $root $rel
    New-Item -ItemType File -Force -Path $p | Out-Null
    if ($Hidden){ attrib +h $p | Out-Null }
    return ([string][System.IO.Path]::GetFullPath($p))
}

function Full($path){ return ([IO.Path]::GetFullPath($path)) }

function Run-Filter($root, [int]$threads=2){
    # Capture both stdout+stderr; we only compare stdout in success cases.
    $out = & $ExePath $root $threads 2>&1
    if ($null -eq $out){ $out = @() }
    # Trim blanks/whitespace
    $lines = @($out | ForEach-Object { $_.ToString().Trim() } | Where-Object { $_ -ne "" })
    return @{
        Output   = $lines
        ExitCode = $LASTEXITCODE
    }
}

function Compare-Set([string[]]$Actual, [string[]]$Expected){
    # Compare as sets (order-insensitive)
    $a = @($Actual   | Sort-Object -Unique)
    $e = @($Expected | Sort-Object -Unique)
    $diff = Compare-Object -ReferenceObject $e -DifferenceObject $a
    return @{ Equal = ($diff.Count -eq 0); Diff = $diff; A=$a; E=$e }
}

function Assert-AbsolutePaths([string[]]$Actual){
    $bad = @($Actual | Where-Object { $_ -notmatch '^[A-Za-z]:\\' -and $_ -notmatch '^\\\\\?\\' })
    return @{ Ok = ($bad.Count -eq 0); Bad = $bad }
}

function Record-Result($name, [bool]$ok, $detailsScriptBlock){
    if ($ok){
        Write-Ok $name
        $script:PassCount++
    } else {
        Write-Bad $name
        if ($detailsScriptBlock){ & $detailsScriptBlock }
        $script:FailCount++
    }
}

# Reset test root
New-CleanDir $TestRoot

# --------------------------------------------------------------------------------
# TESTS
# --------------------------------------------------------------------------------

# T0: Missing .backupignore -> program should FAIL (non-zero exit)
$T0 = New-TestDir "T0_MissingIgnore" @()  # Do NOT create .backupignore
$T0_file = Add-File $T0 "orphan.txt"
$res = Run-Filter $T0
Record-Result "T0 - Missing .backupignore fails" ($res.ExitCode -ne 0) {
    if ($Verbose){ Write-Info "ExitCode=$($res.ExitCode)"; Write-Diff "Program output (stderr+stdout):" $res.Output }
}

# T1: Empty directory (with comments-only .backupignore) -> only .backupignore printed
$T1 = New-TestDir "T1_Empty" @("# nothing to ignore", "", "   ")
$T1_ignore = Full (Join-Path $T1 ".backupignore")
$res = Run-Filter $T1
$cmp = Compare-Set $res.Output @($T1_ignore)
$abs = Assert-AbsolutePaths $res.Output
Record-Result "T1 - Empty dir prints only .backupignore (abs paths)" ($res.ExitCode -eq 0 -and $cmp.Equal -and $abs.Ok) {
    if ($Verbose){
        Write-Info "ExitCode=$($res.ExitCode)"
        Write-Diff "Expected:" $cmp.E
        Write-Diff "Actual:"   $cmp.A
        if (-not $abs.Ok){ Write-Diff "Non-absolute lines:" $abs.Bad }
        if (-not $cmp.Equal){ Write-Host ($cmp.Diff | Out-String) }
    }
}

# T2: Basic files + hidden; ignore file with comments only (no actual ignores)
$T2 = New-TestDir "T2_BasicHidden" @("# allow all")
$T2_ignore = Full (Join-Path $T2 ".backupignore")
$T2_f1 = Add-File $T2 "file1.txt"
$T2_f2 = Add-File $T2 "file2.c"
$T2_f3 = Add-File $T2 "file3.txt"
$T2_hidden = Add-File $T2 ".hidden" -Hidden
$res = Run-Filter $T2
$expected = @($T2_ignore,$T2_f1,$T2_f2,$T2_f3,$T2_hidden)
$cmp = Compare-Set $res.Output $expected
$abs = Assert-AbsolutePaths $res.Output
Record-Result "T2 - Basic + hidden (includes .backupignore)" ($res.ExitCode -eq 0 -and $cmp.Equal -and $abs.Ok) {
    if ($Verbose){
        Write-Info "ExitCode=$($res.ExitCode)"
        Write-Diff "Expected:" $cmp.E
        Write-Diff "Actual:"   $cmp.A
        if (-not $abs.Ok){ Write-Diff "Non-absolute lines:" $abs.Bad }
        if (-not $cmp.Equal){ Write-Host ($cmp.Diff | Out-String) }
    }
}

# T3: Nested directories; traversal works; no ignores
$T3 = New-TestDir "T3_Nested" @("# traverse all")
$T3_ignore = Full (Join-Path $T3 ".backupignore")
$T3_rootA = Add-File $T3 "rootA.txt"
$T3_sub   = Add-Dir  $T3 "sub"
$T3_subF  = Add-File $T3 "sub\nested.txt"
$res = Run-Filter $T3
$expected = @($T3_ignore,$T3_rootA,$T3_subF)
$cmp = Compare-Set $res.Output $expected
$abs = Assert-AbsolutePaths $res.Output
Record-Result "T3 - Recurses into subdirs" ($res.ExitCode -eq 0 -and $cmp.Equal -and $abs.Ok) {
    if ($Verbose){
        Write-Info "ExitCode=$($res.ExitCode)"
        Write-Diff "Expected:" $cmp.E
        Write-Diff "Actual:"   $cmp.A
        if (-not $abs.Ok){ Write-Diff "Non-absolute lines:" $abs.Bad }
        if (-not $cmp.Equal){ Write-Host ($cmp.Diff | Out-String) }
    }
}

# T4: Moderately long filename (avoid MAX_PATH pitfalls); no ignores
$T4 = New-TestDir "T4_LongName" @("# allow all")
$T4_ignore = Full (Join-Path $T4 ".backupignore")
$longBase = ("a" * 120) + ".txt"    # safe length
$T4_long  = Add-File $T4 $longBase
$res = Run-Filter $T4
$expected = @($T4_ignore,$T4_long)
$cmp = Compare-Set $res.Output $expected
$abs = Assert-AbsolutePaths $res.Output
Record-Result "T4 - Long-ish filename handled" ($res.ExitCode -eq 0 -and $cmp.Equal -and $abs.Ok) {
    if ($Verbose){
        Write-Info "ExitCode=$($res.ExitCode)"
        Write-Diff "Expected:" $cmp.E
        Write-Diff "Actual:"   $cmp.A
        if (-not $abs.Ok){ Write-Diff "Non-absolute lines:" $abs.Bad }
        if (-not $cmp.Equal){ Write-Host ($cmp.Diff | Out-String) }
    }
}

# T5: Special/Unicode filenames; no ignores
$T5 = New-TestDir "T5_SpecialUnicode" @("# allow all")
$T5_ignore = Full (Join-Path $T5 ".backupignore")
$T5_s1 = Add-File $T5 "space file.txt"
$T5_s2 = Add-File $T5 "üñîçødé.txt"
$T5_s3 = Add-File $T5 "symbols_#@!.txt"
$res = Run-Filter $T5
$expected = @($T5_ignore,$T5_s1,$T5_s2,$T5_s3)
$cmp = Compare-Set $res.Output $expected
$abs = Assert-AbsolutePaths $res.Output
Record-Result "T5 - Special/Unicode filenames" ($res.ExitCode -eq 0 -and $cmp.Equal -and $abs.Ok) {
    if ($Verbose){
        Write-Info "ExitCode=$($res.ExitCode)"
        Write-Diff "Expected:" $cmp.E
        Write-Diff "Actual:"   $cmp.A
        if (-not $abs.Ok){ Write-Diff "Non-absolute lines:" $abs.Bad }
        if (-not $cmp.Equal){ Write-Host ($cmp.Diff | Out-String) }
    }
}

# T6: Multiple threads -> same set of outputs
$T6 = New-TestDir "T6_MultiThread" @("# allow all")
$T6_ignore = Full (Join-Path $T6 ".backupignore")
$T6_a = Add-File $T6 "a.txt"
$T6_b = Add-File $T6 "b.c"
$T6_sub = Add-Dir $T6 "sub"
$T6_s1 = Add-File $T6 "sub\c.log"
$res1 = Run-Filter $T6 1
$res4 = Run-Filter $T6 4
$exp = @($T6_ignore,$T6_a,$T6_b,$T6_s1)
$cmp1 = Compare-Set $res1.Output $exp
$cmp4 = Compare-Set $res4.Output $exp
$abs1 = Assert-AbsolutePaths $res1.Output
$abs4 = Assert-AbsolutePaths $res4.Output
Record-Result "T6 - Deterministic across threads" ($res1.ExitCode -eq 0 -and $res4.ExitCode -eq 0 -and $cmp1.Equal -and $cmp4.Equal -and $abs1.Ok -and $abs4.Ok) {
    if ($Verbose){
        Write-Info "ExitCode(1 thread)=$($res1.ExitCode); ExitCode(4 threads)=$($res4.ExitCode)"
        Write-Diff "Expected:" $exp
        Write-Diff "Actual (1t):" $res1.Output
        Write-Diff "Actual (4t):" $res4.Output
        if (-not $cmp1.Equal){ Write-Host "Diff(1t):`n$($cmp1.Diff | Out-String)" }
        if (-not $cmp4.Equal){ Write-Host "Diff(4t):`n$($cmp4.Diff | Out-String)" }
    }
}

# T7: Pattern matching via .backupignore (ignore *.doc and b.c)
$T7 = New-TestDir "T7_Patterns" @(
    "# ignore docs and one specific file",
    "*.doc",
    "b.c",
    ""
)
$T7_ignore = Full (Join-Path $T7 ".backupignore")
$T7_a = Add-File $T7 "a.txt"
$T7_b = Add-File $T7 "b.c"
$T7_c = Add-File $T7 "c.log"
$T7_d = Add-File $T7 "d.txt"
$T7_e = Add-File $T7 "e.doc"
$res = Run-Filter $T7
# Expect everything except e.doc and b.c (but include .backupignore)
$expected = @($T7_ignore,$T7_a,$T7_c,$T7_d)
$cmp = Compare-Set $res.Output $expected
$abs = Assert-AbsolutePaths $res.Output
Record-Result "T7 - .backupignore patterns (*.doc, b.c)" ($res.ExitCode -eq 0 -and $cmp.Equal -and $abs.Ok) {
    if ($Verbose){
        Write-Info "ExitCode=$($res.ExitCode)"
        Write-Diff "Expected:" $cmp.E
        Write-Diff "Actual:"   $cmp.A
        if (-not $abs.Ok){ Write-Diff "Non-absolute lines:" $abs.Bad }
        if (-not $cmp.Equal){ Write-Host ($cmp.Diff | Out-String) }
    }
}

# T8: Recursive ignore (*.txt everywhere)
$T8 = New-TestDir "T8_Recursive" @(
    "# ignore all .txt recursively",
    "*.txt"
)
$T8_ignore = Full (Join-Path $T8 ".backupignore")
$T8_rootTxt = Add-File $T8 "root.txt"
$T8_rootMd  = Add-File $T8 "root.md"
$T8_lvl1    = Add-Dir  $T8 "level1"
$T8_l1_txt  = Add-File $T8 "level1\file1.txt"
$T8_lvl2    = Add-Dir  $T8 "level1\level2"
$T8_l2_txt  = Add-File $T8 "level1\level2\another.txt"
$T8_l2_log  = Add-File $T8 "level1\level2\keep.log"
$res = Run-Filter $T8
# Expect everything except *.txt; include .backupignore
$expected = @($T8_ignore,$T8_rootMd,$T8_l2_log)
$cmp = Compare-Set $res.Output $expected
$abs = Assert-AbsolutePaths $res.Output
Record-Result "T8 - Recursive ignore (*.txt)" ($res.ExitCode -eq 0 -and $cmp.Equal -and $abs.Ok) {
    if ($Verbose){
        Write-Info "ExitCode=$($res.ExitCode)"
        Write-Diff "Expected:" $cmp.E
        Write-Diff "Actual:"   $cmp.A
        if (-not $abs.Ok){ Write-Diff "Non-absolute lines:" $abs.Bad }
        if (-not $cmp.Equal){ Write-Host ($cmp.Diff | Out-String) }
    }
}

# --------------------------------------------------------------------------------
# Summary
# --------------------------------------------------------------------------------
Write-Host "--------------------------------------------------"
Write-Host "Test summary: $($script:PassCount) passed, $($script:FailCount) failed of $($script:PassCount + $script:FailCount)"
Write-Host "--------------------------------------------------"

if ($Verbose) {
    Write-Info "Test data left at: $TestRoot"
} else {
    # Clean up test data when not verbose
    try { Remove-Item -Recurse -Force $TestRoot | Out-Null } catch {}
}

# Non-zero exit if any test failed (handy for CI)
if ($script:FailCount -gt 0) { exit 1 } else { exit 0 }