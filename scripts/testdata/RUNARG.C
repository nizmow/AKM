/* Run C:\TARGET.PRG with the tail from C:\RUNARG.TXT and report child status. */
#include <mint/osbind.h>
#include <stdio.h>
#include <string.h>

static char *line_end(char *text)
{
	while (*text && *text != '\r' && *text != '\n')
		text++;
	while (*text == '\r' || *text == '\n')
		*text++ = '\0';
	return text;
}

int main(void)
{
	char config[512];
	char target[128];
	char tail_text[128];
	char tail[130];
	char output[32];
	long handle;
	long count;
	long status;
	char *next;

	handle = Fopen("C:\\RUNARG.TXT", 0);
	if (handle < 0) {
		Cconws("STATUS -1\r\n");
		return 1;
	}
	count = Fread(handle, sizeof(config) - 1, config);
	Fclose(handle);
	if (count <= 0) {
		Cconws("STATUS -1\r\n");
		return 1;
	}
	config[count] = '\0';
	strncpy(target, config, sizeof(target) - 1);
	target[sizeof(target) - 1] = '\0';
	next = line_end(target);
	if (*target == '\0') {
		Cconws("STATUS -1\r\n");
		return 1;
	}
	strncpy(tail_text, next, sizeof(tail_text) - 1);
	tail_text[sizeof(tail_text) - 1] = '\0';
	line_end(tail_text);

	count = (long)strlen(tail_text);
	tail[0] = (char)count;
	memcpy(tail + 1, tail_text, count);
	tail[count + 1] = '\0';

	status = Pexec(0, target, tail, NULL);
	sprintf(output, "STATUS %ld\r\n", status);
	Cconws(output);
	return 0;
}
