/*
 * NeoST scenario helper: wait for MAGXDESK, then start MGSEARCH in its own
 * directory so the resource file and INF path resolve consistently.
 */
#include <gem.h>
#include <mint/osbind.h>

int main(void)
{
	appl_init();
	evnt_timer(8000);
	Dsetpath("C:\\GEMSYS\\GEMDESK");
	Pexec(0, "C:\\GEMSYS\\GEMDESK\\MGSEARCH.APP", "", (char *) 0);
	appl_exit();
	return 0;
}
