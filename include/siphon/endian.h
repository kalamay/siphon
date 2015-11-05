#ifndef SIPHON_ENDIAN_H
#define SIPHON_ENDIAN_H

#if BYTE_ORDER == LITTLE_ENDIAN
# define sp_htobe16(v) __builtin_bswap16(v)
# define sp_htole16(v) (v)
# define sp_be16toh(v) __builtin_bswap16(v)
# define sp_le16toh(v) (v)
# define sp_htobe32(v) __builtin_bswap32(v)
# define sp_htole32(v) (v)
# define sp_be32toh(v) __builtin_bswap32(v)
# define sp_le32toh(v) (v)
# define sp_htobe64(v) __builtin_bswap64(v)
# define sp_htole64(v) (v)
# define sp_be64toh(v) __builtin_bswap64(v)
# define sp_le64toh(v) (v)
#elif BYTE_ORDER == BIG_ENDIAN
# define sp_htobe16(v) (v)
# define sp_htole16(v) __builtin_bswap16(v)
# define sp_be16toh(v) (v)
# define sp_le16toh(v) __builtin_bswap16(v)
# define sp_htobe32(v) (v)
# define sp_htole32(v) __builtin_bswap32(v)
# define sp_be32toh(v) (v)
# define sp_le32toh(v) __builtin_bswap32(v)
# define sp_htobe64(v) (v)
# define sp_htole64(v) __builtin_bswap64(v)
# define sp_be64toh(v) (v)
# define sp_le64toh(v) __builtin_bswap64(v)
#endif

#endif

