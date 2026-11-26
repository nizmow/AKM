#include <mint/osbind.h>
#include <stdio.h>
#include <string.h>

struct test_case {
	const char *tail;
};

static const struct test_case cases[] = {
	{ "SET.PRG 64" },
	{ "SAME.PRG 64" },
	{ "REMOVE.PRG -" },
	{ "BADHDR.PRG" },
	{ "BADREL.PRG" },
	{ "MISSING.PRG" },
	{ "SET.PRG 0" },
	{ "SET.PRG nope" }
};

int main(void)
{
	unsigned int i;
	char output[32];
	char path[] = "C:\\LIMITMEM.TTP";

	Cconws("BEGIN\r\n");
	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		char tail[128];
		size_t length = strlen(cases[i].tail);
		long status;

		tail[0] = (char)length;
		memcpy(tail + 1, cases[i].tail, length + 1);
		status = Pexec(0, path, tail, NULL);
		sprintf(output, "CASE %02u %ld\r\n", i + 1, status);
		Cconws(output);
	}
	Cconws("DONE 8\r\n");
	return 0;
}
