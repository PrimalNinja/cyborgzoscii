#!/usr/bin/env python3
"""
ZOSCII Command Line Tool
"""
import argparse
import sys
from pathlib import Path

def main():
    parser = argparse.ArgumentParser(description='ZOSCII Encoder/Decoder')
    subparsers = parser.add_subparsers(dest='command', help='Commands')
    
    # Encode subcommand
    encode_parser = subparsers.add_parser('encode', help='Encode message to ZOSCII')
    encode_parser.add_argument('rom_file', help='ROM file to use for encoding')
    encode_parser.add_argument('message', help='Message to encode')
    encode_parser.add_argument('-o', '--output', help='Output file (default: message.bin)')
    encode_parser.add_argument('-e', '--encoding', choices=['ascii', 'petscii', 'ebcdic'], 
                              default='ascii', help='Input character encoding')
    encode_parser.add_argument('-m', '--memory-blocks', help='Memory blocks JSON file')
    encode_parser.add_argument('-u', '--unmappable', type=int, default=42,
                              help='Character code for unmappable chars (default: 42)')
    encode_parser.add_argument('-v', '--verbose', action='store_true', help='Verbose output')
    
    # Decode subcommand
    decode_parser = subparsers.add_parser('decode', help='Decode ZOSCII to message')
    decode_parser.add_argument('rom_file', help='ROM file used for encoding')
    decode_parser.add_argument('address_file', help='ZOSCII address file')
    decode_parser.add_argument('-o', '--output', help='Output text file')
    decode_parser.add_argument('-v', '--verbose', action='store_true', help='Verbose output')
    
    # Info subcommand
    info_parser = subparsers.add_parser('info', help='Analyze ROM file for ZOSCII compatibility')
    info_parser.add_argument('rom_file', help='ROM file to analyze')
    info_parser.add_argument('-m', '--memory-blocks', help='Memory blocks JSON file')
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return 1
    
    try:
        if args.command == 'encode':
            return cmd_encode(args)
        elif args.command == 'decode':
            return cmd_decode(args)
        elif args.command == 'info':
            return cmd_info(args)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

def cmd_encode(args):
    """Handle encode command."""
    import json
    
    # Load ROM file
    try:
        with open(args.rom_file, 'rb') as f:
            rom_data = f.read()
        if args.verbose:
            print(f"Loaded ROM: {args.rom_file} ({len(rom_data)} bytes)")
    except FileNotFoundError:
        print(f"Error: ROM file '{args.rom_file}' not found", file=sys.stderr)
        return 1
    
    # Load memory blocks
    if args.memory_blocks:
        try:
            with open(args.memory_blocks, 'r') as f:
                memory_blocks = json.load(f)
        except FileNotFoundError:
            print(f"Error: Memory blocks file '{args.memory_blocks}' not found", file=sys.stderr)
            return 1
    else:
        memory_blocks = [{'start': 0, 'size': len(rom_data)}]
    
    if args.verbose:
        print(f"Memory blocks: {len(memory_blocks)} blocks")
        for i, block in enumerate(memory_blocks):
            print(f"  Block {i}: 0x{block['start']:04X}-0x{block['start'] + block['size'] - 1:04X}")
    
    # Create encoder
    encoder = ZOSCIIEncoder(rom_data, memory_blocks)
    
    # Select converter
    converter = None
    if args.encoding == 'petscii':
        converter = petscii_to_ascii
    elif args.encoding == 'ebcdic':
        converter = ebcdic_to_ascii
    
    # Encode message
    try:
        addresses = encoder.encode(args.message, converter, args.unmappable)
        if args.verbose:
            print(f"Encoded message: '{args.message}' ({len(args.message)} chars)")
            print(f"Generated addresses: {len(addresses)}")
            print(f"Encoding: {args.encoding}")
    except Exception as e:
        print(f"Encoding failed: {e}", file=sys.stderr)
        return 1
    
    # Save to file
    output_file = args.output or 'message.bin'
    try:
        encoder.save_addresses(addresses, output_file)
        print(f"Saved to: {output_file}")
    except Exception as e:
        print(f"Failed to save file: {e}", file=sys.stderr)
        return 1
    
    # Display results
    if args.verbose:
        print("\nAddress mapping:")
        for i, addr in enumerate(addresses):
            char = args.message[i] if i < len(args.message) else '?'
            print(f"  '{char}' -> 0x{addr:04X} ({addr})")
    
    print(f"Encoding complete: {len(addresses)} addresses generated")
    return 0

def cmd_decode(args):
    """Handle decode command."""
    # Load ROM file
    try:
        with open(args.rom_file, 'rb') as f:
            rom_data = f.read()
        if args.verbose:
            print(f"Loaded ROM: {args.rom_file} ({len(rom_data)} bytes)")
    except FileNotFoundError:
        print(f"Error: ROM file '{args.rom_file}' not found", file=sys.stderr)
        return 1
    
    # Load address file
    try:
        addresses = ZOSCIIEncoder.load_addresses(args.address_file)
        if args.verbose:
            print(f"Loaded addresses: {args.address_file} ({len(addresses)} addresses)")
    except FileNotFoundError:
        print(f"Error: Address file '{args.address_file}' not found", file=sys.stderr)
        return 1
    
    # Create encoder for decoding
    memory_blocks = [{'start': 0, 'size': len(rom_data)}]
    encoder = ZOSCIIEncoder(rom_data, memory_blocks)
    
    # Decode message
    try:
        message = encoder.decode(addresses)
        if args.verbose:
            print(f"Decoded message: '{message}' ({len(message)} chars)")
    except Exception as e:
        print(f"Decoding failed: {e}", file=sys.stderr)
        return 1
    
    # Output result
    if args.output:
        try:
            with open(args.output, 'w') as f:
                f.write(message)
            print(f"Saved to: {args.output}")
        except Exception as e:
            print(f"Failed to save file: {e}", file=sys.stderr)
            return 1
    else:
        print(f"Decoded message: {message}")
    
    return 0

def cmd_info(args):
    """Handle info command to analyze ROM compatibility."""
    import json
    from collections import Counter
    
    # Load ROM file
    try:
        with open(args.rom_file, 'rb') as f:
            rom_data = f.read()
        print(f"ROM File: {args.rom_file}")
        print(f"Size: {len(rom_data)} bytes ({len(rom_data) / 1024:.1f} KB)")
    except FileNotFoundError:
        print(f"Error: ROM file '{args.rom_file}' not found", file=sys.stderr)
        return 1

    # Load memory blocks
    if args.memory_blocks:
        try:
            with open(args.memory_blocks, 'r') as f:
                memory_blocks = json.load(f)
        except FileNotFoundError:
            print(f"Error: Memory blocks file '{args.memory_blocks}' not found", file=sys.stderr)
            return 1
    else:
        memory_blocks = [{'start': 0, 'size': len(rom_data)}]
    
    print(f"\nMemory Blocks: {len(memory_blocks)}")
    total_valid_bytes = 0
    for i, block in enumerate(memory_blocks):
        print(f"  Block {i}: 0x{block['start']:04X}-0x{block['start'] + block['size'] - 1:04X} ({block['size']} bytes)")
        total_valid_bytes += block['size']
    
    print(f"Total valid memory: {total_valid_bytes} bytes ({total_valid_bytes / 1024:.1f} KB)")
    
    # Analyze byte distribution
    encoder = ZOSCIIEncoder(rom_data, memory_blocks)
    byte_counts = Counter()
    valid_addresses = 0
    
    for address in range(len(rom_data)):
        if encoder._is_valid_address(address):
            byte_value = rom_data[address]
            byte_counts[byte_value] += 1
            valid_addresses += 1
    
    print(f"\nByte Analysis:")
    print(f"Valid addresses: {valid_addresses}")
    print(f"Unique byte values: {len(byte_counts)}/256")
    print(f"Coverage: {len(byte_counts)/256*100:.1f}%")
    
    # Check ASCII printable range
    printable_ascii = set(range(32, 127))  # Space to ~
    available_ascii = set(byte_counts.keys()) & printable_ascii
    print(f"ASCII printable characters available: {len(available_ascii)}/95")
    
    missing_ascii = printable_ascii - set(byte_counts.keys())
    if missing_ascii:
        print(f"Missing ASCII characters: {sorted(missing_ascii)}")
        print(f"Missing chars: {''.join(chr(c) for c in sorted(missing_ascii))}")
    
    # Show most/least common bytes
    if byte_counts:
        most_common = byte_counts.most_common(5)
        least_common = byte_counts.most_common()[-5:]
        
        print(f"\nMost common bytes:")
        for byte_val, count in most_common:
            char = chr(byte_val) if 32 <= byte_val <= 126 else f"\\x{byte_val:02x}"
            print(f"  {byte_val:3d} ('{char}'): {count} occurrences")
        
        print(f"\nLeast common bytes:")
        for byte_val, count in reversed(least_common):
            char = chr(byte_val) if 32 <= byte_val <= 126 else f"\\x{byte_val:02x}"
            print(f"  {byte_val:3d} ('{char}'): {count} occurrences")
    
    # ZOSCII suitability assessment
    print(f"\nZOSCII Suitability Assessment:")
    if len(byte_counts) >= 200:
        print("✓ Excellent byte diversity (200+ unique values)")
    elif len(byte_counts) >= 150:
        print("✓ Good byte diversity (150+ unique values)")
    elif len(byte_counts) >= 100:
        print("⚠ Fair byte diversity (100+ unique values)")
    else:
        print("✗ Poor byte diversity (<100 unique values)")
    
    if len(available_ascii) >= 90:
        print("✓ Excellent ASCII coverage (90+ printable chars)")
    elif len(available_ascii) >= 70:
        print("✓ Good ASCII coverage (70+ printable chars)")
    elif len(available_ascii) >= 50:
        print("⚠ Fair ASCII coverage (50+ printable chars)")
    else:
        print("✗ Poor ASCII coverage (<50 printable chars)")
    
    # Entropy estimation
    if valid_addresses > 0:
        entropy = 0
        for count in byte_counts.values():
            if count > 0:
                p = count / valid_addresses
                entropy -= p * (p.bit_length() - 1) if p > 0 else 0
        print(f"Estimated entropy: {entropy:.2f} bits per byte")
        if entropy > 6:
            print("✓ High entropy - good for randomization")
        elif entropy > 4:
            print("✓ Medium entropy - adequate for randomization")
        else:
            print("⚠ Low entropy - limited randomization")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())