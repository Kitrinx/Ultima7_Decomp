# Building U7STRAX.DRV

## Requirements

- Turbo Assembler 2.51 and TLINK (5.0 used; any TLINK with `/t`), on the `PATH`.
- Borland MAKE (3.5 used) and DOS `COPY`, or DOSBox-X.

The Borland tools are commercial. Supply your own copy; never commit it.

## Build

```
make
```

Each `.ASM` assembles and links to a `.COM` image; `COPY /B` joins the header and the two records
into `U7STRAX.DRV`.

## Output

`U7STRAX.DRV` equals the shipped file: 5,379 bytes, SHA-256
`d31f1697e91b8e13c70b9b94ccd14bcac1ae00280589ff1f7f431f0ae5620393`. Each record also equals its
slice of the shipped file: `MT32DRV.COM` 1,207 bytes at 0x90, `ADLIBDRV.COM` 4,028 bytes at 0x547.

Checked 2026-10-10 under DOSBox-X (TASM 2.51, TLINK 5.0, MAKE 3.5): identical.
