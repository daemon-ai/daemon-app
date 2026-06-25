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

The source of truth is the contract in the `daemon-node` repo (the `daemon-node/` submodule of the
superproject), not these files. Do not edit `generated/` by hand. The superproject owns the sync, the
Nix-idiomatic way:

```
# from the superproject root
nix build .#daemon-zcbor-codec     # pure derivation: zcbor-smoke.cddl -> generated C/H in the store
nix flake check                    # checks.codec-drift fails if generated/ here is stale
nix run .#update-codec             # the one impure step: copy the store output into generated/
```

Under the hood that runs `daemon-node`'s single canonical `crates/contracts/daemon-api/zcbor-codegen.sh`
(also exposed as `cargo run -p xtask -- gen-zcbor` inside the daemon-node flake shell). The
`daemon-api` contract is proven byte-compatible with this codec by `cargo run -p xtask -- verify-codec`
(daemon-node `checks.verify-codec`).

Refresh `vendor/` from the same `zcbor` package only if its version changes. The codegen subset
currently covers `zcbor-smoke.cddl`; growing it toward the full `daemon-api.cddl` requires quoting the
CDDL's bareword map keys (zcbor needs `"key":`), resolving its `any` members, and working around
zcbor's helper-name collisions on unions of single-key maps (e.g. `origin-scope`).
