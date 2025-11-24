// Cyborg ZOSCII v20250817
// (c) 2025 Cyborg Unicorn Pty Ltd.
// This software is released under MIT License.

// Example minimal (not optimised) JavaScript Encoder/Decoder

rom = "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ";
encode = (r,m) => [...m].map(c => [...r].map((b,i)=>b==c?i:[]).flat().sort(()=>Math.random()-.5)[0]);
decode = (r,a) => a.map(a => r[a]).join('');
e = encode(rom, "HELLO");
console.log(e);
console.log(decode(rom,e));



