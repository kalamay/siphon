#ifndef SIPHON_ENDIAN_H
#define SIPHON_ENDIAN_H

#if defined(__linux__)
#ifndef _BSD_SOURCE
# define _BSD_SOURCE
#endif
#ifndef __USE_BSD
# define __USE_BSD
#endif

# include <endian.h>

#elif defined(__APPLE__)
# include <machine/endian.h>
# if BYTE_ORDER == LITTLE_ENDIAN
#  define IX_LITTLE_ENDIAN
#  define htobe16(v) __builtin_bswap16(v)
#  define htole16(v) (v)
#  define be16toh(v) __builtin_bswap16(v)
#  define le16toh(v) (v)
#  define htobe32(v) __builtin_bswap32(v)
#  define htole32(v) (v)
#  define be32toh(v) __builtin_bswap32(v)
#  define le32toh(v) (v)
#  define htobe64(v) __builtin_bswap64(v)
#  define htole64(v) (v)
#  define be64toh(v) __builtin_bswap64(v)
#  define le64toh(v) (v)
# elif BYTE_ORDER == BIG_ENDIAN
#  define IX_LITTLE_ENDIAN
#  define htobe16(v) (v)
#  define htole16(v) __builtin_bswap16(v)
#  define be16toh(v) (v)
#  define le16toh(v) __builtin_bswap16(v)
#  define htobe32(v) (v)
#  define htole32(v) __builtin_bswap32(v)
#  define be32toh(v) (v)
#  define le32toh(v) __builtin_bswap32(v)
#  define htobe64(v) (v)
#  define htole64(v) __builtin_bswap64(v)
#  define be64toh(v) (v)
#  define le64toh(v) __builtin_bswap64(v)
# endif

#elif defined(__OpenBSD__)
# include <machine/endian.h>

#elif defined(__NetBSD__) || defined(__FreeBSD__) || (__DragonFly__)
# include <sys/endian.h>

#else
# error Endianess not available
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
# define IX_LITTLE_ENDIAN
#elif BYTE_ORDER == BIG_ENDIAN
# define IX_BIG_ENDIAN
#endif

#endif

