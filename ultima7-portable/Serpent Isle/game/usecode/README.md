# Usecode

Retail Serpent Isle + Silver Seed. From the repository root, the strict check is:

    python3 .agents/tools/si-reconstruction/usecode_check.py

Build its compiler with `make -C ultima7-usecode` first. The check builds a private source copy,
compares all three files with pinned retail originals, and checks that retained source stayed unchanged.
All 1,237 functions use readable source; no `#bytes` or `#linear` records remain.

The `.use` files here compile in the order `usecode.lnk` gives, against the names in
`usecode.uh`, as Origin's were. To build:

    u7ucproj project.u7p <directory>

That writes `USECODE`, `LINKDEP1` and `LINKDEP2`; copy all three into the game's `STATIC`
folder. With `--check` the build also fails unless `USECODE` matches the shipped file.
Direct builds rewrite `usecode.map`; use the strict check for preserved-source acceptance.

When changing it:

- A usable's number comes from its `usableindex` line in `usecode.uh`. A routine keeps the
  number `usecode.map` holds for its name; a new routine takes the next free number, and
  the build adds it to the map. A renamed routine is a new one, unless `#id(0x...)` after
  its name keeps the old number.
- A new file is added to `usecode.lnk`.
- The build puts any function a call names in the caller's link table, sizes the locals to
  the slots used, and refuses a call with the wrong argument count or one whose result is
  used when there is none, or left unused when there is one.
- To run a function the engine loads it with everything it can call: 35 functions and
  65,389 bytes at most. The build refuses anything past that, and writes nothing.
- `preserve_calls "OpenTrial_i"` retains its shipped call signature defects. This caller
  requires the complete expected retail USECODE hash before any output or map is written.
  Other callers retain signature checks. Editing this project requires removing that policy
  and correcting the calls.
