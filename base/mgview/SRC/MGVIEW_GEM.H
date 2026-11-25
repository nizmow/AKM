#ifndef MGVIEW_GEM_H
#define MGVIEW_GEM_H

#include <stddef.h>
#include <gem.h>
#include <gemx.h>
#include <mint/osbind.h>
#include <mint/mintbind.h>

/* Messages used by MGVIEW that are not supplied by this SDK's GEMlib. */
#ifndef VA_START
#define VA_START 0x4711
#endif
#ifndef DD_NAK
#define DD_NAK 1
#endif

void *memchr2(const void *s, size_t len);

#endif /* MGVIEW_GEM_H */
