#ifndef INSTALL_H
#define INSTALL_H

/* One optional part INSTALL.EXE offered, as read from install.prm. */
struct InstallOption {
	char kind;
	long offset;
	long size;
	char enabled;
	char unused[10];
	InstallOption *next;
	InstallOption();
};

/* What install.prm holds of an option: everything but the link. */
#define OPTION_RECORD_SIZE 20

/* The requirements INSTALL.EXE leaves in install.prm, with the options it offered. */
struct InstallParams {
	long dosMemory;
	long speechMemory;
	long adlibMemory;           /* added for the AdLib music driver */
	long rolandMemory;          /* added for the Roland music driver */
	long extendedMemory;
	long fullDiskSpace;
	long diskSpace;
	int unused;
	int optionCount;
	InstallOption *options;
	InstallParams();
	~InstallParams();
	void setDefaults();
	void clear();
	int load(char *name);
	int save(char *name);
	void show();
	InstallOption *option(int n);
	int append(InstallOption *option);
	int add(int extra);
	int count();
	long lastOffset();
	long endOffset();
};

extern InstallParams InstallInfo;

#endif
