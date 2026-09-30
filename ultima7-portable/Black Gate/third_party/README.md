# Vendored dependencies

Plain source copies at fixed versions, taken with `git archive` (no build outputs, no git
metadata). Update them only on purpose, and record the new version here.

| Library | Upstream | Version | License | What is included |
| --- | --- | --- | --- | --- |
| libmatoya | https://github.com/snowcone-ltd/libmatoya | commit `952f2a22bdb4cf832db110c00aa3e57952358eae` | MIT (`libmatoya/LICENSE`) | the whole repository |
| Munt mt32emu | https://github.com/munt/munt | tag `libmt32emu_2_8_3`, commit `3b05ec276f9e605af86b0eaef7f5eda43477a31f` | LGPL 2.1 or later (`munt/mt32emu/COPYING.LESSER.txt`) | the `mt32emu` library only |

To refresh one from a checkout of its upstream repository:

```sh
rm -rf third_party/libmatoya && mkdir third_party/libmatoya
git -C /path/to/libmatoya archive <commit> | tar -x -C third_party/libmatoya

rm -rf third_party/munt && mkdir third_party/munt
git -C /path/to/munt archive <commit> mt32emu | tar -x -C third_party/munt
```

libmatoya writes its build output to `libmatoya/bin/`, which is ignored.
