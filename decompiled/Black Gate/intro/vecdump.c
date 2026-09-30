/* Black Gate INTRO.EXE, resident segment 9 (file offsets 0x00b098 to 0x00b125, 141 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include <stdio.h>

#define VECTOR_COUNT    256

int PrintVectors(FILE *fp);

/* Writes the interrupt vector table to a file. */
int DumpVectors(char *fileName)
{
	FILE *fp;
	int result;

	fp = fopen(fileName, "w");
	result = PrintVectors(fp);
	if (fp)
		fclose(fp);
	return result;
}

int PrintVectors(FILE *fp)
{
	unsigned far *vector;
	int i;

	if (fp) {
		fprintf(fp, "Interrupt Vector Table Dump\n");
		fprintf(fp, "Vector Number\t\tPointer\n");
		vector = 0;
		for (i = 0; i < VECTOR_COUNT; i++) {
			fprintf(fp, "%02x\t\t\t%04x:%04x\n", i, vector[1], vector[0]);
			vector += 2;
		}
		return 1;
	}
	return 0;
}
