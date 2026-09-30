/* Reads and writes through a null pointer, as the DOS game made them.
 *
 * A null near pointer read the start of the data segment: four zero bytes, then Borland's
 * copyright text. The game relies on that in places (an empty string, a table not yet loaded).
 * Here the first 64 KB cannot be mapped, so a fault there is served from a stand-in segment
 * of the same shape and the program carries on. Each place it happens is logged once.
 *
 * arm64 macOS emulates the faulting load or store. x64 Windows and Linux point the
 * instruction's address register at the segment, run the one instruction, and put the
 * register back. Elsewhere such an access still crashes.
 */
#ifdef __linux__
#define _GNU_SOURCE                             /* register names in ucontext.h */
#endif
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "backend.h"

#if (defined(_WIN32) && defined(_M_X64)) || (defined(__linux__) && defined(__x86_64__))
#define X64_HANDLER 1
#else
#define X64_HANDLER 0
#endif

#if (defined(__APPLE__) && defined(__aarch64__)) || X64_HANDLER

#define SEGMENT_SIZE 0x10000
#define MAX_LOGGED 128

#define SEGMENT_START "\0\0\0\0Borland C++ - Copyright 1991 Borland Intl."

/* Padded, so a wide read that starts near the end stays inside. */
static uint8_t segment[SEGMENT_SIZE + 64] = SEGMENT_START;
static uintptr_t logged[MAX_LOGGED];
static int logged_count;

/* The game can write through a null pointer too; each program starts with a clean segment. */
void nullpage_reset(void)
{
	memset(segment, 0, sizeof segment);
	memcpy(segment, SEGMENT_START, sizeof SEGMENT_START);
}

#else

void nullpage_reset(void)
{
}

#endif

#if defined(__APPLE__) && defined(__aarch64__)

#include <dlfcn.h>
#include <mach/thread_status.h>
#include <sys/ucontext.h>

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

#elif X64_HANDLER

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>

typedef CONTEXT cpu_context;
#else
#include <dlfcn.h>
#include <ucontext.h>

typedef mcontext_t cpu_context;
#endif

#define NO_REG (-1)
#define TRAP_FLAG 0x100

/* What the handler needs to know about one x64 instruction. Registers are numbered as the
 * encoding numbers them: rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi, r8 to r15. */
typedef struct {
	int base, index, scale;
	bool string;          /* movs, cmps, stos, lods, scas: addresses in rsi and rdi */
	int reg;              /* the ModRM reg operand when it is a general register */
	bool pure_load;       /* reg is only written, with the loaded value */
	int size;             /* reg's size in bytes */
	bool high_byte;       /* reg is ah, ch, dh or bh */
	bool writes_rax_rdx;  /* mul, div, cmpxchg */
} insn_info;

/* The instruction being stepped on this thread, with the registers to put back after. */
static _Thread_local struct {
	bool armed;
	int count;
	int regs[2];
	uint64_t original[2];
	insn_info info;
} pending;

static bool one_byte_has_modrm(unsigned op)
{
	if (op < 0x40)
		return (op & 7) < 4;
	return op == 0x63 || op == 0x69 || op == 0x6b || (op >= 0x80 && op <= 0x8f) ||
		op == 0xc0 || op == 0xc1 || op == 0xc6 || op == 0xc7 || (op >= 0xd0 && op <= 0xd3) ||
		(op >= 0xd8 && op <= 0xdf) || op == 0xf6 || op == 0xf7 || op == 0xfe || op == 0xff;
}

static bool two_byte_has_modrm(unsigned op)
{
	return !((op >= 0x05 && op <= 0x09) || op == 0x0b || op == 0x0e || (op >= 0x30 && op <= 0x37) ||
		op == 0x77 || (op >= 0x80 && op <= 0x8f) || (op >= 0xa0 && op <= 0xa2) ||
		(op >= 0xa8 && op <= 0xaa) || (op >= 0xc8 && op <= 0xcf));
}

/* Finds the memory operand of the instruction at p. False when it is not understood. */
static bool decode(const uint8_t *p, insn_info *in)
{
	bool size16 = false, vex = false;
	unsigned rex = 0, map, op, modrm, mod, rm, regf;

	memset(in, 0, sizeof *in);
	in->base = in->index = in->reg = NO_REG;
	in->scale = 1;
	for (;; p++) {
		if (*p == 0x66)
			size16 = true;
		else if (*p == 0x67 || *p == 0x64 || *p == 0x65)
			return false;                      /* 32-bit addresses, fs, gs */
		else if (*p != 0xf0 && *p != 0xf2 && *p != 0xf3 && *p != 0x26 && *p != 0x2e &&
			*p != 0x36 && *p != 0x3e)
			break;
	}
	if ((*p & 0xf0) == 0x40)
		rex = *p++;                                /* 0100WRXB */
	if (*p == 0xc5) {
		rex = (p[1] & 0x80) ? 0 : 4;
		map = 1, op = p[2], p += 3, vex = true;
	} else if (*p == 0xc4) {
		rex = ((~p[1] >> 5) & 7) | ((p[2] & 0x80) ? 8 : 0);
		map = p[1] & 0x1f, op = p[3], p += 4, vex = true;
		if (map < 1 || map > 3)
			return false;
	} else if (*p == 0x62) {
		/* EVEX (AVX-512, as in glibc's string functions): R, X, B inverted as in VEX. The
		 * displacement is scaled, but only the registers matter here. */
		rex = ((~p[1] >> 5) & 7) | ((p[2] & 0x80) ? 8 : 0);
		map = p[1] & 7, op = p[4], p += 5, vex = true;
		if (map == 0 || map == 4)
			return false;
	} else if (*p == 0x0f) {
		if (p[1] == 0x38 || p[1] == 0x3a)
			map = p[1] == 0x38 ? 2 : 3, op = p[2], p += 3;
		else
			map = 1, op = p[1], p += 2;
	} else {
		map = 0, op = *p++;
	}

	if (map == 0 && ((op >= 0xa4 && op <= 0xa7) || (op >= 0xaa && op <= 0xaf))) {
		in->string = true;
		return true;
	}
	if (!vex && ((map == 0 && !one_byte_has_modrm(op)) || (map == 1 && !two_byte_has_modrm(op))))
		return false;

	modrm = *p++;
	mod = modrm >> 6, rm = modrm & 7;
	regf = ((modrm >> 3) & 7) | ((rex & 4) ? 8 : 0);
	if (mod == 3)
		return false;
	if (rm == 4) {
		unsigned sib = *p++, index = ((sib >> 3) & 7) | ((rex & 2) ? 8 : 0);

		in->scale = 1 << (sib >> 6);
		if (index != 4)
			in->index = (int) index;
		if (!((sib & 7) == 5 && mod == 0))
			in->base = (int) ((sib & 7) | ((rex & 1) ? 8 : 0));
	} else if (rm == 5 && mod == 0) {
		return false;                              /* rip-relative */
	} else {
		in->base = (int) (rm | ((rex & 1) ? 8 : 0));
	}

	/* Where the reg field names a general register, note it: it may be the one moved. */
	in->size = (rex & 8) ? 8 : size16 ? 2 : 4;
	if (vex) {
		if (map == 2 && op >= 0xf0)
			in->reg = (int) regf;                  /* BMI */
	} else if (map == 0) {
		if (op < 0x40 || op == 0x63 || op == 0x69 || op == 0x6b || (op >= 0x84 && op <= 0x8b)) {
			in->reg = (int) regf;
			in->pure_load = op == 0x8a || op == 0x8b || op == 0x63;
			if (op < 0x40 ? !(op & 1) : (op == 0x84 || op == 0x86 || op == 0x88 || op == 0x8a))
				in->size = 1;
		} else if ((op == 0xf6 || op == 0xf7) && (regf & 7) >= 4) {
			in->writes_rax_rdx = true;
		}
	} else if (map == 1) {
		if ((op >= 0x40 && op <= 0x4f) || op == 0xa3 || op == 0xa4 || op == 0xa5 || op == 0xab ||
			op == 0xac || op == 0xad || op == 0xaf || (op >= 0xb0 && op <= 0xb3) ||
			(op >= 0xb6 && op <= 0xc1 && op != 0xb9 && op != 0xba)) {
			in->reg = (int) regf;
			in->pure_load = op == 0xb6 || op == 0xb7 || op == 0xbe || op == 0xbf;
			if (op == 0xb0 || op == 0xc0)
				in->size = 1;
			in->writes_rax_rdx = op == 0xb0 || op == 0xb1;
		}
	} else if (map == 2 && (op == 0xf0 || op == 0xf1)) {
		in->reg = (int) regf;                      /* movbe, crc32 */
	}
	if (in->size == 1 && in->reg >= 4 && in->reg < 8 && !rex) {
		in->reg -= 4;
		in->high_byte = true;
	}
	return true;
}

/* ---- Registers, in the encoding's numbering ---- */

#ifdef _WIN32
static uint64_t *gpr(cpu_context *c, int n)
{
	return (uint64_t *) &c->Rax + n;
}

static void set_trap(cpu_context *c, bool on)
{
	if (on)
		c->EFlags |= TRAP_FLAG;
	else
		c->EFlags &= ~(DWORD) TRAP_FLAG;
}
#else
static const int greg_index[16] = {
	REG_RAX, REG_RCX, REG_RDX, REG_RBX, REG_RSP, REG_RBP, REG_RSI, REG_RDI,
	REG_R8, REG_R9, REG_R10, REG_R11, REG_R12, REG_R13, REG_R14, REG_R15,
};

static uint64_t *gpr(cpu_context *c, int n)
{
	return (uint64_t *) &c->gregs[greg_index[n]];
}

static void set_trap(cpu_context *c, bool on)
{
	if (on)
		c->gregs[REG_EFL] |= TRAP_FLAG;
	else
		c->gregs[REG_EFL] &= ~(greg_t) TRAP_FLAG;
}
#endif

static void log_once(uintptr_t pc, uintptr_t address, const char *what);

static void arm(cpu_context *c, int reg)
{
	pending.regs[pending.count] = reg;
	pending.original[pending.count] = *gpr(c, reg);
	pending.count++;
}

/* Whether the instruction also uses the moved register as a value, which would see the move. */
static bool uses_moved_value(const insn_info *in, int reg)
{
	if (in->string)
		return false;
	if (in->base == in->index)
		return true;
	if (in->reg == reg && !in->pure_load)
		return true;
	return in->writes_rax_rdx && (reg == 0 || reg == 2);
}

/* Points the faulting instruction at the segment and arms a single step. False when it cannot. */
static bool begin_step(cpu_context *c, uintptr_t pc, uintptr_t address)
{
	insn_info in;

	if (!decode((const uint8_t *) pc, &in)) {
		log_once(pc, address, "unhandled null access");
		return false;
	}
	pending.count = 0;
	if (in.string) {
		if (*gpr(c, 6) < SEGMENT_SIZE)
			arm(c, 6);
		if (*gpr(c, 7) < SEGMENT_SIZE)
			arm(c, 7);
	} else if (in.base != NO_REG && *gpr(c, in.base) < SEGMENT_SIZE) {
		arm(c, in.base);
	} else if (in.index != NO_REG && in.scale == 1 && *gpr(c, in.index) < SEGMENT_SIZE) {
		arm(c, in.index);
	}
	for (int i = 0; i < pending.count; i++) {
		if (uses_moved_value(&in, pending.regs[i]))
			pending.count = 0;
	}
	if (pending.count == 0) {
		log_once(pc, address, "unhandled null access");
		return false;
	}

	for (int i = 0; i < pending.count; i++)
		*gpr(c, pending.regs[i]) += (uintptr_t) segment;
	pending.info = in;
	pending.armed = true;
	set_trap(c, true);
	log_once(pc, address, "null access");
	return true;
}

/* After the one instruction: puts the moved registers back. */
static void finish_step(cpu_context *c)
{
	uintptr_t moved_by = (uintptr_t) segment;

	for (int i = 0; i < pending.count; i++) {
		int reg = pending.regs[i];
		uint64_t *value = gpr(c, reg), original = pending.original[i], mask = ~(uint64_t) 0;

		if (pending.info.string) {
			*value -= moved_by;                    /* keep how far it advanced */
			continue;
		}
		if (reg != pending.info.reg) {
			*value = original;
			continue;
		}
		/* A load into the moved register: keep what it loaded. Narrow loads keep the rest. */
		if (pending.info.size == 1)
			mask = pending.info.high_byte ? 0xff00 : 0xff;
		else if (pending.info.size == 2)
			mask = 0xffff;
		*value = (original & ~mask) | (*value & mask);
	}
	set_trap(c, false);
	pending.armed = false;
}

static bool first_logging(uintptr_t pc)
{
	for (int i = 0; i < logged_count; i++) {
		if (logged[i] == pc)
			return false;
	}
	if (logged_count < MAX_LOGGED)
		logged[logged_count++] = pc;
	return true;
}

#ifdef _WIN32

static bool dbghelp_ready;

static void log_once(uintptr_t pc, uintptr_t address, const char *what)
{
	char buffer[sizeof(SYMBOL_INFO) + 256];
	SYMBOL_INFO *symbol = (SYMBOL_INFO *) buffer;
	DWORD64 offset = 0;
	HMODULE module;
	char path[MAX_PATH], text[512];

	if (!first_logging(pc))
		return;
	memset(buffer, 0, sizeof buffer);
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	symbol->MaxNameLen = 255;
	if (dbghelp_ready && SymFromAddr(GetCurrentProcess(), pc, &offset, symbol))
		snprintf(text, sizeof text, "%s at %s+%llu (address %#llx)\n", what, symbol->Name,
			(unsigned long long) offset, (unsigned long long) address);
	else if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR) pc, &module) &&
		GetModuleFileNameA(module, path, sizeof path) != 0)
		snprintf(text, sizeof text, "%s at %s+%#llx (address %#llx)\n", what,
			strrchr(path, '\\') ? strrchr(path, '\\') + 1 : path,
			(unsigned long long) (pc - (uintptr_t) module), (unsigned long long) address);
	else
		snprintf(text, sizeof text, "%s at %#llx (address %#llx)\n", what,
			(unsigned long long) pc, (unsigned long long) address);
	fputs(text, stderr);
}

static LONG CALLBACK on_exception(EXCEPTION_POINTERS *pointers)
{
	EXCEPTION_RECORD *record = pointers->ExceptionRecord;
	CONTEXT *c = pointers->ContextRecord;

	if (record->ExceptionCode == EXCEPTION_SINGLE_STEP && pending.armed) {
		finish_step(c);
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	/* Only reads and writes below 64 KB; anything else is a real crash. */
	if (record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION || record->NumberParameters < 2 ||
		record->ExceptionInformation[0] > 1 || record->ExceptionInformation[1] >= SEGMENT_SIZE ||
		pending.armed)
		return EXCEPTION_CONTINUE_SEARCH;
	if (!begin_step(c, (uintptr_t) c->Rip, (uintptr_t) record->ExceptionInformation[1]))
		return EXCEPTION_CONTINUE_SEARCH;
	return EXCEPTION_CONTINUE_EXECUTION;
}

void nullpage_install(void)
{
	SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);
	dbghelp_ready = SymInitialize(GetCurrentProcess(), NULL, TRUE);
	AddVectoredExceptionHandler(1, on_exception);
}

#else

static void log_once(uintptr_t pc, uintptr_t address, const char *what)
{
	Dl_info info;
	char text[512];

	if (!first_logging(pc))
		return;
	if (dladdr((void *) pc, &info) && info.dli_sname != NULL)
		snprintf(text, sizeof text, "%s at %s+%lu (address %#lx)\n", what, info.dli_sname,
			(unsigned long) (pc - (uintptr_t) info.dli_saddr), (unsigned long) address);
	else if (dladdr((void *) pc, &info) && info.dli_fname != NULL)
		snprintf(text, sizeof text, "%s at %s+%#lx (address %#lx)\n", what,
			strrchr(info.dli_fname, '/') ? strrchr(info.dli_fname, '/') + 1 : info.dli_fname,
			(unsigned long) (pc - (uintptr_t) info.dli_fbase), (unsigned long) address);
	else
		snprintf(text, sizeof text, "%s at %#lx (address %#lx)\n", what, (unsigned long) pc,
			(unsigned long) address);
	fputs(text, stderr);
}

static void on_signal(int sig, siginfo_t *info, void *context)
{
	mcontext_t *c = &((ucontext_t *) context)->uc_mcontext;

	if (sig == SIGTRAP) {
		if (pending.armed) {
			finish_step(c);
			return;
		}
		/* Not ours: take the default action now. */
		signal(SIGTRAP, SIG_DFL);
		raise(SIGTRAP);
		return;
	}
	/* Only accesses below 64 KB; anything else is a real crash. */
	if ((uintptr_t) info->si_addr < SEGMENT_SIZE && !pending.armed &&
		begin_step(c, (uintptr_t) c->gregs[REG_RIP], (uintptr_t) info->si_addr))
		return;
	signal(sig, SIG_DFL);
}

void nullpage_install(void)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof sa);
	sa.sa_sigaction = on_signal;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGBUS, &sa, NULL);
	sigaction(SIGTRAP, &sa, NULL);
}

#endif

#else

void nullpage_install(void)
{
}

#endif
