# Compression Subsystem

`tessera::compress` is a pluggable codec layer plus a self-describing frame
envelope. It sits above the foundation and below the artifact modules; sections
may declare a codec so their payloads are stored compressed.

## Codecs

All codecs implement `compress::Codec` and are reachable as process-wide
singletons through `find_codec(CodecId)`.

| Codec | Id | Scheme |
|-------|----|--------|
| `StoreCodec` | 0 | Identity. `decompress` checks the stored length equals the declared size. |
| `RleCodec` | 1 | PackBits-style byte runs (literal runs ≤128, repeat runs ≤129). |
| `Lz77Codec` | 2 | LZSS sliding window, 12-bit distance (≤4096) / 4-bit length (3–18), tag-byte grouping. |
| `DeltaCodec` | 3 | Byte-delta filter followed by run-length coding. |

`decompress` takes the declared uncompressed size. Each codec pre-sizes its
output to that size, checks each token against the remaining size before
applying it, and returns an error if it does not produce exactly the declared
number of bytes.

## Frames

`compress_frame` / `decompress_frame` wrap a codec payload:

```
u32 magic | u8 codec | u8 flags | uvarint uncompressed | uvarint compressed | payload | u32 crc32c
```

The reader checks the magic, bounds both declared sizes against
`core::kMaxBlockSize`, verifies the CRC over the payload, and then invokes the
codec. `compress_frame` falls back to `Store` when a transform does not shrink
the input, so a frame is never larger than the raw bytes plus a small header.
