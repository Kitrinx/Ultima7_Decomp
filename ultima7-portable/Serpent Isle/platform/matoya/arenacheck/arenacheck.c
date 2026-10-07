/* Test builds only (U7_ARENA_CHECK): finds linear-memory accesses outside the arena.
 *
 * Every LINEAR() address is checked. One past the arena's end (or negative) is logged once per
 * call site and caller, and the access goes to a zeroed stand-in block so the game carries on.
 * One in the unused first 4 KB is logged and left alone, except 0, which the game converts
 * without using (an absent table).
 *
 * The arena is followed by 4 GB of inaccessible memory, so a pointer run past its end faults
 * instead of reading the heap. Any crash in these builds is logged with its callers and the
 * process exits quietly, so no crash report window opens.
 */
#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../backend.h"

#define LOW_AREA 0x1000
#define GUARD_SIZE (UINT64_C(4) << 30)
#define STAND_IN_SIZE 0x20000
#define MAX_LOGGED 512
#define CALLERS 10

extern uint8_t *LinearBase;
extern uint32_t LinearSize;

static uint8_t stand_in[STAND_IN_SIZE];
static void *logged[MAX_LOGGED][2];
static int logged_count;

static bool first_logging(void *site, void *caller)
{
	for (int i = 0; i < logged_count; i++) {
		if (logged[i][0] == site && logged[i][1] == caller)
			return false;
	}
	if (logged_count < MAX_LOGGED) {
		logged[logged_count][0] = site;
		logged[logged_count][1] = caller;
		logged_count++;
	}
	return true;
}

uint8_t *ArenaCheck(uint32_t address)
{
	bool outside = address >= LinearSize;

	if (outside || (address != 0 && address < LOW_AREA)) {
		/* frames[0] is here, frames[1] the LINEAR() site, then its callers. */
		void *frames[CALLERS + 2];
		int count = backtrace(frames, CALLERS + 2);

		if (count > 1 && first_logging(frames[1], count > 2 ? frames[2] : NULL)) {
			uintptr_t callers[CALLERS];

			for (int i = 2; i < count; i++)
				callers[i - 2] = (uintptr_t) frames[i];
			print_access(outside ? "linear address past the arena" : "linear address in the unused first 4 KB",
				(uintptr_t) frames[1], address, callers, count > 2 ? count - 2 : 0);
		}
		if (outside) {
			memset(stand_in, 0, 64);
			return stand_in;
		}
	}
	return LinearBase + address;
}

uint8_t *ArenaCheckAllocate(uint32_t size)
{
	uint8_t *p = mmap(NULL, size + GUARD_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);

	if (p == MAP_FAILED || mprotect(p, size, PROT_READ | PROT_WRITE) != 0)
		return NULL;
	return p;
}

void arena_check_crash(int sig, uintptr_t pc, uintptr_t address, const uintptr_t *callers, int count)
{
	uintptr_t base = (uintptr_t) LinearBase;
	char what[96];

	if (LinearBase != NULL && address >= base + LinearSize && address - base < LinearSize + GUARD_SIZE)
		snprintf(what, sizeof what, "crash (signal %d) past the arena, linear %#lx", sig,
			(unsigned long) (address - base));
	else
		snprintf(what, sizeof what, "crash (signal %d)", sig);
	print_access(what, pc, address, callers, count);
	_exit(128 + sig);
}

/* Names the callers of a fatal game error. */
void arena_check_trace(const char *what)
{
	void *frames[CALLERS + 2];
	uintptr_t callers[CALLERS];
	int count = backtrace(frames, CALLERS + 2), n = 0;

	for (int i = 2; i < count; i++)
		callers[n++] = (uintptr_t) frames[i];
	print_access(what, count > 1 ? (uintptr_t) frames[1] : 0, 0, callers, n);
}

/* Aborts and traps: the same log and quiet exit. Faults come through nullpage.c. */
static void on_signal(int sig)
{
	void *frames[CALLERS + 3];
	uintptr_t callers[CALLERS];
	int count = backtrace(frames, CALLERS + 3), n = 0;

	for (int i = 3; i < count; i++)
		callers[n++] = (uintptr_t) frames[i];
	arena_check_crash(sig, count > 2 ? (uintptr_t) frames[2] : 0, 0, callers, n);
}

__attribute__((constructor)) static void install(void)
{
	signal(SIGABRT, on_signal);
	signal(SIGILL, on_signal);
	signal(SIGFPE, on_signal);
#ifndef __x86_64__
	signal(SIGTRAP, on_signal);
#endif
}
