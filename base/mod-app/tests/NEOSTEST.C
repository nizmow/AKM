#include <mint/osbind.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define MOD_APP "C:\\MOD_APP.TTP"

static const char *const cases[] = {
	"-Cia papillon.prg - -1 - 1",
	"-Cia papillon.prg - -1 - 1",
	"-Cid papillon.prg *.x - -1",
	"-Cid papillon.prg *.x - -1",
	"-Cid missing.prg *.z - -1",
	"-Cis ABCD sample - 12",
	"-Cis ABCD sample - 12",
	"-Cia space.prg - -1 'C:\\MY APPS' 1",
	"-Cia empty.prg - -1 '' 1",
	"-Cia longpath.prg - -1 C:\\AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA 1",
	"-Cia tool.prg RESOURCE-NAME-TOO-LONG 0 - 1",
	"-Cid papillon.prg 123456789012345678901234567890123 - -1",
	"-Cia code.prg - 0 - 7",
	"-Cia code.prg - -1 - 1",
	"-Cid papillon.prg *.y - -1",
	"-Cia quoted.prg - -1 'D:\\APP FILES\\PROGRAMS' 1",
	"-Cid papillon.prg *.x - -1"
};

/* Preserve each step, including file absence, for independent host assertions. */
static int snapshot(unsigned int number)
{
	static const char *const suffixes[] = {"INF", "BAK", "DAT"};
	unsigned int i;

	for (i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); ++i)
	{
		char source[64], destination[64], buffer[512];
		FILE *in, *out;
		size_t count;
		int failed;

		sprintf(source, "C:\\GEMSYS\\GEMDESK\\APPLICAT.%s", suffixes[i]);
		sprintf(destination, "C:\\RESULTS\\%02u.%s", number, suffixes[i]);
		errno = 0;
		in = fopen(source, "rb");
		if (!in)
		{
			if (errno == ENOENT)
				continue;
			return 0;
		}
		out = fopen(destination, "wb");
		if (!out)
		{
			fclose(in);
			return 0;
		}
		failed = 0;
		while ((count = fread(buffer, 1, sizeof(buffer), in)) != 0)
		{
			if (fwrite(buffer, 1, count, out) != count)
			{
				failed = 1;
				break;
			}
		}
		if (ferror(in))
			failed = 1;
		if (fclose(in))
			failed = 1;
		if (fclose(out))
			failed = 1;
		if (failed)
			return 0;
	}
	return 1;
}

int main(void)
{
	unsigned int i;
	char tail[128];

	if (!snapshot(0))
	{
		printf("SNAPSHOT 00 FAILED\r\n");
		return 1;
	}

	for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
	{
		size_t len = strlen(cases[i]);
		long result;

		if (len > 126)
		{
			printf("CASE %02u DRIVER-TAIL-TOO-LONG\r\n", i + 1);
			return 1;
		}
		tail[0] = (char)len;
		memcpy(tail + 1, cases[i], len + 1);
		result = Pexec(0, MOD_APP, tail, NULL);
		printf("CASE %02u %ld\r\n", i + 1, result);
		if (!snapshot(i + 1))
		{
			printf("SNAPSHOT %02u FAILED\r\n", i + 1);
			return 1;
		}
	}

	printf("DONE %u\r\n", (unsigned int)(sizeof(cases) / sizeof(cases[0])));
	return 0;
}
