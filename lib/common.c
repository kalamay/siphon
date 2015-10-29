#include "../include/siphon/common.h"
#include "../include/siphon/fmt.h"

void
sp_print_ptr (const void *val, FILE *out)
{
	fprintf (out, "%p", val);
}

void
sp_print_str (const void *val, FILE *out)
{
	sp_fmt_str (out, val, strlen (val), true);
}

