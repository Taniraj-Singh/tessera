# Encoding Subsystem

`tessera::encoding` supplies the compact variable-length integer codecs used by
the framed formats (compression frames, archive entries, incremental pipeline
metadata). It reads through `buffer::Reader` and writes into
`memory::ByteBuffer`, inheriting their bounds discipline.

## LEB128 varints

Unsigned values are stored little-endian in 7-bit groups, high bit set on every
non-terminal byte:

```
0x00        -> 0
0xAC 0x02   -> 300
0xFF … 0x01 -> 2^64 - 1   (10 bytes)
```

`read_uvarint64` caps the decode at `kMaxVarint64Bytes` (10) groups and rejects:

- a stream that ends without a terminating byte (`InvalidLength`, "missing
  terminator");
- a value whose high group carries bits that do not fit in 64 (`InvalidLength`,
  "overflows 64 bits").

`read_uvarint32` additionally rejects values above `0xFFFFFFFF`. Because framed
lengths are attacker-controlled, an over-long or non-terminating varint must be
a bounded error, never an unbounded loop or an oversized allocation.

## ZigZag

`encoding/zigzag.hpp` maps signed integers so small negatives encode compactly:
`0, -1, 1, -2, 2 -> 0, 1, 2, 3, 4`. `write_svarint*` composes zigzag with the
unsigned varint writer; `read_svarint*` reverses it.

## Sizing

`uvarint_size(value)` returns the byte count `write_uvarint64` would emit, used
to reserve exact header space before a payload's extent is known.
