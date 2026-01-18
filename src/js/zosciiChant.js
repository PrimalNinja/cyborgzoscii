/**
 * ZOSCII: The Chant Implementation
 * ES5 Function Constructor Pattern
 * 
 * cleanIt()                = Clear the ROM state
 * foldIt(rom, blocks)      = Fold the ROM into 2D lookup array using memory blocks
 * alignIt(message)         = Align message to random positions (encode)
 * checkIt(addresses)       = Check addresses against ROM (decode)
 * 
 * (c) 2026 Cyborg Unicorn Pty Ltd
 * MIT Licensed - Because sharing is caring
 */

function ZOSCIIChant() {
  var rom = null;
  var foldedROM = null;

  // Private: Convert string to Uint8Array
  function stringToBytes(str) {
    var bytes = new Uint8Array(str.length);
    for (var i = 0; i < str.length; i++) {
      bytes[i] = str.charCodeAt(i);
    }
    return bytes;
  }

  // Private: Convert Uint8Array to string
  function bytesToString(bytes) {
    var str = '';
    for (var i = 0; i < bytes.length; i++) {
      str += String.fromCharCode(bytes[i]);
    }
    return str;
  }

  // Public: Clean the ROM state
  this.cleanIt = function() {
    rom = null;
    foldedROM = null;
    return this;
  };

  // Public: Fold ROM into 2D lookup array
  // romData: Uint8Array containing the ROM/binary data
  // Automatically uses entire ROM (up to 64KB for Uint16 addressing)
  this.foldIt = function(romData) {
    rom = new Uint8Array(romData);
    
    // Validate ROM size for Uint16 addressing (max 64KB)
    if (rom.length > 65536) {
      throw new Error('ROM size exceeds 64KB (65536 bytes). Use smaller ROM or implement Uint32 addressing.');
    }
    
    // Initialize 256 arrays (one per possible byte value)
    foldedROM = new Array(256);
    for (var i = 0; i < 256; i++) {
      foldedROM[i] = [];
    }
    
    // Fold: Map each position to its byte value's array
    // Process entire ROM from 0 to rom.length
    for (var pos = 0; pos < rom.length; pos++) {
      var byteValue = rom[pos];
      foldedROM[byteValue].push(pos);
    }
    
    return this;
  };

  // Public: Align message to random positions (encode)
  // message: String or Uint8Array
  // Returns: Uint16Array of addresses (2x expansion)
  this.alignIt = function(message) {
    if (!foldedROM) {
      throw new Error('Cannot align: ROM not folded. Call foldIt() first.');
    }

    var messageBytes = typeof message === 'string' 
      ? stringToBytes(message)
      : new Uint8Array(message);

    // Use Uint16Array for 2x expansion (not Uint32)
    var addresses = new Uint16Array(messageBytes.length);
    
    for (var i = 0; i < messageBytes.length; i++) {
      var byteValue = messageBytes[i];
      var positions = foldedROM[byteValue];
      
      if (positions.length === 0) {
        throw new Error(
          'Cannot align: byte ' + byteValue + 
          ' (0x' + byteValue.toString(16).toUpperCase() + ')' +
          ' not found in ROM'
        );
      }
      
      var randomIndex = Math.floor(Math.random() * positions.length);
      addresses[i] = positions[randomIndex];
    }
    
    return addresses;
  };

  // Public: Check addresses against ROM (decode)
  // addresses: Uint16Array or array of addresses
  // Returns: Uint8Array of decoded bytes
  this.checkIt = function(addresses) {
    if (!rom) {
      throw new Error('Cannot check: ROM not loaded. Call foldIt() first.');
    }

    var addressArray = new Uint16Array(addresses);
    var decoded = new Uint8Array(addressArray.length);
    
    for (var i = 0; i < addressArray.length; i++) {
      decoded[i] = rom[addressArray[i]];
    }
    
    return decoded;
  };

  // Public: Convenience - encode string, returns Uint16Array
  this.encodeString = function(message) {
    return this.alignIt(message);
  };

  // Public: Convenience - decode to string
  this.decodeToString = function(addresses) {
    var bytes = this.checkIt(addresses);
    return bytesToString(bytes);
  };

  // Public: Convenience - encode binary, returns Uint16Array
  this.encodeBinary = function(binaryData) {
    return this.alignIt(binaryData);
  };

  // Public: Convenience - decode to binary
  this.decodeToBinary = function(addresses) {
    return this.checkIt(addresses);
  };
}

// ============================================
// TEST HARNESS
// ============================================

function generateROM(size) {
  var rom = new Uint8Array(size);
  for (var i = 0; i < size; i++) {
    rom[i] = Math.floor(Math.random() * 256);
  }
  return rom;
}

function arrayToString(arr) {
  var result = [];
  for (var i = 0; i < arr.length && i < 10; i++) {
    result.push(arr[i]);
  }
  if (arr.length > 10) {
    result.push('... (' + arr.length + ' total)');
  }
  return result.join(', ');
}

function bytesToString(bytes) {
  var str = '';
  for (var i = 0; i < bytes.length; i++) {
    str += String.fromCharCode(bytes[i]);
  }
  return str;
}

// Generate ROM (64KB)
var rom = generateROM(65536);

// Create instance
var zoscii = new ZOSCIIChant();

// Test 1: Basic encoding
var message = 'The code IS the chanting itself';

console.log('Message: ' + message);
console.log('');

// Encode: cleanIt().foldIt(rom).alignIt(message)
var addresses = zoscii.cleanIt().foldIt(rom).alignIt(message);
console.log('Encoded: ' + arrayToString(addresses));
console.log('Size: ' + (addresses.length * 2) + ' bytes (2x expansion)');
console.log('');

// Decode: checkIt(addresses)
var decoded = zoscii.checkIt(addresses);
console.log('Decoded: ' + bytesToString(decoded));
console.log('Match: ' + (bytesToString(decoded) === message ? 'PASS' : 'FAIL'));
console.log('');

// Test 2: Smaller ROM
console.log('Test with 8KB ROM:');
var smallROM = generateROM(8192);
var zoscii2 = new ZOSCIIChant();
var message2 = 'ZOSCII';

var addresses2 = zoscii2.cleanIt().foldIt(smallROM).alignIt(message2);
console.log('Message: ' + message2);
console.log('Encoded: ' + arrayToString(addresses2));
console.log('Decoded: ' + zoscii2.decodeToString(addresses2));
console.log('Match: ' + (zoscii2.decodeToString(addresses2) === message2 ? 'PASS' : 'FAIL'));