/* Forced into every game file in U7_RESET_CHECK builds: the game's writable globals go into
 * sections of their own, so the reset check can compare them as a whole. */
#ifdef __APPLE__
#pragma clang section bss = "__DATA,__u7bss" data = "__DATA,__u7data"
#else
#pragma clang section bss = "u7bss" data = "u7data"
#endif
