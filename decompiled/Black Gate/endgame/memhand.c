/* Black Gate ENDGAME.EXE, resident segment 45 (file offsets 0x00e484 to 0x00e7c2, 830 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d rebuilds it byte for byte as C++.
 */

#include "memsys.h"

/* Allocate, checking first that enough is free. With fail set a shortage is fatal; otherwise
 * it returns null. */
void far *MemoryHandler::allocateChecked(long size, unsigned char flags, unsigned char fail)
{
	void far *block;
	long free;

	free = available();
	if (size > free) {
		if (fail) {
			reportFailure(type(), size, free);
			error("\n");
		} else
			return 0;
	}
	block = allocate(size, flags);
	if (block == 0) {
		if (fail) {
			reportFailure(type(), size, free);
			error("\n");
		} else
			return 0;
	}
	return block;
}

/* The same, naming the request by a number in the message. */
void far *MemoryHandler::allocateChecked(long size, unsigned char flags, unsigned char fail, int code)
{
	void far *block;
	long free;

	free = available();
	if (size > free) {
		if (fail) {
			reportFailure(type(), size, free);
			error("\n%d\n", code);
		} else
			return 0;
	}
	block = allocate(size, flags);
	if (block == 0) {
		if (fail) {
			reportFailure(type(), size, free);
			error("\n%d\n", code);
		} else
			return 0;
	}
	return block;
}

/* The same, naming what the memory was for in the message. */
void far *MemoryHandler::allocateChecked(long size, unsigned char flags, unsigned char fail, char *what)
{
	void far *block;
	long free;

	free = available();
	if (size > free) {
		if (fail) {
			reportFailure(type(), size, free);
			error("\n%s\n", what);
		} else
			return 0;
	}
	block = allocate(size, flags);
	if (block == 0) {
		if (fail) {
			reportFailure(type(), size, free);
			error("\n%s\n", what);
		} else
			return 0;
	}
	return block;
}

/* Put the error code, the memory type and the sizes into the message. */
void MemoryHandler::reportFailure(unsigned char type, long size, long free)
{
	String text;

	if (type == NEAR_MEMORY) {
		message.append("\nError: 0x0103\n");
		text.format("Failed Memory Type: %d (Near)\n", type);
		message.append(text);
	} else if (type == FAR_MEMORY) {
		message.append("\nError: 0x0203\n");
		text.format("Failed Memory Type: %d (Far)\n", type);
		message.append(text);
	} else if (type == EMS_MEMORY) {
		message.append("\nError: 0x0303\n");
		text.format("Failed Memory Type: %d (EMS)\n", type);
		message.append(text);
	}
	text.format("Bytes asked for   : %ld\nBytes available   : %ld\n", size, free);
	message.append(text);
}
