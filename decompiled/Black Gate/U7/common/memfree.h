#ifndef MEMFREE_H
#define MEMFREE_H

#ifdef __cplusplus
/* One static instance records the near heap as it stood at startup. */
class NearMemoryInfo {
public:
	NearMemoryInfo();
	unsigned getNearFree();
};

extern NearMemoryInfo NearMemory;
#endif

/* Free memory in the near heap, the far heap and high memory. */
struct MemInfo {
	unsigned nearFree;
	unsigned long farFree;
	unsigned long highFree;
#ifdef __cplusplus
	MemInfo() : nearFree(0), farFree(0), highFree(0) {}
#endif
};

#ifdef __cplusplus
extern "C" {
#endif
void GetMemoryInfo(struct MemInfo *m);
char *SprintfMemoryUsage(struct MemInfo *start);
#ifdef __cplusplus
}
#endif

#endif
