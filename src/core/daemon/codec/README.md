# Vendored NodeApi CBOR codec

This directory vendors the CBOR codec for the daemon NodeApi wire contract. Nothing here is
hand-written: GUI/TUI view models never see CBOR, and the client must not hand-roll Qt CBOR.

## Layout

- `vendor/` - the zcbor C runtime (`zcbor_common`, `zcbor_decode`, `zcbor_encode`, `zcbor_print`),
  copied verbatim from the `zcbor` 0.9.1 Python package shipped in the `../daemon` flake.
- `generated/` - `zcbor`-generated encode/decode/types for the daemon-api client subset
  (`daemon_api_client_*`). Entry types are `api-request` / `api-response`.

The C++ facade over this codec is [`../node_api_codec.h`](../node_api_codec.h); it exercises the
daemon slice the client currently uses (Health / SessionsQuery / Subscribe requests, and decoding of
Health / SessionPage / LogPage responses).

## Regeneration

The source of truth is the contract in the `daemon-node` repo (the `daemon-node/` submodule of the
superproject), not these files. Do not edit `generated/` by hand. The superproject owns the sync, the
Nix-idiomatic way:

```
# from the superproject root (submodule contents are gitlinks, hence ?submodules=1)
nix build '.?submodules=1#daemon-zcbor-codec'   # pure: daemon-api.cddl -> generated C/H in the store
nix build '.?submodules=1#checks.<system>.codec-drift'   # fails if generated/ here is stale
nix run '.?submodules=1#update-codec'           # the one impure step: copy the store output into the tree
```

Under the hood that runs `daemon-node`'s single canonical `crates/contracts/daemon-api/zcbor-codegen.sh`
(also exposed as `cargo run -p xtask -- gen-zcbor` inside the daemon-node flake shell). The
`daemon-api` contract is proven byte-compatible with this codec by `cargo run -p xtask -- verify-codec`
(daemon-node `checks.verify-codec`).

Refresh `vendor/` from the same `zcbor` package only if its version changes. The codec is generated
from the single authoritative `daemon-api.cddl` (full surface). That file is authored in zcbor
dialect: quoted map keys (`"key":`), every union arm is a named rule, same-type tuples are labeled
(`[k: tstr, v: tstr]`), opaque fields are `any`, and a few type rules carry a `-t` suffix to avoid
zcbor deriving a coder name that collides with a same-named map member (e.g. `origin-scope` vs map
`origin`.`scope`). Rule names are local and never on the wire, so these are encoding-invariant.
