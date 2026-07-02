# Lifecycle Subsystems

State-management support around the container model.

## Transactions (`tessera::txn`)

`Transaction` wraps a base container and stages edits (`add_section`,
`remove_section`, `rename_section`) on a private working copy, keeping the
catalog and header count in sync after each op. `commit()` runs semantic
validation and returns the edited container only if it is sound; `rollback()`
restores the original snapshot. The caller's base is never mutated in place, so a
rejected commit leaves no partial state.

## Recovery (`tessera::recovery`)

Best-effort repair of a damaged container model:

- `drop_unsalvageable` — removes sections with invalid names or rows whose field
  count disagrees with the schema;
- `realign_section_ids` — forces schema/record ids to match the catalog id;
- `trim_row_counts` — corrects declared counts to the actual row vectors;
- `rebuild_catalog` — regenerates the catalog and header count.

`repair()` runs the sequence and returns a `RepairReport` of what changed; the
result should pass structural validation.

## Resource management (`tessera::resource`)

`ResourceManager` tracks byte and handle usage against a `Budget` (0 = unbounded)
and fails `acquire` closed — with an overflow guard — when a request would exceed
either limit. `Lease` is a move-only RAII reservation that returns its bytes to
the manager exactly once on destruction.
