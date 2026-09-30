/* Black Gate U7.EXE, resident segment 83 (file offsets 0x02bf06 to 0x02ca19, 2835 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "u7port.h"
#include <stdio.h>
#include <string.h>
#include "plat.h"
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
		int8_t changed = !hdr.unchanged();
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
		for (int32_t i = 0; i < (int16_t)hdr.count; i++)
			writeEntry(i, &blank);
	}
	fragmentation = -1;
}

void FlexWriter::writeEntry(int16_t i, FlexEntry *entry)
{
	if (entry->size == 0)
		entry->clear();
	if (!(int16_t)DosWrite(handle, (hdr.valid() ? (int32_t) sizeof(FlexHeader) : INT32_C(0)) + i * sizeof(FlexEntry), sizeof(FlexEntry),
		entry))
		ReportError(0xa913);
	fragmentation = -1;
}

void FlexWriter::write(int16_t i, void *buffer, int32_t size, uint8_t quiet)
{
	int32_t offset;
	FlexEntry entry, old;

	if (size <= 0 || buffer == 0)
		offset = 0;
	else {
		getEntry(i, &old);
		offset = plat_file_length(handle);
		if (offset < (hdr.valid() ? (int32_t) sizeof(FlexHeader) : INT32_C(0)))
			ReportError(0xa917);
	}
	entry = FlexEntry(offset, size);
	writeEntry(i, &entry);
	if (!(int16_t)entry.empty()) {
		if (DosWrite(handle, entry.offset, entry.size, buffer)) {
			if (verify) {
				getEntry(i, &old);
				if (!(int16_t)(uint8_t)(old == entry))
					ReportError(0xa918);
				int8_t before = 0, after = 0;
				int32_t i;
				for (i = 0; i < size; i++)
					before += ((char *)buffer)[i];
				readEntry(&old, buffer, 0);
				for (i = 0; i < size; i++)
					after += ((char *)buffer)[i];
				if (before != after)
					ReportError(0xa919);
			}
		} else {
			if (!(int16_t)quiet)
				ReportError(0xa914);
			else
				setError(0xa914);
		}
	}
	fragmentation = -1;
}

void FlexWriter::writeBlock(int16_t i, int32_t block, int32_t size, uint8_t quiet)
{
	int32_t offset;
	FlexEntry entry, old;

	if (size <= 0 || block == 0)
		offset = 0;
	else {
		getEntry(i, &old);
		offset = plat_file_length(handle);
		if (offset < (hdr.valid() ? (int32_t) sizeof(FlexHeader) : INT32_C(0)))
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
			if (!(int16_t)quiet)
				ReportError(0xa915);
			else
				setError(0xa914);
		}
		fragmentation = -1;
	}
}

void FlexWriter::replace(char *path, int16_t i, void *buffer, int32_t size)
{
	open(path);
	write(i, buffer, size, 0);
	close();
}

int32_t FlexWriter::countObjectBytes()
{
	FlexEntry entry;
	int32_t sum;
	int16_t i;

	sum = 0;
	for (i = 0; i < (int16_t)hdr.count; i++) {
		getEntry(i, &entry);
		sum += entry.size;
	}
	return sum;
}

int16_t FlexWriter::measureWaste()
{
	if (fragmentation == -1) {
		int32_t used = countObjectBytes() + (hdr.valid() ? (int32_t) sizeof(FlexHeader) : INT32_C(0)) + (int16_t)hdr.count * sizeof(FlexEntry);
		if (used == 0)
			fragmentation = 0;
		else {
			int32_t total = getFileLength();
			total = total * 100 - 1;
			fragmentation = total / used - 99;
		}
	}
	hdr.packed = fragmentation == 0;
	return fragmentation;
}

void FlexWriter::compact(void *buffer, int32_t size)
{
	uint8_t ownBuffer;
	FlexEntry current, previous;
	int32_t used;
	int16_t index;
	FlexWriter output;

	ownBuffer = size == -INT32_C(1);
	if (ownBuffer) {
		size = GetFarHeapLargest(8) - 100;
		buffer = AllocateFarOrFail(size, 0);
	}
	plat_file_remove(FlexTempFileName);
	output.openForWrite(FlexTempFileName, &hdr);
	int16_t pending = 0;
	FlexEntry entries[100];
	int16_t i;

	used = 0;
	previous.clear();
	for (index = 0; index < (int16_t)hdr.count; index++) {
		getEntry(index, &current);
		if (!(int16_t)current.empty() && (uint8_t)(current == previous)) {
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
	if (!plat_file_remove(name))
		ReportError(0xa911);
	if (!plat_file_rename(FlexTempFileName, name))
		ReportError(0xa912);
	open(name);
	fragmentation = 0;
	hdr.packed = 1;
}

uint8_t FlexWriter::compactIfNeeded(int16_t percent)
{
	uint8_t needed = measureWaste() > percent;
	if (needed)
		compact(0, -INT32_C(1));
	return !needed;
}
