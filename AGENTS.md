# Guidance for AI coding assistants

This file is for other AI coding assistants and automated tools working in this
repository. Humans should read `CONTRIBUTING.md`.

## Attribution of AI assistance

OpenAI / other model-generated changes must be disclosed in every commit. Use
standard git trailers by appending to the commit message:

```
Co-authored-by: <Assistant Name> <email>
```

Use the identity of the specific assistant that produced the change. For example:

```
Co-authored-by: DeepSeek-V4 Flash <ai@deepseek.com>
```

Rules:

- Always disclose AI assistance; never strip an existing `Co-authored-by:`
  trailer added for you or by other tools.
- Name the model/assistant that actually wrote the code, not a generic one.
- If several tools assisted, add one trailer per tool.
- If you only helped debug/consult but wrote no committed code, you do not need a
  trailer, but you may note it in the commit body.

## Commit style

Follow the repository's existing convention (see recent `git log`):

- Imperative, lowercase subject, optionally `[area]:`, e.g. `fix:`, `[feature]`, `[gl]`.
- Short subject line; additional context goes in the body.
- Use 4-space indentation in code; keep style consistent with surrounding code.

## Building / testing

- The repo uses a Nix flake (`flake.nix`) via direnv; enter the shell with `direnv allow`.
- A CMake build exists under `build/`; rebuild with:
  `cmake --build build`
- Do not assume the host Mesa/OpenGL is available; rely on the bundled Mesa the
  flake provides (see `flake.nix`), especially for Anything EGL/OpenGL.