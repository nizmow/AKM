/*
 * NeoST scenario helper: wait for MAGXDESK, then start MGFORMAT with a normal
 * command tail.  The host scenario drives the format/copy dialogs and checks
 * the disposable floppy image.
 */
#include <gem.h>
#include <mint/osbind.h>
#include <string.h>

#define APP "C:\\MGFORMAT.PRG"

static void run(const char *tail)
{
	char command[128];
	unsigned int len = strlen(tail);

	if (len > 126)
		return;
	command[0] = (char) len;
	memcpy(command + 1, tail, len);
	command[len + 1] = '\0';
	Pexec(0, APP, command, NULL);
}

int main(void)
{
	appl_init();
	evnt_timer(12000);
	Dsetpath("C:\\");
	run("-f A:");
	appl_exit();
	return 0;
}
