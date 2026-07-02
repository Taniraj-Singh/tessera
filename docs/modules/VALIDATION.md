# Validation Subsystem

Three layers of validation, increasing in depth:

## 1. Structural — `validate::validate_model` / `validate_bytes`

The original pass: magic, header/catalog section-count agreement, per-section id
agreement, declared row counts, and an optional serialize→parse round trip.

## 2. Semantic — `validate::validate_semantics`

Value-level checks over an in-memory container, using `reflect::FieldRef` and the
`schema::compile` layout:

- every value's variant alternative agrees with its declared field type;
- `Utf8`/`Bytes` honor their length caps and UTF-8 is well-formed;
- row/field counts agree per section;
- section ids and names are unique (via `HashIndex`/`StringIndex`).

Because it inspects decoded values rather than the wire form, it catches defects
in programmatically constructed containers *before* serialization — cases the
structural pass cannot see.

## 3. Constraint — `validate::check_constraints`

Declarative, out-of-band rules a caller supplies (`Constraint{section, field,
kind, bound}`):

| Kind | Meaning |
|------|---------|
| `NotEmpty` | aggregate field non-empty in every row |
| `Unique` | field value distinct across rows (numeric via `HashIndex`, string via `StringIndex`) |
| `MinValue` / `MaxValue` | numeric field within a bound |
| `Monotonic` | numeric field non-decreasing across rows |

A rule naming a missing section or field is itself a violation. All three passes
return the shared `ValidationReport { valid, diagnostics }`.
