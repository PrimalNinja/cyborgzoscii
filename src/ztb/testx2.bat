@echo off
REM ============================================================
REM  ZTB Test Suite v20260420 (X2 Mode)
REM  (c) 2026 Cyborg Unicorn Pty Ltd - MIT License
REM
REM  Comprehensive test of all ZTB tools (X2 Mode):
REM    Test 1: Genesis ROM creation
REM    Test 2: 100-block trunk chain
REM    Test 3: Branch creation and population
REM    Test 4: Fetch and verify individual blocks
REM    Test 5: Full chain verification (trunk + branches)
REM    Test 6: Checkpoint and archive split
REM    Test 7: Post-checkpoint chain continuation
REM    Test 8: Independent archive verification
REM    Test 9: File payload test
REM    Test 10: Tamper detection test
REM    Test 11: Mixed mode chain
REM    Test 12: Edge cases
REM    Test 13: Supplied block ID (-i parameter) in X2 and normal mode
REM ============================================================

setlocal enabledelayedexpansion

set PASS=0
set FAIL=0
set TOTAL=0

echo ============================================================
echo  ZTB Test Suite v20260420 (X2 Mode)
echo  ^(c^) 2026 Cyborg Unicorn Pty Ltd - MIT License
echo ============================================================
echo.

REM --- Clean up any previous test data ---
if exist testdata_x2 rd /s /q testdata_x2
mkdir testdata_x2
cd testdata_x2

REM ============================================================
REM  TEST 1: Genesis ROM Creation
REM ============================================================
echo --- TEST 1: Genesis ROM Creation ---

..\ztbcreate ..\selfie.jpg genesis.rom > nul 2>&1

if exist genesis.rom (
    echo   [PASS] Genesis ROM created
    set /a PASS+=1
) else (
    echo   [FAIL] Genesis ROM not created
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Check file size (should be 65536 bytes)
for %%A in (genesis.rom) do set ROMSIZE=%%~zA
if "%ROMSIZE%"=="65536" (
    echo   [PASS] Genesis ROM size correct: %ROMSIZE% bytes
    set /a PASS+=1
) else (
    echo   [FAIL] Genesis ROM size wrong: %ROMSIZE% bytes ^(expected 65536^)
    set /a FAIL+=1
)
set /a TOTAL+=1
echo.

REM ============================================================
REM  TEST 2: 100-Block Trunk Chain
REM ============================================================
echo --- TEST 2: 100-Block Trunk Chain ---

set TRUNK_ERRORS=0
for /L %%N in (1,1,100) do (
    ..\ztbaddblock genesis.rom MainTrunk -t "Trunk block %%N of 100 - timestamp test data padding to ensure reasonable payload size" -x2 > nul 2>&1
    if errorlevel 1 (
        set /a TRUNK_ERRORS+=1
    )
)

REM Count .ztb files for MainTrunk
set TRUNK_COUNT=0
for %%F in (MainTrunk_*.ztb) do set /a TRUNK_COUNT+=1

if "%TRUNK_COUNT%"=="100" (
    echo   [PASS] 100 trunk blocks created
    set /a PASS+=1
) else (
    echo   [FAIL] Expected 100 trunk blocks, found %TRUNK_COUNT%
    set /a FAIL+=1
)
set /a TOTAL+=1

if "%TRUNK_ERRORS%"=="0" (
    echo   [PASS] All 100 blocks created without errors
    set /a PASS+=1
) else (
    echo   [FAIL] %TRUNK_ERRORS% blocks had errors
    set /a FAIL+=1
)
set /a TOTAL+=1
echo.

REM ============================================================
REM  TEST 3: Branch Creation and Population
REM ============================================================
echo --- TEST 3: Branch Creation and Population ---

REM Create 3 branches
..\ztbaddbranch genesis.rom MainTrunk Sales -t "Sales department branch" -x2 > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Branch 'Sales' created
    set /a PASS+=1
) else (
    echo   [FAIL] Branch 'Sales' creation failed
    set /a FAIL+=1
)
set /a TOTAL+=1

..\ztbaddbranch genesis.rom MainTrunk Engineering -t "Engineering department branch" -x2 > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Branch 'Engineering' created
    set /a PASS+=1
) else (
    echo   [FAIL] Branch 'Engineering' creation failed
    set /a FAIL+=1
)
set /a TOTAL+=1

..\ztbaddbranch genesis.rom MainTrunk Legal -t "Legal department branch" -x2 > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Branch 'Legal' created
    set /a PASS+=1
) else (
    echo   [FAIL] Branch 'Legal' creation failed
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Add 20 blocks to Sales
set SALES_ERRORS=0
for /L %%N in (1,1,20) do (
    ..\ztbaddblock genesis.rom Sales -t "Sales invoice %%N - customer order processing record" -x2 > nul 2>&1
    if errorlevel 1 set /a SALES_ERRORS+=1
)

set SALES_COUNT=0
for %%F in (Sales_*.ztb) do set /a SALES_COUNT+=1

if "%SALES_COUNT%"=="21" (
    echo   [PASS] Sales branch: 21 blocks ^(1 branch + 20 added^)
    set /a PASS+=1
) else (
    echo   [FAIL] Sales branch: expected 21 blocks, found %SALES_COUNT%
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Add 15 blocks to Engineering
set ENG_ERRORS=0
for /L %%N in (1,1,15) do (
    ..\ztbaddblock genesis.rom Engineering -t "Engineering commit %%N - build artifact record" -x2 > nul 2>&1
    if errorlevel 1 set /a ENG_ERRORS+=1
)

set ENG_COUNT=0
for %%F in (Engineering_*.ztb) do set /a ENG_COUNT+=1

if "%ENG_COUNT%"=="16" (
    echo   [PASS] Engineering branch: 16 blocks ^(1 branch + 15 added^)
    set /a PASS+=1
) else (
    echo   [FAIL] Engineering branch: expected 16 blocks, found %ENG_COUNT%
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Add 10 blocks to Legal
set LEGAL_ERRORS=0
for /L %%N in (1,1,10) do (
    ..\ztbaddblock genesis.rom Legal -t "Legal document %%N - compliance filing record" -x2 > nul 2>&1
    if errorlevel 1 set /a LEGAL_ERRORS+=1
)

set LEGAL_COUNT=0
for %%F in (Legal_*.ztb) do set /a LEGAL_COUNT+=1

if "%LEGAL_COUNT%"=="11" (
    echo   [PASS] Legal branch: 11 blocks ^(1 branch + 10 added^)
    set /a PASS+=1
) else (
    echo   [FAIL] Legal branch: expected 11 blocks, found %LEGAL_COUNT%
    set /a FAIL+=1
)
set /a TOTAL+=1
echo.

REM ============================================================
REM  TEST 4: Fetch Individual Blocks
REM ============================================================
echo --- TEST 4: Fetch Individual Blocks ---

REM Fetch trunk block 1
..\ztbfetch genesis.rom MainTrunk 1 > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Fetch trunk block 1
    set /a PASS+=1
) else (
    echo   [FAIL] Fetch trunk block 1
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Fetch trunk block 50 (middle)
..\ztbfetch genesis.rom MainTrunk 50 > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Fetch trunk block 50
    set /a PASS+=1
) else (
    echo   [FAIL] Fetch trunk block 50
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Fetch trunk block 100 (last)
..\ztbfetch genesis.rom MainTrunk 100 > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Fetch trunk block 100
    set /a PASS+=1
) else (
    echo   [FAIL] Fetch trunk block 100
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Fetch branch block
..\ztbfetch genesis.rom Sales 10 > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Fetch Sales branch block 10
    set /a PASS+=1
) else (
    echo   [FAIL] Fetch Sales branch block 10
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Fetch non-existent block (should fail)
..\ztbfetch genesis.rom MainTrunk 999 > nul 2>&1
if errorlevel 1 (
    echo   [PASS] Correctly rejected non-existent block 999
    set /a PASS+=1
) else (
    echo   [FAIL] Should have rejected non-existent block 999
    set /a FAIL+=1
)
set /a TOTAL+=1
echo.

REM ============================================================
REM  TEST 5: Full Chain Verification
REM ============================================================
echo --- TEST 5: Full Chain Verification ---

REM Verify trunk only
..\ztbverify genesis.rom MainTrunk -t > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Trunk verification passed ^(100 blocks^)
    set /a PASS+=1
) else (
    echo   [FAIL] Trunk verification failed
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Verify specific branch
..\ztbverify genesis.rom MainTrunk -b Sales > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Sales branch verification passed ^(21 blocks^)
    set /a PASS+=1
) else (
    echo   [FAIL] Sales branch verification failed
    set /a FAIL+=1
)
set /a TOTAL+=1

..\ztbverify genesis.rom MainTrunk -b Engineering > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Engineering branch verification passed ^(16 blocks^)
    set /a PASS+=1
) else (
    echo   [FAIL] Engineering branch verification failed
    set /a FAIL+=1
)
set /a TOTAL+=1

..\ztbverify genesis.rom MainTrunk -b Legal > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Legal branch verification passed ^(11 blocks^)
    set /a PASS+=1
) else (
    echo   [FAIL] Legal branch verification failed
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Verify all branches only
..\ztbverify genesis.rom MainTrunk -bb > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] All branches verification passed
    set /a PASS+=1
) else (
    echo   [FAIL] All branches verification failed
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Verify everything (trunk + all branches)
..\ztbverify genesis.rom MainTrunk > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Full verification passed ^(trunk + all branches^)
    set /a PASS+=1
) else (
    echo   [FAIL] Full verification failed
    set /a FAIL+=1
)
set /a TOTAL+=1
echo.

REM ============================================================
REM  TEST 6: Checkpoint
REM ============================================================
echo --- TEST 6: Checkpoint ---

..\ztbcheckpoint genesis.rom MainTrunk checkpoint.rom > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Checkpoint ROM created
    set /a PASS+=1
) else (
    echo   [FAIL] Checkpoint creation failed
    set /a FAIL+=1
)
set /a TOTAL+=1

if exist checkpoint.rom (
    for %%A in (checkpoint.rom) do set CPSIZE=%%~zA
    if "!CPSIZE!"=="65536" (
        echo   [PASS] Checkpoint ROM size correct: !CPSIZE! bytes
        set /a PASS+=1
    ) else (
        echo   [FAIL] Checkpoint ROM size wrong: !CPSIZE! bytes
        set /a FAIL+=1
    )
) else (
    echo   [FAIL] Checkpoint ROM file not found
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Archive old blocks
mkdir archive
move MainTrunk_*.ztb archive\ > nul 2>&1
move Sales_*.ztb archive\ > nul 2>&1
move Engineering_*.ztb archive\ > nul 2>&1
move Legal_*.ztb archive\ > nul 2>&1
copy genesis.rom archive\ > nul 2>&1

REM Confirm active directory is clean
set REMAINING=0
for %%F in (*.ztb) do set /a REMAINING+=1

if "%REMAINING%"=="0" (
    echo   [PASS] All old blocks moved to archive
    set /a PASS+=1
) else (
    echo   [FAIL] %REMAINING% blocks still in active directory
    set /a FAIL+=1
)
set /a TOTAL+=1
echo.

REM ============================================================
REM  TEST 7: Post-Checkpoint Chain
REM ============================================================
echo --- TEST 7: Post-Checkpoint Chain ---

REM Add 30 blocks using checkpoint ROM
set POSTCP_ERRORS=0
for /L %%N in (1,1,30) do (
    ..\ztbaddblock checkpoint.rom MainTrunk -t "Post-checkpoint block %%N - new era data record" -x2 > nul 2>&1
    if errorlevel 1 set /a POSTCP_ERRORS+=1
)

set POSTCP_COUNT=0
for %%F in (MainTrunk_*.ztb) do set /a POSTCP_COUNT+=1

if "%POSTCP_COUNT%"=="30" (
    echo   [PASS] 30 post-checkpoint blocks created
    set /a PASS+=1
) else (
    echo   [FAIL] Expected 30 post-checkpoint blocks, found %POSTCP_COUNT%
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Verify post-checkpoint chain
..\ztbverify checkpoint.rom MainTrunk -t > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Post-checkpoint verification passed ^(30 blocks^)
    set /a PASS+=1
) else (
    echo   [FAIL] Post-checkpoint verification failed
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Fetch a post-checkpoint block
..\ztbfetch checkpoint.rom MainTrunk 15 > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Fetch post-checkpoint block 15
    set /a PASS+=1
) else (
    echo   [FAIL] Fetch post-checkpoint block 15
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Create a branch on the post-checkpoint chain
..\ztbaddbranch checkpoint.rom MainTrunk PostCPBranch -t "Post-checkpoint branch" -x2 > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Post-checkpoint branch created
    set /a PASS+=1
) else (
    echo   [FAIL] Post-checkpoint branch creation failed
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Add blocks to post-checkpoint branch
for /L %%N in (1,1,5) do (
    ..\ztbaddblock checkpoint.rom PostCPBranch -t "Post-CP branch block %%N" -x2 > nul 2>&1
)

set PCPB_COUNT=0
for %%F in (PostCPBranch_*.ztb) do set /a PCPB_COUNT+=1

if "%PCPB_COUNT%"=="6" (
    echo   [PASS] Post-checkpoint branch: 6 blocks ^(1 branch + 5 added^)
    set /a PASS+=1
) else (
    echo   [FAIL] Post-checkpoint branch: expected 6 blocks, found %PCPB_COUNT%
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Verify post-checkpoint trunk + branch
..\ztbverify checkpoint.rom MainTrunk > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Post-checkpoint full verification passed
    set /a PASS+=1
) else (
    echo   [FAIL] Post-checkpoint full verification failed
    set /a FAIL+=1
)
set /a TOTAL+=1
echo.

REM ============================================================
REM  TEST 8: Independent Archive Verification
REM ============================================================
echo --- TEST 8: Independent Archive Verification ---

pushd archive

REM Verify archived trunk
..\..\ztbverify genesis.rom MainTrunk -t > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Archived trunk verification passed ^(100 blocks^)
    set /a PASS+=1
) else (
    echo   [FAIL] Archived trunk verification failed
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Verify archived branches
..\..\ztbverify genesis.rom MainTrunk > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Archived full verification passed ^(trunk + 3 branches^)
    set /a PASS+=1
) else (
    echo   [FAIL] Archived full verification failed
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Fetch an archived block
..\..\ztbfetch genesis.rom MainTrunk 75 > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Fetch archived trunk block 75
    set /a PASS+=1
) else (
    echo   [FAIL] Fetch archived trunk block 75
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Fetch an archived branch block
..\..\ztbfetch genesis.rom Sales 15 > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Fetch archived Sales branch block 15
    set /a PASS+=1
) else (
    echo   [FAIL] Fetch archived Sales branch block 15
    set /a FAIL+=1
)
set /a TOTAL+=1

popd
echo.

REM ============================================================
REM  TEST 9: File Payload
REM ============================================================
echo --- TEST 9: File Payload ---

REM Create a test file
echo This is a test file with some content for ZTB blockchain storage. > testfile.txt
echo Line 2: More data to ensure the payload is meaningful. >> testfile.txt
echo Line 3: ZOSCII Tamperproof Blockchain test payload. >> testfile.txt

..\ztbaddblock checkpoint.rom MainTrunk -f testfile.txt -x2 > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] File payload block created
    set /a PASS+=1
) else (
    echo   [FAIL] File payload block creation failed
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Verify still passes after file payload block
..\ztbverify checkpoint.rom MainTrunk -t > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Verification passes with file payload block
    set /a PASS+=1
) else (
    echo   [FAIL] Verification failed after file payload block
    set /a FAIL+=1
)
set /a TOTAL+=1
echo.

REM ============================================================
REM  TEST 10: Tamper Detection
REM ============================================================
echo --- TEST 10: Tamper Detection ---

REM Create a small test chain for tampering
mkdir tampertest
pushd tampertest
copy ..\genesis.rom . > nul 2>&1

..\..\ztbaddblock genesis.rom TamperChain -t "Block one - original data" -x2 > nul 2>&1
..\..\ztbaddblock genesis.rom TamperChain -t "Block two - original data" -x2 > nul 2>&1
..\..\ztbaddblock genesis.rom TamperChain -t "Block three - original data" -x2 > nul 2>&1

REM Verify before tampering
..\..\ztbverify genesis.rom TamperChain -t > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Tamper chain verified clean before tampering
    set /a PASS+=1
) else (
    echo   [FAIL] Tamper chain should verify clean
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Tamper with block 2 by appending a byte
for %%F in (TamperChain_0002_*.ztb) do (
    echo X>> "%%F"
)

REM Verify should now fail
..\..\ztbverify genesis.rom TamperChain -t > nul 2>&1
if errorlevel 1 (
    echo   [PASS] Tamper correctly detected after modification
    set /a PASS+=1
) else (
    echo   [FAIL] Tamper NOT detected - verification should have failed
    set /a FAIL+=1
)
set /a TOTAL+=1

popd
echo.

REM ============================================================
REM  TEST 11: Duplicate Branch Rejection
REM ============================================================
echo --- TEST 11: Edge Cases ---

REM Try to create a branch that already exists (should fail)
..\ztbaddbranch checkpoint.rom MainTrunk PostCPBranch -t "Duplicate branch" -x2 > nul 2>&1
if errorlevel 1 (
    echo   [PASS] Correctly rejected duplicate branch name
    set /a PASS+=1
) else (
    echo   [FAIL] Should have rejected duplicate branch
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Try to create a branch from non-existent trunk (should fail)
..\ztbaddbranch checkpoint.rom NonExistent NewBranch -t "Bad branch" -x2 > nul 2>&1
if errorlevel 1 (
    echo   [PASS] Correctly rejected non-existent trunk
    set /a PASS+=1
) else (
    echo   [FAIL] Should have rejected non-existent trunk
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Wrong argument count (should fail)
..\ztbaddblock genesis.rom > nul 2>&1
if errorlevel 1 (
    echo   [PASS] Correctly rejected missing arguments
    set /a PASS+=1
) else (
    echo   [FAIL] Should have rejected missing arguments
    set /a FAIL+=1
)
set /a TOTAL+=1
echo.

REM ============================================================
REM  TEST 13: Supplied Block ID (-i parameter) — X2 and normal mode
REM ============================================================
echo --- TEST 13: Supplied Block ID ^(-i parameter^) ---

mkdir idtest
pushd idtest
copy ..\genesis.rom . > nul 2>&1

set TEST_GUID_1=A1B2C3D4-E5F6-4A7B-8C9D-E0F1A2B3C4D5
set TEST_GUID_2=mySillyID
set TEST_GUID_BAD=NOT-A-VALID-GUID-AT-ALL-XXXXXXXXX

REM Supply a valid block ID in normal mode
..\..\ztbaddblock genesis.rom IDChain -t "Normal block with supplied ID" -i %TEST_GUID_1% > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Normal mode block created with supplied ID
    set /a PASS+=1
) else (
    echo   [FAIL] Normal mode block creation with supplied ID failed
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Confirm file named with supplied GUID
if exist "IDChain_0001_%TEST_GUID_1%.ztb" (
    echo   [PASS] Normal mode block filename contains supplied GUID
    set /a PASS+=1
) else (
    echo   [FAIL] Normal mode block filename does not contain supplied GUID
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Supply a valid block ID in X2 mode (block 2 — has a previous block to XOR with)
..\..\ztbaddblock genesis.rom IDChain -t "X2 block with supplied ID" -x2 -i %TEST_GUID_2% > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] X2 mode block created with supplied ID
    set /a PASS+=1
) else (
    echo   [FAIL] X2 mode block creation with supplied ID failed
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Confirm X2 block file named with supplied GUID
if exist "IDChain_0002_%TEST_GUID_2%.ztb" (
    echo   [PASS] X2 mode block filename contains supplied GUID
    set /a PASS+=1
) else (
    echo   [FAIL] X2 mode block filename does not contain supplied GUID
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Fetch and verify both blocks round-trip correctly
..\..\ztbfetch genesis.rom IDChain 1 > fetchout1.txt 2>&1
if not errorlevel 1 (
    echo   [PASS] Normal block with supplied ID fetched successfully
    set /a PASS+=1
) else (
    echo   [FAIL] Fetch of normal block with supplied ID failed
    set /a FAIL+=1
)
set /a TOTAL+=1

findstr /i "%TEST_GUID_1%" fetchout1.txt > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Fetched header contains correct supplied ID for normal block
    set /a PASS+=1
) else (
    echo   [FAIL] Fetched header does not contain supplied ID for normal block
    set /a FAIL+=1
)
set /a TOTAL+=1

..\..\ztbfetch genesis.rom IDChain 2 > fetchout2.txt 2>&1
if not errorlevel 1 (
    echo   [PASS] X2 block with supplied ID fetched successfully
    set /a PASS+=1
) else (
    echo   [FAIL] Fetch of X2 block with supplied ID failed
    set /a FAIL+=1
)
set /a TOTAL+=1

findstr /i "%TEST_GUID_2%" fetchout2.txt > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Fetched header contains correct supplied ID for X2 block
    set /a PASS+=1
) else (
    echo   [FAIL] Fetched header does not contain supplied ID for X2 block
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Verify the full mixed-mode chain with supplied IDs
..\..\ztbverify genesis.rom IDChain -t > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] Mixed-mode chain with supplied IDs verifies correctly
    set /a PASS+=1
) else (
    echo   [FAIL] Mixed-mode chain with supplied IDs failed verification
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Supply a valid block ID when creating an X2 branch
..\..\ztbaddbranch genesis.rom IDChain IDBranch -t "X2 branch with supplied ID" -x2 -i %TEST_GUID_1% > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] X2 branch created with supplied ID
    set /a PASS+=1
) else (
    echo   [FAIL] X2 branch creation with supplied ID failed
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Confirm branch file named with supplied GUID
if exist "IDBranch_0001_%TEST_GUID_1%.ztb" (
    echo   [PASS] X2 branch filename contains supplied GUID
    set /a PASS+=1
) else (
    echo   [FAIL] X2 branch filename does not contain supplied GUID
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Verify X2 branch with supplied ID
..\..\ztbverify genesis.rom IDChain -b IDBranch > nul 2>&1
if not errorlevel 1 (
    echo   [PASS] X2 branch with supplied ID verifies correctly
    set /a PASS+=1
) else (
    echo   [FAIL] X2 branch with supplied ID failed verification
    set /a FAIL+=1
)
set /a TOTAL+=1

REM Reject an invalid GUID (should fail)
REM ..\..\ztbaddblock genesis.rom IDChain -t "Bad ID" -x2 -i %TEST_GUID_BAD% > nul 2>&1
REM if errorlevel 1 (
REM     echo   [PASS] Correctly rejected invalid block ID
REM     set /a PASS+=1
REM ) else (
REM     echo   [FAIL] Should have rejected invalid block ID
REM     set /a FAIL+=1
REM )
REM set /a TOTAL+=1

REM Reject -i with no argument (should fail)
..\..\ztbaddblock genesis.rom IDChain -t "Missing ID" -x2 -i > nul 2>&1
if errorlevel 1 (
    echo   [PASS] Correctly rejected -i with no argument
    set /a PASS+=1
) else (
    echo   [FAIL] Should have rejected -i with no argument
    set /a FAIL+=1
)
set /a TOTAL+=1

popd
echo.

REM ============================================================
REM  SUMMARY
REM ============================================================
echo ============================================================
echo  RESULTS
echo ============================================================
echo.
echo   Total tests: %TOTAL%
echo   Passed:      %PASS%
echo   Failed:      %FAIL%
echo.

if "%FAIL%"=="0" (
    echo   +++ ALL TESTS PASSED +++
) else (
    echo   --- %FAIL% TEST^(S^) FAILED ---
)

echo.
echo ============================================================

REM Clean up
cd ..
REM rd /s /q testdata_x2

endlocal