/* Black Gate U7.EXE, resident segment 186 (file offsets 0x04030f to 0x040494, 389 bytes).
 * Borland C++ 2.0 -mm -O -G -P -vi- rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "lowlevel.h"
#include "debug.h"

/* the overlay load log: its entries sit at a linear address */
struct OverlayLog {
	char unusedField1[2];
	unsigned long count;
	unsigned long base;
};

/* Built without -b-, so the mode is a word here and a byte in the file routines. */
enum FileMode { FILE_CREATE, FILE_OPEN };

struct DataFile {
	char *name;
	FileMode mode;
	int handle;
	DataFile() { handle = -1; name = 0; }
	~DataFile();
	unsigned char open(char far *name, FileMode mode);
	void close();
	int write(void far *p, long n);
	void put(unsigned long value);
};

extern unsigned _psp;
extern unsigned _ovrbuffer;

inline void DataFile::put(unsigned long value)
{
	write(&value, sizeof value);
}

char *OverlayFileName = "overlay.dat";
DataFile OverlayFile;

void CreateOverlayFile(void)
{
	OverlayFile.open(OverlayFileName, FILE_CREATE);
}

/* write the program's PSP segment, the overlay buffer's size in bytes, both counts and the log */
void SaveOverlayLog(OverlayLog *log)
{
	unsigned long i;
	unsigned long value;

	value = _psp;
	OverlayFile.put(value);
	value = _ovrbuffer;
	value <<= 4;
	OverlayFile.put(value);
	value = GetOverlayLoadCount();
	OverlayFile.put(value);
	value = GetOverlayLogCount();
	OverlayFile.put(value);
	for (i = 0; log->count > i; i++) {
		value = PeekLong(log->base + (i << 2));
		OverlayFile.put(value);
	}
	OverlayFile.close();
}
