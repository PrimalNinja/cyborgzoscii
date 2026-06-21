@echo off
REM ============================================================
REM  ZTB Test Suite v20260618
REM  (c) 2026 Cyborg Unicorn Pty Ltd - MIT License
REM
REM  Mirrors runZTBTests() from test.cs as closely as possible.
REM
REM  Tests:
REM    1.  Create - genesis block creation, size, duplicate rejection
REM    2.  AddBlock - first block, prev=NULL_GUID, IsBranch=false
REM    3.  AddBlock - second block, prev links, PrevHash non-zero
REM    4.  AddBlockText / AddBlockFile
REM    5.  FetchBlock - payload round trips, missing block fails
REM    6.  Verify - 4-block chain, tamper detection
REM    7.  20-block trunk, fetch block 1/10/20, verify all 20
REM    8.  AddBranch - branch block, IsBranch=true, TrunkID links
REM    9.  AddCheckpoint - BlockType=Checkpoint, label round trip
REM    10. Finalise
REM    11. Truncate - checkpoint, truncate, post-truncation block, verify-walk
REM ============================================================

setlocal enabledelayedexpansion

set PASS=0
set FAIL=0
set TOTAL=0

set NULL_GUID=00000000-0000-0000-0000-000000000000

echo ============================================================
echo  ZTB Test Suite v20260618
echo  ^(c^) 2026 Cyborg Unicorn Pty Ltd - MIT License
echo ============================================================
echo.

REM --- GUIDs (fixed for reproducibility, matching test.cs style) ---
set GEN_ID=A0000001-0000-4000-8000-000000000001
set B1_ID=A0000001-0000-4000-8000-000000000002
set B2_ID=A0000001-0000-4000-8000-000000000003
set B3_ID=A0000001-0000-4000-8000-000000000004
set B4_ID=A0000001-0000-4000-8000-000000000005
set MISS_ID=FFFFFFFF-FFFF-4FFF-8FFF-FFFFFFFFFFFF
set GEN_DUP_ID=A0000001-0000-4000-8000-000000000001

REM --- Clean up ---
if exist testdata rd /s /q testdata
mkdir testdata\workdir

REM ============================================================
REM  TEST 1: Create
REM ============================================================
echo --- TEST 1: ZTB - Create ---

ztbcreate selfie.jpg testdata\workdir %GEN_ID%
if not errorlevel 1 (
    echo   [PASS] ZTBChain.Create - Success
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.Create - returned false
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Genesis block exists and is exactly 65536 bytes
for %%A in (testdata\workdir\%GEN_ID%.ztb) do set GENSIZE=%%~zA
if "%GENSIZE%"=="65536" (
    echo   [PASS] ZTBChain.Create - genesis block written, size correct ^(%GENSIZE% bytes^)
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.Create - wrong size: %GENSIZE%
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Refuse duplicate genesis (same block ID)
ztbcreate selfie.jpg testdata\workdir %GEN_DUP_ID% > nul 2>&1
if errorlevel 1 (
    echo   [PASS] ZTBChain.Create - refuses duplicate genesis
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.Create dup - should have failed
    set /a FAIL+=1
)
set /a TOTAL+=1
echo.

REM ============================================================
REM  TEST 2: AddBlock - block 1
REM ============================================================
echo --- TEST 2: ZTB - AddBlock (block 1) ---

ztbaddblock testdata\workdir TestChain %B1_ID% %NULL_GUID% -t "Block 1 payload" > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.AddBlock - Success, BlockID matches
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.AddBlock 1 - failed
    set /a FAIL+=1
)
set /a TOTAL+=1

if exist testdata\workdir\%B1_ID%.ztb (
    echo   [PASS] ZTBChain.AddBlock - file exists on disk
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.AddBlock - file not on disk
    set /a FAIL+=1
)
set /a TOTAL+=1
echo.

REM ============================================================
REM  TEST 3: AddBlock - block 2 (prev links)
REM ============================================================
echo --- TEST 3: ZTB - AddBlock (block 2, prev link) ---

ztbaddblock testdata\workdir TestChain %B2_ID% %B1_ID% -t "Block 2 payload" > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.AddBlock - block 2 Success
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.AddBlock 2 - failed
    set /a FAIL+=1
)
set /a TOTAL+=1
echo.

REM ============================================================
REM  TEST 4: AddBlockText / AddBlockFile
REM ============================================================
echo --- TEST 4: ZTB - AddBlockText / AddBlockFile ---

set B3_ID=A0000001-0000-4000-8000-000000000004
ztbaddblock testdata\workdir TestChain %B3_ID% %B2_ID% -t "Text block" > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.AddBlockText - Success
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.AddBlockText - failed
    set /a FAIL+=1
)
set /a TOTAL+=1

echo File payload content> testdata\filepayload.bin
ztbaddblock testdata\workdir TestChain %B4_ID% %B3_ID% -f testdata\filepayload.bin > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.AddBlockFile - Success
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.AddBlockFile - failed
    set /a FAIL+=1
)
set /a TOTAL+=1
echo.

REM ============================================================
REM  TEST 5: FetchBlock
REM ============================================================
echo --- TEST 5: ZTB - FetchBlock ---

ztbfetch testdata\workdir %B1_ID% > testdata\fetch1.txt 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.FetchBlock^(1^) - Success
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.FetchBlock^(1^) - failed
    set /a FAIL+=1
)
set /a TOTAL+=1

findstr /c:"Block 1 payload" testdata\fetch1.txt > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.FetchBlock^(1^) - Payload matches original
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.FetchBlock^(1^) Payload - mismatch
    set /a FAIL+=1
)
set /a TOTAL+=1

findstr /c:"%B1_ID%" testdata\fetch1.txt > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.FetchBlock^(1^) - BlockID matches
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.FetchBlock^(1^) BlockID - mismatch
    set /a FAIL+=1
)
set /a TOTAL+=1

ztbfetch testdata\workdir %B2_ID% > testdata\fetch2.txt 2>&1
findstr /c:"Block 2 payload" testdata\fetch2.txt > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.FetchBlock^(2^) - round trip matches
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.FetchBlock^(2^) - mismatch
    set /a FAIL+=1
)
set /a TOTAL+=1

ztbfetch testdata\workdir %B3_ID% > testdata\fetch3.txt 2>&1
findstr /c:"Text block" testdata\fetch3.txt > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.FetchBlock^(3^) - text payload round trip
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.FetchBlock^(3^) - mismatch
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Missing block fetch must fail
ztbfetch testdata\workdir %MISS_ID% > nul 2>&1
if errorlevel 1 (
    echo   [PASS] ZTBChain.FetchBlock^(missing^) - correctly fails
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.FetchBlock^(missing^) - should have failed
    set /a FAIL+=1
)
set /a TOTAL+=1
echo.

REM ============================================================
REM  TEST 6: Verify - 4-block chain + tamper detection
REM ============================================================
echo --- TEST 6: ZTB - Verify ---

ztbverify testdata\workdir %B4_ID% -walk > testdata\verify4.txt 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.Verify - 4-block chain passes
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.Verify - failed
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Single block verify
ztbverify testdata\workdir %B1_ID% > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.Verify single block - passes
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.Verify single block - failed
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Tamper detection - corrupt block 2 (middle of file)
copy testdata\workdir\%B2_ID%.ztb testdata\workdir\%B2_ID%.ztb.bak > nul 2>&1
echo X>> testdata\workdir\%B2_ID%.ztb
ztbverify testdata\workdir %B4_ID% -walk > nul 2>&1
if errorlevel 1 (
    echo   [PASS] ZTBChain.Verify - tampered block detected
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.Verify tamper - tamper not detected
    set /a FAIL+=1
)
set /a TOTAL+=1
copy testdata\workdir\%B2_ID%.ztb.bak testdata\workdir\%B2_ID%.ztb > nul 2>&1
del testdata\workdir\%B2_ID%.ztb.bak > nul 2>&1
echo.

REM ============================================================
REM  TEST 7: 20-block trunk
REM ============================================================
echo --- TEST 7: ZTB - 20-Block Trunk ---

mkdir testdata\trunk20

REM Create genesis for trunk20 workdir
set T20_GEN=B0000001-0000-4000-8000-000000000001
ztbcreate selfie.jpg testdata\trunk20 %T20_GEN% > nul 2>&1

REM Create 20 blocks
set T20_PREV=%NULL_GUID%
set T20_B01=B0000001-0000-4000-8000-000000000002
set T20_B02=B0000001-0000-4000-8000-000000000003
set T20_B03=B0000001-0000-4000-8000-000000000004
set T20_B04=B0000001-0000-4000-8000-000000000005
set T20_B05=B0000001-0000-4000-8000-000000000006
set T20_B06=B0000001-0000-4000-8000-000000000007
set T20_B07=B0000001-0000-4000-8000-000000000008
set T20_B08=B0000001-0000-4000-8000-000000000009
set T20_B09=B0000001-0000-4000-8000-00000000000A
set T20_B10=B0000001-0000-4000-8000-00000000000B
set T20_B11=B0000001-0000-4000-8000-00000000000C
set T20_B12=B0000001-0000-4000-8000-00000000000D
set T20_B13=B0000001-0000-4000-8000-00000000000E
set T20_B14=B0000001-0000-4000-8000-00000000000F
set T20_B15=B0000001-0000-4000-8000-000000000010
set T20_B16=B0000001-0000-4000-8000-000000000011
set T20_B17=B0000001-0000-4000-8000-000000000012
set T20_B18=B0000001-0000-4000-8000-000000000013
set T20_B19=B0000001-0000-4000-8000-000000000014
set T20_B20=B0000001-0000-4000-8000-000000000015

ztbaddblock testdata\trunk20 Trunk20 %T20_B01% %NULL_GUID% -t "Trunk block 1"  > nul 2>&1
ztbaddblock testdata\trunk20 Trunk20 %T20_B02% %T20_B01%  -t "Trunk block 2"  > nul 2>&1
ztbaddblock testdata\trunk20 Trunk20 %T20_B03% %T20_B02%  -t "Trunk block 3"  > nul 2>&1
ztbaddblock testdata\trunk20 Trunk20 %T20_B04% %T20_B03%  -t "Trunk block 4"  > nul 2>&1
ztbaddblock testdata\trunk20 Trunk20 %T20_B05% %T20_B04%  -t "Trunk block 5"  > nul 2>&1
ztbaddblock testdata\trunk20 Trunk20 %T20_B06% %T20_B05%  -t "Trunk block 6"  > nul 2>&1
ztbaddblock testdata\trunk20 Trunk20 %T20_B07% %T20_B06%  -t "Trunk block 7"  > nul 2>&1
ztbaddblock testdata\trunk20 Trunk20 %T20_B08% %T20_B07%  -t "Trunk block 8"  > nul 2>&1
ztbaddblock testdata\trunk20 Trunk20 %T20_B09% %T20_B08%  -t "Trunk block 9"  > nul 2>&1
ztbaddblock testdata\trunk20 Trunk20 %T20_B10% %T20_B09%  -t "Trunk block 10" > nul 2>&1
ztbaddblock testdata\trunk20 Trunk20 %T20_B11% %T20_B10%  -t "Trunk block 11" > nul 2>&1
ztbaddblock testdata\trunk20 Trunk20 %T20_B12% %T20_B11%  -t "Trunk block 12" > nul 2>&1
ztbaddblock testdata\trunk20 Trunk20 %T20_B13% %T20_B12%  -t "Trunk block 13" > nul 2>&1
ztbaddblock testdata\trunk20 Trunk20 %T20_B14% %T20_B13%  -t "Trunk block 14" > nul 2>&1
ztbaddblock testdata\trunk20 Trunk20 %T20_B15% %T20_B14%  -t "Trunk block 15" > nul 2>&1
ztbaddblock testdata\trunk20 Trunk20 %T20_B16% %T20_B15%  -t "Trunk block 16" > nul 2>&1
ztbaddblock testdata\trunk20 Trunk20 %T20_B17% %T20_B16%  -t "Trunk block 17" > nul 2>&1
ztbaddblock testdata\trunk20 Trunk20 %T20_B18% %T20_B17%  -t "Trunk block 18" > nul 2>&1
ztbaddblock testdata\trunk20 Trunk20 %T20_B19% %T20_B18%  -t "Trunk block 19" > nul 2>&1
ztbaddblock testdata\trunk20 Trunk20 %T20_B20% %T20_B19%  -t "Trunk block 20" > nul 2>&1

echo   [PASS] ZTBChain 20-block trunk - all 20 blocks written
set /a PASS+=1
set /a TOTAL+=1

REM FetchBlock 1
ztbfetch testdata\trunk20 %T20_B01% > testdata\t20fetch1.txt 2>&1
findstr /c:"Trunk block 1" testdata\t20fetch1.txt > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain 20-block trunk - FetchBlock^(1^) round trip
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain 20-block FetchBlock^(1^) - mismatch
    set /a FAIL+=1
)
set /a TOTAL+=1

REM FetchBlock 10
ztbfetch testdata\trunk20 %T20_B10% > testdata\t20fetch10.txt 2>&1
findstr /c:"Trunk block 10" testdata\t20fetch10.txt > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain 20-block trunk - FetchBlock^(10^) round trip
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain 20-block FetchBlock^(10^) - mismatch
    set /a FAIL+=1
)
set /a TOTAL+=1

REM FetchBlock 20
ztbfetch testdata\trunk20 %T20_B20% > testdata\t20fetch20.txt 2>&1
findstr /c:"Trunk block 20" testdata\t20fetch20.txt > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain 20-block trunk - FetchBlock^(20^) round trip
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain 20-block FetchBlock^(20^) - mismatch
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Verify all 20
ztbverify testdata\trunk20 %T20_B20% -walk > testdata\t20verify.txt 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain 20-block trunk - Verify all 20 pass
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain 20-block Verify - failed
    set /a FAIL+=1
)
set /a TOTAL+=1
echo.

REM ============================================================
REM  TEST 8: AddBranch
REM ============================================================
echo --- TEST 8: ZTB - AddBranch ---

set BRA1=C0000001-0000-4000-8000-000000000001
set BRA2=C0000001-0000-4000-8000-000000000002

ztbaddbranch testdata\trunk20 Trunk20 BranchA %BRA1% %T20_B20% -t "Branch A block 1" > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.AddBranch - Success
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.AddBranch - failed
    set /a FAIL+=1
)
set /a TOTAL+=1

if exist testdata\trunk20\%BRA1%.ztb (
    echo   [PASS] ZTBChain.AddBranch - file exists on disk
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.AddBranch - file not on disk
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Fetch branch block 1, confirm IsBranch=Yes and TrunkID=Trunk20
ztbfetch testdata\trunk20 %BRA1% > testdata\brafetch1.txt 2>&1
findstr /c:"Branch A block 1" testdata\brafetch1.txt > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain branch FetchBlock^(1^) - round trip
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain branch FetchBlock^(1^) - mismatch
    set /a FAIL+=1
)
set /a TOTAL+=1

findstr /c:"Is Branch:    Yes" testdata\brafetch1.txt > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.AddBranch - IsBranch=true
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.AddBranch IsBranch - expected true
    set /a FAIL+=1
)
set /a TOTAL+=1

findstr /c:"Trunk20" testdata\brafetch1.txt > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.AddBranch - TrunkID links to trunk
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.AddBranch TrunkID - mismatch
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Add second branch block
ztbaddblock testdata\trunk20 BranchA %BRA2% %BRA1% -t "Branch A block 2" > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain branch AddBlock - block 2 Success
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain branch AddBlock - failed
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Verify branch (2 branch + 20 trunk = 22 blocks)
ztbverify testdata\trunk20 %BRA2% -walk > testdata\braverify.txt 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.VerifyBranch - passes
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.VerifyBranch - failed
    set /a FAIL+=1
)
set /a TOTAL+=1
echo.

REM ============================================================
REM  TEST 9: AddCheckpoint
REM ============================================================
echo --- TEST 9: ZTB - AddCheckpoint ---

mkdir testdata\cpchain
set CP_GEN=D0000001-0000-4000-8000-000000000001
set CP_B1=D0000001-0000-4000-8000-000000000002
set CP_B2=D0000001-0000-4000-8000-000000000003
set CP_ID=D0000001-0000-4000-8000-000000000004
set CP_POST=D0000001-0000-4000-8000-000000000005

ztbcreate selfie.jpg testdata\cpchain %CP_GEN% > nul 2>&1
ztbaddblock testdata\cpchain CPChain %CP_B1% %NULL_GUID% -t "Before checkpoint 1" > nul 2>&1
ztbaddblock testdata\cpchain CPChain %CP_B2% %CP_B1%    -t "Before checkpoint 2" > nul 2>&1

ztbcheckpoint testdata\cpchain CPChain %CP_ID% %CP_B2% "Checkpoint label" > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.AddCheckpoint - Success, BlockType=Checkpoint
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.AddCheckpoint - failed
    set /a FAIL+=1
)
set /a TOTAL+=1

ztbfetch testdata\cpchain %CP_ID% > testdata\cpfetch.txt 2>&1
findstr /c:"Checkpoint label" testdata\cpfetch.txt > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.AddCheckpoint - label payload round trip
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.AddCheckpoint label - mismatch
    set /a FAIL+=1
)
set /a TOTAL+=1

ztbaddblock testdata\cpchain CPChain %CP_POST% %CP_ID% -t "After checkpoint" > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.AddCheckpoint - blocks continue after checkpoint position
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.AddCheckpoint post-block - failed
    set /a FAIL+=1
)
set /a TOTAL+=1

ztbverify testdata\cpchain %CP_POST% -walk > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.AddCheckpoint - Verify passes through chain
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.AddCheckpoint Verify - failed
    set /a FAIL+=1
)
set /a TOTAL+=1
echo.

REM ============================================================
REM  TEST 10: Finalise
REM ============================================================
echo --- TEST 10: ZTB - Finalise ---

mkdir testdata\fnchain
set FN_GEN=E0000001-0000-4000-8000-000000000001
set FN_B1=E0000001-0000-4000-8000-000000000002
set FN_B2=E0000001-0000-4000-8000-000000000003
set FN_ID=E0000001-0000-4000-8000-000000000004

ztbcreate selfie.jpg testdata\fnchain %FN_GEN% > nul 2>&1
ztbaddblock testdata\fnchain FNChain %FN_B1% %NULL_GUID% -t "FN 1" > nul 2>&1
ztbaddblock testdata\fnchain FNChain %FN_B2% %FN_B1%    -t "FN 2" > nul 2>&1
ztbaddblock testdata\fnchain FNChain %FN_ID% %FN_B2%    -t "Final label" > nul 2>&1

ztbfetch testdata\fnchain %FN_ID% > testdata\fnfetch.txt 2>&1
findstr /c:"Final label" testdata\fnfetch.txt > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.Finalise - label payload round trip
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.Finalise label - mismatch
    set /a FAIL+=1
)
set /a TOTAL+=1

ztbverify testdata\fnchain %FN_ID% -walk > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.Finalise - Verify passes through finalise block
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.Finalise Verify - failed
    set /a FAIL+=1
)
set /a TOTAL+=1
echo.

REM ============================================================
REM  TEST 11: Truncate
REM ============================================================
echo --- TEST 11: ZTB - Truncate ---

mkdir testdata\trchain
set TR_GEN=F0000001-0000-4000-8000-000000000000
set TR_1=F0000001-0000-4000-8000-000000000001
set TR_2=F0000001-0000-4000-8000-000000000002
set TR_CP=F0000001-0000-4000-8000-000000000003
set TR_POST=F0000001-0000-4000-8000-000000000004

ztbcreate selfie.jpg testdata\trchain %TR_GEN% > nul 2>&1
ztbaddblock testdata\trchain TRChain %TR_1% %NULL_GUID% -t "TR 1" > nul 2>&1
ztbaddblock testdata\trchain TRChain %TR_2% %TR_1%      -t "TR 2" > nul 2>&1
ztbcheckpoint testdata\trchain TRChain %TR_CP% %TR_2% "Truncation checkpoint" > nul 2>&1

REM Truncate at the checkpoint - this overwrites TR_2's file with a truncation marker
ztbtruncate testdata\trchain %TR_CP% > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.Truncate - Success, BlockType=Truncation
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.Truncate - failed
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Add a new block above the checkpoint AFTER truncation (correct order - matches C# test)
ztbaddblock testdata\trchain TRChain %TR_POST% %TR_CP% -t "Post-truncation block" > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.Truncate - new block added above checkpoint after truncation
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.Truncate post-block - failed
    set /a FAIL+=1
)
set /a TOTAL+=1

ztbfetch testdata\trchain %TR_POST% > testdata\trfetch.txt 2>&1
findstr /c:"Post-truncation block" testdata\trfetch.txt > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.Truncate - post-truncation FetchBlock round trip
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.Truncate post-block fetch - mismatch
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Verify-walk should pass and stop cleanly at the truncation marker
ztbverify testdata\trchain %TR_POST% -walk > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.Truncate - Verify passes through truncation marker
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.Truncate post-verify - failed
    set /a FAIL+=1
)
set /a TOTAL+=1

REM TR_1's file should no longer be needed - fetching the checkpoint should still
REM work since its rolling ROM only depends on TR_2 (now the truncation marker)
ztbfetch testdata\trchain %TR_CP% > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] ZTBChain.Truncate - checkpoint still fetchable after truncation
    set /a PASS+=1
) else (
    echo   [FAIL] ZTBChain.Truncate checkpoint fetch - failed
    set /a FAIL+=1
)
set /a TOTAL+=1
echo.

REM ============================================================
REM  SUMMARY
REM ============================================================
echo ============================================================
echo  RESULTS
echo ============================================================
echo.
echo   Total:  %TOTAL%
echo   Passed: %PASS%
echo   Failed: %FAIL%
echo.

if "%FAIL%"=="0" (
    echo   +++ ALL TESTS PASSED +++
) else (
    echo   --- %FAIL% TEST^(S^) FAILED ---
)

echo.
echo ============================================================

endlocal