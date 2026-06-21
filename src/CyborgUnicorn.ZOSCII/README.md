# CyborgUnicorn.ZOSCII

ZOSCII (Zero Overhead Secure Code Information Interchange) SDK for .NET.

Information-theoretically secure encoding achieving **I(M;A)=0** — the encoded output
is statistically independent of the input without the ROM key material.

## Naming conventions

| Prefix | Purpose |
|---|---|
| `Z` | ZOSCII encode / decode / verify |
| `B` | Binary operations: compare, split, join, verify |
| `MQ` | Message Queue operations (queue, store, replication, monitoring) |

---

## ZEncode / ZDecode

All string overloads assume UTF-8. Both ends must agree on encoding.

```csharp
byte[] encoded   = ZEncode.Bytes(data, rom);
byte[] decoded   = ZDecode.Bytes(encoded, rom);
byte[] chain     = ZEncode.Chain(data, new[] { rom1, rom2, rom3 });
byte[] unchained = ZDecode.Chain(chain, new[] { rom1, rom2, rom3 });
byte[] tango     = ZEncode.Chain(data, new[] { rom1, rom2, rom3 }, true);   // Tango: round-robin ROMs per byte, 2x expansion, up to 3x entropy
byte[] untango   = ZDecode.Chain(tango, new[] { rom1, rom2, rom3 }, true);  // must match encode
bool   ok        = ZEncode.ChainFile("input.bin", "output.zoc", new[] { rom1, rom2, rom3 });
bool   ok        = ZDecode.ChainFile("output.zoc", "recovered.bin", new[] { rom1, rom2, rom3 });
ZEncode.File("input.bin", "output.zoc", rom);
ZDecode.File("output.zoc", "recovered.bin", rom);
string b64       = ZEncode.ToBase64(data, rom);
byte[] back      = ZDecode.FromBase64(b64, rom);
byte[] encoded   = ZEncode.String("hello", rom);
string text      = ZDecode.ToString(encoded, rom);
string text      = ZDecode.FileToString("output.zoc", rom);
```

---

## ZVerify / BVerify

```csharp
bool blnMatch1 = ZVerify.File("encoded.zoc", "original.bin", rom);
bool blnMatch2 = ZVerify.Bytes(encodedBytes, plainBytes, rom);
bool blnMatch4 = BVerify.File("file1.bin", "file2.bin");
bool blnMatch5 = BVerify.Bytes(arr1, arr2);
```

---

## ZOSCIIRom

```csharp
using (ZOSCIIRom rom = ZOSCIIRom.FromFile("mykey.jpg")) { }
using (ZOSCIIRom rom = ZOSCIIRom.FromBytes(rawBytes)) { }
using (ZOSCIIRom rom = ZOSCIIRom.FromBase64(base64String)) { }
```

---

## SecureDelete

```csharp
SecureDelete.File("sensitive.bin");
SecureDelete.Folder("sensitive_dir");
```

---

## MQClient

MQPublishResult: Success, ErrorMessage, ServerMessage, StoredName (Put only)
MQFetchResult:   HasMessage, EncodedBytes, Filename, Pointer
Encoding/decoding is the caller's responsibility — MQClient sends and receives raw bytes.
Scan returns string[] (unidentified names) or null on failure
Identify returns string[] (identified names) or null on failure

```csharp
var mq = new MQClient();              // default 60s timeout
var mq = new MQClient(120);           // custom timeout

// User-Agent control (default: random GUID per request)
mq.SetUserAgentRandom();              // fresh GUID on every request (default)
mq.SetUserAgentNone();                // omit User-Agent header entirely
mq.SetUserAgent("MyApp/1.0");        // fixed string until changed

string strQueueServer = "https://your-server/index.php";

// Queue
MQPublishResult pub    = mq.Publish(strQueueServer, "myqueue", data);
MQCheckStatus   status = mq.Check(strQueueServer, "myqueue", lastPointer);
MQFetchResult   result = mq.FetchNext(strQueueServer, "myqueue", lastPointer);
if (result.HasMessage) { lastPointer = result.Pointer; }

// Store
MQPublishResult  upload   = mq.Put(strQueueServer, data);
MQFetchResult    retrieve = mq.Get(strQueueServer, upload.StoredName);
string[] arrUnidentified  = mq.Scan(strQueueServer);
string[] arrIdentified    = mq.Identify(strQueueServer, arrUnidentified);

// Replication - one message per call, caller loops and persists pointer
// Replicate without localQueue stores as unidentified (-u suffix), discoverable via Scan
string strNewPointer;
MQPublishResult rep = mq.Replicate(remoteURL, remoteQueue, strQueueServer, localQueue,
    lastPointer, out strNewPointer, intRetentionDays_a: 7);
if (rep.Success && rep.ServerMessage != "up-to-date") { lastPointer = strNewPointer; }

MQPublishResult rep = mq.Replicate(remoteURL, remoteQueue, strQueueServer, "",
    lastPointer, out strNewPointer, intRetentionDays_a: 7);  // empty localQueue = store as unidentified
```

---

## ZRollingHash

BRAINLESS rolling hash — 4-pass XOR chain, 32-bit (4-byte) output.
Two modes: reverse (default, requires complete payload) and forward (streamable).
Forward and reverse produce different hashes for the same input.
No ROM required. Works on bytes or files.

```csharp
// Hash bytes — reverse (default, requires full payload)
byte[] arrHash = ZRollingHash.Bytes(arrData);

// Hash bytes — forward (streamable)
byte[] arrHash = ZRollingHash.Bytes(arrData, true);

// Hash a file
byte[] arrHash = ZRollingHash.File("data.bin");
byte[] arrHash = ZRollingHash.File("data.bin", true);  // forward

// Verify
bool blnOk = ZRollingHash.Verify(arrData, arrHash);
bool blnOk = ZRollingHash.Verify(arrData, arrHash, true);  // forward
bool blnOk = ZRollingHash.VerifyFile("data.bin", arrHash);
bool blnOk = ZRollingHash.VerifyFile("data.bin", arrHash, true);  // forward
```

---

## Source

Extracts the embedded source code archive for this package.

```csharp
bool blnOk = Source.SaveAs(@"C:\MyFolder\CyborgUnicorn.ZOSCII.source.zip");
```

To embed source.zip when building the nuget, add to the .csproj:
```xml
<ItemGroup>
  <EmbeddedResource Include="source.zip" />
</ItemGroup>
```

---

## ZTBChain

ZOSCII Tamperproof Blockchain — quantum-proof by structure. Integrity via rolling ROM + hash,
no cryptographic assumptions. Transparent ledger: chain structure is public, payload security
is the caller's responsibility.

Block files on disk: `<BlockID>.ztb`
Genesis block: `<BlockID>.ztb` (65536 bytes: byte[0]=block_type, bytes[1-65535]=ROM)
Truncation block: `<BlockID>.ztb` (111 + 65536 bytes: raw header + raw ROM, not ZOSCII-encoded)

**Enums**

`ZTBBlockType`: Genesis=0, Normal=1, Checkpoint=2, Truncation=3, Finalise=4, Bridge=5
`ZTBHashType`:  CRC32Full=0, CRC321KB=1, RollingFull=2 (default), Rolling1KB=3

**Result types**

`ZTBBlockResult`:  Success, BlockID, PrevBlockID, TrunkID, IsBranch, BlockType, HashType, Hash, PrevHash, PayloadLen, PaddedLen, Filename, Payload
`ZTBVerifyResult`: Success, VerifiedBlocks, FailedBlocks

**Constants**

`ZTBChain.NULL_GUID`           — `"00000000-0000-0000-0000-000000000000"`
`ZTBChain.GENESIS_SIZE_PUBLIC` — 65536
`ZTBChain.HEADER_RAW_SIZE`     — 111

```csharp
// Create genesis block from 1-3 entropy source files (JPEG, MP3, etc.)
// Caller supplies the GUID for the genesis block
bool ok = ZTBChain.Create(strGenesisBlockID, new[] { "photo.jpg", "music.mp3" },
                           @"C:\MyChain", "MainTrunk");

// Open a chain (file-based or memory chain with null workDir)
ZTBChain chain = ZTBChain.Open(@"C:\MyChain", "MainTrunk");
ZTBChain chain = ZTBChain.Open(@"C:\MyChain", "MainTrunk", ZTBHashType.CRC32Full);
ZTBChain chain = ZTBChain.Open(null, "MainTrunk");   // memory chain — wire callbacks before use

// Add blocks — caller supplies both GUIDs; strPrevBlockID = null for first block
ZTBBlockResult r = chain.AddBlock(strNewBlockID, strPrevBlockID, data);
ZTBBlockResult r = chain.AddBlockText(strNewBlockID, strPrevBlockID, "hello");
ZTBBlockResult r = chain.AddBlockFile(strNewBlockID, strPrevBlockID, "doc.bin");

// Checkpoint — labeled marker block (BlockType=Checkpoint)
ZTBBlockResult r = chain.AddCheckpoint(strNewBlockID, strPrevBlockID, "New financial year 2026");

// Truncate — rewrites the checkpoint's prev block as a Truncation block in-place,
// storing the full rolling ROM as its raw payload. Everything below can be SecureDeleted.
// The chain above the checkpoint remains fully verifiable.
ZTBBlockResult r = chain.Truncate(strNewBlockID, strCheckpointBlockID);

// Finalise — permanently seal the chain after the specified block
ZTBBlockResult r = chain.Finalise(strNewBlockID, strPrevBlockID, "Optional label");

// Branch — called on the BRANCH chain; strPrevBlockID is the trunk tip; strTrunkChainID is trunk's ChainID
ZTBChain branch  = ZTBChain.Open(@"C:\MyChain", "Sales");
ZTBBlockResult r = branch.AddBranch(strNewBlockID, strTrunkTipBlockID, data, "MainTrunk");

// Add subsequent branch blocks — open the branch chain and call AddBlock normally
ZTBBlockResult r = branch.AddBlock(strNewBlockID, strPrevBlockID, data);

// Fetch — direct access by BlockID
ZTBBlockResult r = chain.FetchBlock(strBlockID);
byte[] payload   = r.Payload;

// Verify — single block (blnWalk=false) or walk back to root (blnWalk=true)
// Stops cleanly at Genesis or Truncation block
ZTBVerifyResult v = chain.Verify(strBlockID, true);    // walk full chain
ZTBVerifyResult v = chain.Verify(strBlockID, false);   // single block only

// Callbacks — hook into block I/O (all null by default)
chain.OnBeforeSaveBlock = (result, bytes, path) => true;   // return false to cancel
chain.OnSaveBlock       = (result, bytes, path) => false;  // return true to skip disk write
chain.OnAfterSaveBlock  = (result, path)        => true;   // return false = treat as failed
chain.OnLoadBlock       = (filename)            => null;   // return bytes to override disk read
chain.OnFindGenesis     = (chainID)             => null;   // return genesis bytes for memory chains

// Properties
string id      = chain.ChainID;
string workDir = chain.WorkDir;
```

---

**Block format**

```
bytes 0-110:   RAW (not encoded)
  byte  0:     block_type
  byte  1:     block_version (1)
  byte  2:     is_branch
  bytes 3-38:  trunk_id       (36 bytes ASCII)
  bytes 39-74: block_id       (36 bytes ASCII)
  bytes 75-110:prev_block_id  (36 bytes ASCII)
bytes 111+:    ZOSCII encoded (Normal, Checkpoint, Finalise)
  byte  0:     hash_type
  bytes 1-4:   hash (of full unencoded block, hash field zeroed)
  bytes 5-8:   prev_hash (hash of entire previous block)
  bytes 9-12:  payload_len
  bytes 13-16: padded_len
  bytes 17+:   payload (xorshift32 padded to 512 bytes minimum)
```

Genesis block: bytes 0-65535, byte[0]=block_type=0, bytes[1-65535]=ROM. Not ZOSCII-encoded.
Truncation block: bytes 0-110 raw header + bytes 111-65646 raw ROM. Not ZOSCII-encoded.

**Tamper detection**

| HashType     | Detects tamper within 1KB | Detects tamper beyond 1KB |
|--------------|--------------------------|--------------------------|
| RollingFull  | Yes                      | Yes                      |
| Rolling1KB   | Yes                      | No (by design)           |
| CRC32Full    | Yes                      | Yes                      |
| CRC321KB     | Yes                      | No (by design)           |

**Truncation workflow**

```
(before truncation)               (after truncation + SecureDelete of old blocks)
block 13 (Normal)                 block 13 (Normal)
  └── block 12 (Normal)             └── block 12 (Normal)
        └── block 11 (Checkpoint)         └── block 11 (Checkpoint)
              └── block 10 (Normal)             └── block 10 (Truncation, payload=ROM)
                    └── block 9
                          └── ...
                                └── genesis
```

`Truncate(newGUID, checkpointBlockID)` overwrites the checkpoint's prev block in-place with a
Truncation block (same GUID, same PrevBlockID, BlockType=Truncation, raw ROM payload). All blocks
below the Truncation block can be `SecureDelete`d. The chain above the checkpoint verifies normally.

---

## Encryption

Plugin-based encryption registry. Drop DLLs implementing `IEncryptionProvider` into a folder,
probe by filename, and the encryption appears in the dropdown automatically.

**Interface** (implemented by each plugin DLL):

```csharp
public interface IEncryptionProvider
{
    EncryptionType GetEncryptionType();
    byte[]? Encrypt(byte[]? plaintext, byte[]? key, byte[]? iv = null);
    byte[]? Decrypt(byte[]? ciphertext, byte[]? key, byte[]? iv = null);
    byte[]? GenerateKey();
    byte[]? GenerateIV();
}

public struct EncryptionType
{
    public string Code { get; set; }   // "AES256GCM"
    public string Name { get; set; }   // "AES-256-GCM"
}
```

**Host application usage:**

```csharp
using Encrypt;

var registry = new Encryption();

// Probe specific DLLs — caller controls trust
string[] arrPlugins = new[] { "encryption1.dll", "encryption2.dll", "encryption3.dll" };
registry.Probe(@"Plugins\", arrPlugins);

// Populate dropdown — GetMethods() returns string[,] matching GetBootstrapMethods() pattern
string[,] arrMethods = registry.GetMethods();
// arrMethods[0,0] = "AES256GCM"   → dropdown value
// arrMethods[0,1] = "AES-256-GCM" → dropdown label

// User picks "AES256GCM" from dropdown
string strCode = "AES256GCM";
byte[] arrKey  = registry.GenerateKey(strCode);
byte[] arrIV   = registry.GenerateIV(strCode);   // null if algorithm doesn't use IV

// UNSIGNAL first, then encrypt
byte[] arrUnsignalled = UEncode.Bytes(arrPlaintext, rom);
byte[] arrCiphertext  = registry.Encrypt(strCode, arrUnsignalled, arrKey, arrIV);
byte[] arrRecovered   = registry.Decrypt(strCode, arrCiphertext, arrKey, arrIV);
```

**Available encryption providers** (compile each to its own DLL):

| DLL | Code | Name | IV Required |
|-----|------|------|-------------|
| XORProvider.dll | `XOR` | XOR | ❌ |
| DESProvider.dll | `DES` | DES | ✅ (8 bytes) |
| TripleDESProvider.dll | `3DES` | 3DES | ✅ (8 bytes) |
| RC2Provider.dll | `RC2` | RC2 | ✅ (8 bytes) |
| AESCBCProvider.dll | `AES256CBC` | AES-256-CBC | ✅ (16 bytes) |
| AESCCMProvider.dll | `AES256CCM` | AES-256-CCM | ✅ (13 bytes) |
| AESGCMProvider.dll | `AES256GCM` | AES-256-GCM | ✅ (12 bytes) |
| ChaCha20Provider.dll | `CHACHA20` | ChaCha20-Poly1305 | ✅ (12 bytes) |
| RSAProvider.dll | `RSA4096` | RSA-4096 | ❌ |
| BRAINLESSProvider.dll | `BRAINLESS` | BRAINLESS Ouroboros | ✅ (1 byte: mode selector) |

**Output formats** (ciphertext includes IV + tag where applicable):

- CBC/3DES/DES/RC2: `IV (8/16 bytes) + Ciphertext`
- GCM/CCM/ChaCha20: `IV (12-13 bytes) + Tag (16 bytes) + Ciphertext`
- XOR: `Ciphertext only`
- RSA: `Ciphertext only` (key blob determines public/private)

**Adding a new encryption provider** (Amiga-style probing):

```csharp
// Drop encryption4.dll in the plugins folder, add to filename list
registry.Probe("plugins", new[] { "encryption1.dll", "encryption2.dll", "encryption3.dll", "encryption4.dll" });
// encryption4.dll appears in dropdown automatically — no code changes
```

**Building a custom provider:**

1. Reference `Encrypt` namespace (or copy the interface into your project)
2. Implement `IEncryptionProvider`
3. Compile to DLL
4. Drop in plugins folder

```csharp
public class MyProvider : IEncryptionProvider
{
    public EncryptionType GetEncryptionType()
    {
        return new EncryptionType { Code = "MYCIPHER", Name = "My Cipher" };
    }

    public byte[]? Encrypt(byte[]? plaintext, byte[]? key, byte[]? iv = null)
    {
        // Your encryption logic here
    }

    public byte[]? Decrypt(byte[]? ciphertext, byte[]? key, byte[]? iv = null)
    {
        // Your decryption logic here
    }

    public byte[]? GenerateKey() => new byte[32];
    public byte[]? GenerateIV() => null;
}
```

**Why this works with POWERUP:**

UNSIGNAL pre-encoding removes the attack surface before encryption sees the data.
Broken algorithms (DES, RC2, XOR) become viable because the pre-encoding eliminates
the patterns their vulnerabilities depend on. The encryption algorithm becomes
almost irrelevant — the security is in the UNSIGNAL layer.

---

## License

MIT LICENSE
Commercial Licenses Available

(c) 2026 Cyborg Unicorn Pty Ltd - https://cyborgunicorn.com.au