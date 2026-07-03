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

## Read path

`ArchiveReader::open` parses the header and directory (magic, entry-count bound,
directory CRC, and per-entry frame offset/length range checks). `read_entry` and
`parse_entry` then decompress a frame and parse the container it wraps:

```
archive header → directory entry → frame header → codec decompress
              → container header → catalog → schema → record batch
```

## Write path

`ArchiveWriter::add_container` serializes a container to canonical bytes, frames
it under the chosen codec, and records the content CRC. `build()` computes the
directory length, assigns frame offsets after it, writes the directory CRC, and
concatenates header + directory + frames.
