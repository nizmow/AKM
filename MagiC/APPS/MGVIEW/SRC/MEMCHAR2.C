#include <stddef.h>

void *memchr2(const void *s, size_t len)
{
	const unsigned char *p = (const unsigned char *)s;

	while (len != 0)
	{
		if (*p == '\r' || *p == '\n')
			return (void *)p;
		++p;
		--len;
	}
	return NULL;
}
