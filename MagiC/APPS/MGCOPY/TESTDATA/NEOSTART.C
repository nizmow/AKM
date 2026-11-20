/*
 * NeoST scenario helper: wait for MAGXDESK, then run headless MGCOPY cases.
 * Results are written to C:\MGCOPY.RES for host-side comparison.
 */
#include <gem.h>
#include <mint/osbind.h>
#include <stdio.h>
#include <string.h>

#define APP "C:\\MGCOPY.APP"
#define RESULT "C:\\MGCOPY.RES"

static int result_handle;

static void record(const char *line)
{
	long len = strlen(line);
	Fwrite(result_handle, len, (void *) line);
}

static void run_case(int number, const char *tail)
{
	char command[128];
	char output[64];
	long status;
	unsigned int len = strlen(tail);

	if (len > 126)
	{
		sprintf(output, "CASE %02d BAD-TAIL\r\n", number);
		record(output);
		return;
	}
	command[0] = (char) len;
	memcpy(command + 1, tail, len);
	command[len + 1] = '\0';
	status = Pexec(0, APP, command, NULL);
	sprintf(output, "CASE %02d %ld\r\n", number, status);
	record(output);
}

int main(void)
{
	appl_init();
	evnt_timer(8000);
	Dsetpath("C:\\");

	result_handle = (int) Fcreate(RESULT, 0);
	if (result_handle < 0)
	{
		appl_exit();
		return 1;
	}

	run_case(1, "-Cq C:\\SRC.TXT -4 C:\\DEST\\");
	run_case(2, "-Mq C:\\MOVE.TXT -4 C:\\DEST\\");
	run_case(3, "-Aq C:\\ALIAS.TXT -4 C:\\DEST\\");
	run_case(4, "-Dq C:\\DEL.TXT -4");

	Fclose(result_handle);
	appl_exit();
	return 0;
}
