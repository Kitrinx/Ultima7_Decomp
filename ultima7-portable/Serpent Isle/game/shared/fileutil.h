#ifndef SHARED_FILEUTIL_H
#define SHARED_FILEUTIL_H

inline char *GetWorkString(char **string)
{
	return *string;
}

inline int32_t ClampLong(int32_t low, int32_t value, int32_t high)
{
	if (value < low)
		return low;
	if (value > high)
		return high;
	return value;
}

#endif
