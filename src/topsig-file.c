#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "topsig-file.h"

// This code was contributed by Eric Tang

int IsDirectory(const char *filename)
{
	struct stat st;
	if( stat(filename, &st) == 0)
		return S_ISDIR(st.st_mode);
	return 0;
}

const char *GetDirSeparator()
{
#if defined(WINDOWS) || defined(_WIN32) || defined(_WIN64)
	static const char *SEPARATOR = "\\";
#else
	static const char *SEPARATOR = "/";
#endif
	return SEPARATOR;
}
