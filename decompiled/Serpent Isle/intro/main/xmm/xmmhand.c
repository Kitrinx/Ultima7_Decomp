/* Serpent Isle INTRO.EXE, resident segment 83 (file offsets 0x015788 to 0x0158b3, 299 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 */

/* path: xmmhand.c */
#include <stdio.h>
#include <io.h>
#include "init.h"
#include "xmmblock.h"
#include "xmmhand.h"

#define XMM_ERROR(line) FatalError(__FILE__, line)

unsigned char HasXmmhand(void)
{
	if (access("xmmhand.dat", 0) != 0)
		return 0;
	return 1;
}

/* The caller passes the handle it opened; the handle is read from XMSHandle instead. */
void SaveXmmhand(int handle)
{
	FILE *fp;

	if ((fp = fopen("xmmhand.dat", "wb")) == NULL)
		XMM_ERROR(76);
	if (fwrite(&XMSHandle, sizeof XMSHandle, 1, fp) != 1)
		XMM_ERROR(79);
	fclose(fp);
}

int LoadXmmhand(void)
{
	FILE *fp;
	int handle = 0;

	if ((fp = fopen("xmmhand.dat", "rb")) == NULL)
		XMM_ERROR(90);
	if (fread(&handle, sizeof handle, 1, fp) != 1)
		XMM_ERROR(93);
	fclose(fp);
	if (remove("xmmhand.dat") != 0)
		XMM_ERROR(97);
	return handle;
}

void DeleteXmmhand(void)
{
	if (remove("xmmhand.dat") != 0)
		XMM_ERROR(106);
}
