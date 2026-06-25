# Vendored NodeApi CBOR codec

This directory vendors the CBOR codec for the daemon NodeApi wire contract. Nothing here is
hand-written: GUI/TUI view models never see CBOR, and the client must not hand-roll Qt CBOR.

## Layout

- `vendor/` - the zcbor C runtime (`zcbor_common`, `zcbor_decode`, `zcbor_encode`, `zcbor_print`),
  copied verbatim from the `zcbor` 0.9.1 Python package shipped in the `../daemon` flake.
- `generated/` - `zcbor`-generated encode/decode/types for the daemon-api subset. Entry types are
  `api-request` / `api-response`.

The C++ facade over this codec is [`../node_api_codec.h`](../node_api_codec.h); it currently
exercises only the first daemon slice (Health request, SessionsQuery request, and decoding of
Health / SessionPage responses).

## Regeneration

The source of truth is the contract in the daemon repo, not these files. Regenerate from there:

```
# in ../daemon (inside the flake dev shell, which provides zcbor)
cargo run -p xtask -- zcbor-spike
```

That writes `target/zcbor-spike/{src,include}/daemon_api_smoke_*`. Copy the generated `*.c`/`*.h`
into `generated/`, and refresh `vendor/` from the same `zcbor` package if its version changes.
The codegen subset currently covers `zcbor-smoke.cddl`; growing it to the full `daemon-api.cddl`
(e.g. the command surface) requires concrete DTOs in that CDDL first.
