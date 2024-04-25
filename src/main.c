#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <drm.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2

/* We use void, because opening consoles
 * isn't strictly necessary for the shim
 * to function and boot correctly. */
void initConsoleToFD()
{
	/* We don't actually use stdin for anything,
	 * but keeping it in here to be consistent
	 * with other init systems is good. */

	// Open console device
	int stdin = open("/dev/console", O_RDONLY, 0);

	// Link stdin to console
	dup2(stdin, STDIN);

	/* stdout is going to get printed to kmsg,
	 * hopefully pstore can catch it. If not,
	 * early debugging will need to be performed on a
	 * Samsung device with last_kmsg buffer in place */

	int stdout = open("/dev/console", O_RDWR, 0);

	// Link stdout and stderr to console
	dup2(stdout, STDOUT);
	dup2(stdout, STDERR);

	if (stdout > 2) close(stdout);
	if (stdin > 2) close(stdin);
}

int main()
{
    initConsoleToFD();
	printf("Hi, I'm alive!\n");
	initDrm();
    /* NEVER exit as init */
    for(;;){}
}
