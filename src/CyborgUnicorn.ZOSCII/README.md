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
bool blnMatch1 = ZVerify.File("encoded.zos", "original.bin", rom);
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

## License

MIT LICENSE
Commercial Licenses Available

(c) 2026 Cyborg Unicorn Pty Ltd - https://cyborgunicorn.com.au