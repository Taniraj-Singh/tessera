# Archive Subsystem

`tessera::archive` implements the Tessera Binary Archive (TBA) — a
container-of-containers. It bundles several TBC containers, each stored as a
`compress` frame, behind a directory that records each frame's location and a
checksum of its decompressed contents.

## Layout

```
[ fixed header (28 bytes) ] [ directory block ] [ frame region ]
```

**Header:** magic, version, flags, entry count, directory length, directory
CRC32C, creation timestamp.

**Directory entry:** id, name, `frame_offset`, `frame_length`,
`content_crc32c` (CRC of the *decompressed* container).

**Frame region:** one `compress::compress_frame` per entry, each wrapping a
canonical TBC container.

## Read path — depth by construction

`ArchiveReader::open` validates the header and directory eagerly (magic,
entry-count bound, directory CRC, and per-entry frame bounds with overflow
checks). `read_entry`/`parse_entry` then lazily walk the full chain:

```
archive header → directory entry → frame header → codec decompress
              → container header → catalog → schema → record batch
```

No single function reveals that path — reconstructing a valid input requires
composing every stage, which is exactly the property the difficulty bar rewards.

## Write path

`ArchiveWriter::add_container` serializes a container to canonical bytes, frames
it under the chosen codec, and records the content CRC. `build()` computes the
directory length, assigns frame offsets after it, writes the directory CRC, and
concatenates header + directory + frames.

## Fuzzing

`fuzz/archive_fuzzer.cpp` opens an arbitrary buffer as an archive and parses
every entry, exercising the deepest reachable logic in the SDK from one input.
