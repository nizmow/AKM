/*
 * NeoST scenario helper: MagiC starts #_APP entries before MAGXDESK owns the
 * menu bar.  Start MGNOTICE after the desktop settles so its windows and menu
 * are foregrounded, then let the test drive it through the normal AES surface.
 */
#include <gem.h>
#include <mint/osbind.h>

int main(void)
{
	appl_init();
	evnt_timer(8000);
	Pexec(0, "C:\\GEMSYS\\MAGIC\\START\\MGNOTICE.APP", "", (char *) 0);
	appl_exit();
	return 0;
}
