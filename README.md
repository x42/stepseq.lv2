stepseq.lv2 - Midi Step Sequencer
=================================

stepseq.lv2 is simple step sequencer for moddevices.com

Install
-------

Compiling stepseq requires the LV2 SDK, bash, gnu-make, and a c-compiler.

```bash
  git clone https://github.com/x42/stepseq.lv2.git
  cd stepseq.lv2
  make submodules
  make
  sudo make install PREFIX=/usr
```

Note to packagers: The Makefile honors `PREFIX` and `DESTDIR` variables as well
as `CFLAGS`, `LDFLAGS` and `OPTIMIZATIONS` (additions to `CFLAGS`), also
see the first 10 lines of the Makefile.

The number of steps and notes can be set at compile time using `N_NOTES`
and `N_STEPS` make variables. Both should be at least 4. The default is 8x8.
