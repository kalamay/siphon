#ifndef SIPHON_MSGPACK_H
#define SIPHON_MSGPACK_H

#include "common.h"

#define SP_MSGPACK_TAG_MAX 9

typedef enum {
	SP_MSGPACK_NONE = -1,
	SP_MSGPACK_MAP = 1,
	SP_MSGPACK_ARRAY,
	SP_MSGPACK_MAP_END,
	SP_MSGPACK_ARRAY_END,
	SP_MSGPACK_NIL,
	SP_MSGPACK_TRUE,
	SP_MSGPACK_FALSE,
	SP_MSGPACK_SIGNED,
	SP_MSGPACK_UNSIGNED,
	SP_MSGPACK_FLOAT,
	SP_MSGPACK_DOUBLE,
	SP_MSGPACK_STRING,
	SP_MSGPACK_BINARY,
	SP_MSGPACK_EXT
} SpMsgpackType;

typedef union {
	int64_t i64;
	uint64_t u64;
	float f32;
	double f64;
	uint32_t count;
	struct {
		uint32_t len;
		int8_t type;
	} ext;
} SpMsgpackTag;

typedef struct {
	SpMsgpackTag tag;    // value enum
	SpMsgpackType type;  // type of the current parsed value
	unsigned cs;         // current scanner state
	uint8_t key;         // is parsing a map key
	uint8_t depth;       // stack entry size
	uint8_t stack[3];    // map/array bit stack
	uint32_t counts[24]; // map/array entry remaining counts
} SpMsgpack;

SP_EXPORT void
sp_msgpack_init (SpMsgpack *p);

SP_EXPORT void
sp_msgpack_reset (SpMsgpack *p);

SP_EXPORT void
sp_msgpack_final (SpMsgpack *p);

SP_EXPORT ssize_t
sp_msgpack_next (SpMsgpack *p, const void *restrict buf, size_t len, bool eof);

SP_EXPORT bool
sp_msgpack_is_done (const SpMsgpack *p);

SP_EXPORT size_t
sp_msgpack_enc (SpMsgpackType type, const SpMsgpackTag *tag, void *buf);

SP_EXPORT size_t
sp_msgpack_enc_nil (void *buf);

SP_EXPORT size_t
sp_msgpack_enc_true (void *buf);

SP_EXPORT size_t
sp_msgpack_enc_false (void *buf);

SP_EXPORT size_t
sp_msgpack_enc_signed (void *buf, int64_t val);

SP_EXPORT size_t
sp_msgpack_enc_unsigned (void *buf, uint64_t val);

SP_EXPORT size_t
sp_msgpack_enc_float (void *buf, float val);

SP_EXPORT size_t
sp_msgpack_enc_double (void *buf, double val);

SP_EXPORT size_t
sp_msgpack_enc_string (void *buf, uint32_t len);

SP_EXPORT size_t
sp_msgpack_enc_binary (void *buf, uint32_t len);

SP_EXPORT size_t
sp_msgpack_enc_array (void *buf, uint32_t count);

SP_EXPORT size_t
sp_msgpack_enc_map (void *buf, uint32_t count);

SP_EXPORT size_t
sp_msgpack_enc_ext (void *buf, int8_t type, uint32_t len);

#endif

