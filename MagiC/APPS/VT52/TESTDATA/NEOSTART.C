/*
 * NeoST scenario helper: wait for MAGXDESK, then start VT52 with a TOS child.
 * The host scenario compares the archived and rebuilt terminal windows.
 */
#include <gem.h>
#include <mint/osbind.h>
#include <string.h>

#define APP  "C:\\VT52.PRG"
#define TAIL "C:\\VT52DEMO.TOS"

int main(void)
{
	char command[64];
	unsigned int len = strlen(TAIL);

	appl_init();
	evnt_timer(12000);
	Dsetpath("C:\\");
	if (len < sizeof(command) - 1) {
		command[0] = (char) len;
		memcpy(command + 1, TAIL, len);
		command[len + 1] = '\0';
		Pexec(0, APP, command, NULL);
	}
	appl_exit();
	return 0;
}
