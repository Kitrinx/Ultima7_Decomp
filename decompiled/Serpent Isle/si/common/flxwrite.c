/* Serpent Isle SI.EXE, resident segment 55 (file offsets 0x021ac5 to 0x0225d8, 2835 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include <io.h>
#include <stdio.h>
#include <string.h>
#include "dosio.h"
#include "easyfile.h"
#include "flex.h"
#include "init.h"
#include "memapi.h"

char *FlexTempFileName = "FLEXFILE.$$$";

void FlexWriter::openForWrite(char *path, FlexHeader *initial)
{
	verify = 0;
	if (!open(path)) {
		FlexHeader saved = hdr;
		char changed = !hdr.unchanged();
		handle = CreateFileOrFail(name);
		if (initial)
			hdr = *initial;
		else if (changed)
			hdr = saved;
		else
			ReportError(0xa920);
		hdr.setFlexMarks();
		hdr.writeIfChanged(handle);
		FlexEntry blank;
		blank.offset = 0;
		blank.size = 0;
		for (long i = 0; i < (int)hdr.count; i++)
			writeEntry(i, &blank);
	}
	fragmentation = -1;
}

void FlexWriter::writeEntry(int i, FlexEntry *entry)
{
	if (entry->size == 0)
		entry->clear();
	if (!(int)DosWrite(handle, (hdr.valid() ? sizeof(FlexHeader) : 0L) + i * sizeof(FlexEntry), sizeof(FlexEntry),
		entry))
		ReportError(0xa913);
	fragmentation = -1;
}

void FlexWriter::write(int i, void far *buffer, long size, unsigned char quiet)
{
	long offset;
	FlexEntry entry, old;

	if (size <= 0 || buffer == 0)
		offset = 0;
	else {
		getEntry(i, &old);
		offset = filelength(handle);
		if (offset < (hdr.valid() ? sizeof(FlexHeader) : 0L))
			ReportError(0xa917);
	}
	entry = FlexEntry(offset, size);
	writeEntry(i, &entry);
	if (!(int)entry.empty()) {
		if (DosWrite(handle, entry.offset, entry.size, buffer)) {
			if (verify) {
				getEntry(i, &old);
				if (!(int)(unsigned char)(old == entry))
					ReportError(0xa918);
				char before = 0, after = 0;
				long i;
				for (i = 0; i < size; i++)
					before += ((char huge *)buffer)[i];
				readEntry(&old, buffer, 0);
				for (i = 0; i < size; i++)
					after += ((char huge *)buffer)[i];
				if (before != after)
					ReportError(0xa919);
			}
		} else {
			if (!(int)quiet)
				ReportError(0xa914);
			else
				setError(0xa914);
		}
	}
	fragmentation = -1;
}

void FlexWriter::writeBlock(int i, long block, long size, unsigned char quiet)
{
	long offset;
	FlexEntry entry, old;

	if (size <= 0 || block == 0)
		offset = 0;
	else {
		getEntry(i, &old);
		offset = filelength(handle);
		if (offset < (hdr.valid() ? sizeof(FlexHeader) : 0L))
			ReportError(0xa917);
	}
	entry = FlexEntry(offset, size);
	if (block == 0)
		entry.clear();
	if (entry.empty()) {
		writeEntry(i, &entry);
	} else {
		writeEntry(i, &entry);
		if (!entry.empty() && !WriteHandleFromVoodoo(handle, entry.offset, entry.size, block)) {
			if (!(int)quiet)
				ReportError(0xa915);
			else
				setError(0xa914);
		}
		fragmentation = -1;
	}
}

void FlexWriter::replace(char *path, int i, void far *buffer, long size)
{
	open(path);
	write(i, buffer, size, 0);
	close();
}

long FlexWriter::countObjectBytes()
{
	FlexEntry entry;
	long sum;
	int i;

	sum = 0;
	for (i = 0; i < (int)hdr.count; i++) {
		getEntry(i, &entry);
		sum += entry.size;
	}
	return sum;
}

int FlexWriter::measureWaste()
{
	if (fragmentation == -1) {
		long used = countObjectBytes() + (hdr.valid() ? sizeof(FlexHeader) : 0L) + (int)hdr.count * sizeof(FlexEntry);
		if (used == 0)
			fragmentation = 0;
		else {
			long total = getFileLength();
			total = total * 100 - 1;
			fragmentation = total / used - 99;
		}
	}
	hdr.packed = fragmentation == 0;
	return fragmentation;
}

void FlexWriter::compact(void far *buffer, long size)
{
	unsigned char ownBuffer;
	FlexEntry current, previous;
	long used;
	int index;
	FlexWriter output;

	ownBuffer = size == -1L;
	if (ownBuffer) {
		size = GetFarHeapLargest(8) - 100;
		buffer = AllocateFarOrFail(size, 0);
	}
	unlink(FlexTempFileName);
	output.openForWrite(FlexTempFileName, &hdr);
	int pending = 0;
	FlexEntry entries[100];
	int i;

	used = 0;
	previous.clear();
	for (index = 0; index < (int)hdr.count; index++) {
		getEntry(index, &current);
		if (!(int)current.empty() && (unsigned char)(current == previous)) {
			current.clear();
			printf("Clipped #%4d   ", index);
		} else {
			previous = current;
		}
		if (size < current.size)
			FatalError("Fbuf %ld<%ld", size, current.size / 1024);
		if (used + current.size > size || pending >= 99) {
			used = 0;
			for (i = 0; i < pending; i++) {
				output.write(index - pending + i,
					LinearToPointer(PointerToLinear(buffer) + used), entries[i].size, 0);
				used += entries[i].size;
			}
			pending = 0;
			used = 0;
		}
		entries[pending] = current;
		readEntry(&entries[pending], LinearToPointer(PointerToLinear(buffer) + used), 0);
		used += entries[pending].size;
		pending++;
	}
	used = 0;
	for (i = 0; i < pending; i++) {
		output.write(index - pending + i,
			LinearToPointer(PointerToLinear(buffer) + used), entries[i].size, 0);
		used += entries[i].size;
	}
	output.close();
	if (ownBuffer)
		FreeFarHeap(buffer);
	close();
	if (unlink(name))
		ReportError(0xa911);
	if (rename(FlexTempFileName, name))
		ReportError(0xa912);
	open(name);
	fragmentation = 0;
	hdr.packed = 1;
}

unsigned char FlexWriter::compactIfNeeded(int percent)
{
	unsigned char needed = measureWaste() > percent;
	if (needed)
		compact(0, -1L);
	return !needed;
}
