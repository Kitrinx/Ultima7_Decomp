#ifndef FILEUTIL_H
#define FILEUTIL_H

inline char *GetWorkString(char **string)
{
	return *string;
}

inline long ClampLong(long low, long value, long high)
{
	if (value < low)
		return low;
	if (value > high)
		return high;
	return value;
}

#endif
