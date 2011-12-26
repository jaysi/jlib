#include "j_if.h"
#include "jer.h"
#include "jtypes.h"

#include <wchar.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int lookup_cmd(wchar_t * cmd)
{
	struct JifCmd *c;

	for (c = jifcmd; c->str; c++)
		if (!wcscmp(c->str, cmd))
			return c->no;
	return -JE_NOTFOUND;
}

void jif_perr(int err)
{
	unsigned char estr[MAX_ERR_STR];
	jer_lookup_str(err, estr);
	printf("\t\tError: %i, %s\n", err, estr);
	perror("\t\tSYS: ");
}

int jif_read_file(char *filename, unsigned char **ex_buf,
		  unsigned long *ex_size)
{
	int handle;
	struct stat st;
	int ret;
	off_t total_read;
	ssize_t this_read;

	*ex_size = 0UL;

	handle = open(filename, O_RDONLY);
	if (handle < 0)
		return -JE_OPEN;

	ret = fstat(handle, &st);
	if (ret < 0)
		return -JE_SYSE;

	if (!st.st_size)
		return -JE_SIZE;

	*ex_buf = (unsigned char *)malloc(st.st_size);

	*ex_size = st.st_size;

	total_read = 0UL;

	do {

		this_read =
		    read(handle, (*ex_buf) + total_read,
			 st.st_size - total_read);

		if (this_read < 0)
			return -JE_READ;

		total_read += this_read;

	} while (total_read < st.st_size);

	wprintf(L"\nLoaded %u bytes from < %s >\n", *ex_size, filename);

	return 0;
}
