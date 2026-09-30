/* Reads and writes through a null pointer, as the DOS game made them.
 *
 * A null near pointer read the start of the data segment: four zero bytes, then Borland's
 * copyright text. The game relies on that in places (an empty string, a table not yet loaded).
 * Here the first 64 KB cannot be mapped, so a fault there is served from a stand-in segment
 * of the same shape and the program carries on. Each place it happens is logged once.
 *
 * Only arm64 macOS is handled so far; elsewhere such an access still crashes.
 */
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "backend.h"

#if defined(__APPLE__) && defined(__aarch64__)

#include <dlfcn.h>
#include <mach/thread_status.h>
#include <sys/ucontext.h>

#define SEGMENT_SIZE 0x10000
#define MAX_LOGGED 128

static uint8_t segment[SEGMENT_SIZE] =
	"\0\0\0\0Borland C++ - Copyright 1991 Borland Intl.";
static uintptr_t logged[MAX_LOGGED];
static int logged_count;

typedef struct __darwin_arm_thread_state64 cpu_state;
typedef struct __darwin_arm_neon_state64 simd_state;

static uint64_t get_x(cpu_state *ss, unsigned n, bool sp)
{
	if (n == 31)
		return sp ? (uint64_t) arm_thread_state64_get_sp(*ss) : 0;
	if (n == 29)
		return (uint64_t) arm_thread_state64_get_fp(*ss);
	if (n == 30)
		return (uint64_t) arm_thread_state64_get_lr(*ss);
	return ss->__x[n];
}

static void set_x(cpu_state *ss, unsigned n, uint64_t value, bool sp)
{
	if (n == 31) {
		if (sp)
			arm_thread_state64_set_sp(*ss, value);
	} else if (n == 29) {
		arm_thread_state64_set_fp(*ss, value);
	} else if (n == 30) {
		arm_thread_state64_set_lr_fptr(*ss, (void *) value);
	} else {
		ss->__x[n] = value;
	}
}

static void access_segment(uint64_t address, uint8_t *bytes, unsigned n, bool store)
{
	for (unsigned i = 0; i < n; i++) {
		uint64_t a = address + i;

		if (a >= SEGMENT_SIZE) {
			if (!store)
				bytes[i] = 0;
		} else if (store) {
			segment[a] = bytes[i];
		} else {
			bytes[i] = segment[a];
		}
	}
}

static uint64_t read_value(const uint8_t *bytes, unsigned n)
{
	uint64_t v = 0;

	for (unsigned i = 0; i < n && i < 8; i++)
		v |= (uint64_t) bytes[i] << (8 * i);
	return v;
}

static uint64_t sign_extend(uint64_t v, unsigned bits)
{
	uint64_t m = 1ull << (bits - 1);

	return (v ^ m) - m;
}

/* One register's worth: a general register (bytes 1-8, loads optionally signed into 32 or 64
 * bits) or a SIMD register (bytes 1-16). */
static void transfer(cpu_state *ss, simd_state *ns, bool simd, unsigned rt, uint64_t address,
	unsigned bytes, bool load, int sign_into)
{
	uint8_t buffer[16] = {0};

	if (load) {
		access_segment(address, buffer, bytes, false);
		if (simd) {
			__uint128_t v = 0;
			memcpy(&v, buffer, bytes);
			ns->__v[rt] = v;
		} else {
			uint64_t v = read_value(buffer, bytes);
			if (sign_into)
				v = sign_extend(v, bytes * 8);
			if (sign_into == 32 || (!sign_into && bytes < 8))
				v &= 0xffffffffu;
			set_x(ss, rt, v, false);
		}
	} else {
		if (simd) {
			memcpy(buffer, &ns->__v[rt], bytes);
		} else {
			uint64_t v = get_x(ss, rt, false);
			memcpy(buffer, &v, bytes);
		}
		access_segment(address, buffer, bytes, true);
	}
}

static uint64_t extend_register(uint64_t v, unsigned option)
{
	switch (option) {
	case 2: return (uint32_t) v;                          /* UXTW */
	case 6: return (uint64_t) (int64_t) (int32_t) v;      /* SXTW */
	default: return v;                                     /* LSL, SXTX */
	}
}

/* Performs one A64 load or store against the stand-in segment. False when not understood. */
static bool emulate(cpu_state *ss, simd_state *ns, uint32_t insn)
{
	unsigned rt = insn & 31, rn = (insn >> 5) & 31;

	if ((insn & 0x3a000000) == 0x28000000) {
		/* Load/store pair. */
		unsigned opc = insn >> 30, type = (insn >> 23) & 3;
		bool simd = (insn >> 26) & 1, load = (insn >> 22) & 1;
		unsigned rt2 = (insn >> 10) & 31, bytes;
		int sign_into = 0;
		int64_t offset = sign_extend((insn >> 15) & 0x7f, 7);
		uint64_t base = get_x(ss, rn, true), address;

		if (simd)
			bytes = opc == 0 ? 4 : opc == 1 ? 8 : 16;
		else if (opc == 1)
			bytes = 4, sign_into = 64;
		else
			bytes = opc == 0 ? 4 : 8;
		offset *= (int64_t) bytes;
		address = type == 1 ? base : base + (uint64_t) offset;
		transfer(ss, ns, simd, rt, address, bytes, load, sign_into);
		transfer(ss, ns, simd, rt2, address + bytes, bytes, load, sign_into);
		if (type == 1 || type == 3)
			set_x(ss, rn, base + (uint64_t) offset, true);
		return true;
	}

	if ((insn & 0x3a000000) == 0x38000000) {
		/* Load/store one register. */
		unsigned size = insn >> 30, opc = (insn >> 22) & 3;
		bool simd = (insn >> 26) & 1, load, unsigned_offset = (insn >> 24) & 1;
		unsigned bytes, scale;
		int sign_into = 0;
		uint64_t base = get_x(ss, rn, true), address;
		bool writeback = false;
		int64_t imm = 0;

		if (simd) {
			scale = (opc & 2) && size == 0 ? 4 : size;
			bytes = 1u << scale;
			load = opc & 1;
		} else {
			scale = size;
			bytes = 1u << size;
			load = opc != 0;
			if (opc == 2 && size == 3)
				return true;                                       /* PRFM */
			if (opc == 2)
				sign_into = 64;
			else if (opc == 3)
				sign_into = 32;
		}

		if (unsigned_offset) {
			address = base + ((uint64_t) ((insn >> 10) & 0xfff) << scale);
		} else if ((insn >> 21) & 1) {
			if (((insn >> 10) & 3) != 2)
				return false;
			unsigned rm = (insn >> 16) & 31, option = (insn >> 13) & 7;
			bool shift = (insn >> 12) & 1;
			address = base + (extend_register(get_x(ss, rm, false), option) << (shift ? scale : 0));
		} else {
			unsigned mode = (insn >> 10) & 3;
			imm = sign_extend((insn >> 12) & 0x1ff, 9);
			address = mode == 1 ? base : base + (uint64_t) imm;
			writeback = mode == 1 || mode == 3;
		}
		transfer(ss, ns, simd, rt, address, bytes, load, sign_into);
		if (writeback)
			set_x(ss, rn, base + (uint64_t) imm, true);
		return true;
	}
	return false;
}

static void log_once(uintptr_t pc, uintptr_t address)
{
	Dl_info info;
	char text[256];

	for (int i = 0; i < logged_count; i++) {
		if (logged[i] == pc)
			return;
	}
	if (logged_count < MAX_LOGGED)
		logged[logged_count++] = pc;
	if (dladdr((void *) pc, &info) && info.dli_sname != NULL)
		snprintf(text, sizeof text, "null access at %s+%lu (address %#lx)\n", info.dli_sname,
			(unsigned long) (pc - (uintptr_t) info.dli_saddr), (unsigned long) address);
	else
		snprintf(text, sizeof text, "null access at %#lx (address %#lx)\n", (unsigned long) pc,
			(unsigned long) address);
	fputs(text, stderr);
}

static void on_fault(int sig, siginfo_t *info, void *context)
{
	ucontext_t *uc = context;
	uintptr_t address = (uintptr_t) info->si_addr;
	cpu_state *ss = &uc->uc_mcontext->__ss;
	uintptr_t pc = (uintptr_t) arm_thread_state64_get_pc(*ss);

	if (address < SEGMENT_SIZE && emulate(ss, &uc->uc_mcontext->__ns, *(const uint32_t *) pc)) {
		log_once(pc, address);
		arm_thread_state64_set_pc_fptr(*ss, (void *) (pc + 4));
		return;
	}
	/* Anything else is a real crash: let it happen as usual. */
	signal(sig, SIG_DFL);
}

void nullpage_install(void)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof sa);
	sa.sa_sigaction = on_fault;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGBUS, &sa, NULL);
}

#else

void nullpage_install(void)
{
}

#endif
