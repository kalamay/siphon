#include "../../include/siphon/vec.h"
#include "../../include/siphon/map.h"
#include "../../include/siphon/hash.h"
#include "../../include/siphon/alloc.h"

struct SpHttpMap {
	SpMap map;
	size_t encode_size;
	size_t scatter_count;
};

struct SpHttpEntry {
	uint16_t **values;
	uint16_t len;
} __attribute__((packed));

static void
pstr_assign (uint16_t *s, const void *val, uint16_t len)
{
	*s = len;
	uint8_t *buf = (uint8_t *)(s+1);
	memcpy (buf, val, len);
	buf[len] = 0;
}

static uint16_t *
pstr_new (const void *val, uint16_t len)
{
	uint16_t *s = sp_malloc (sizeof *s + len + 1);
	if (s != NULL) {
		pstr_assign (s, val, len);
	}
	return s;
}

static void
pstr_free (uint16_t *s)
{
	if (s != NULL) {
		sp_free (s, sizeof *s + *s + 1);
	}
}

static bool
pstr_case_eq (const uint16_t *s, const void *val, uint16_t len)
{
	return *s == len && strncasecmp ((const char *)(s+1), val, len) == 0;
}

static void
pstr_set (const uint16_t *s, struct iovec *iov)
{
	iov->iov_len = *s;
	iov->iov_base = (void *)(s+1);
}

static void
pstr_copy (const uint16_t *s, void *dst)
{
	memcpy (dst, s+1, *s);
}

static bool
entry_iskey (const void *val, const void *key, size_t len)
{
	const SpHttpEntry *e = val;
	return pstr_case_eq (&e->len, key, len);
}

static SpHttpEntry *
entry_new (const void *name, uint16_t len)
{
	SpHttpEntry *e = sp_malloc (sizeof *e + len + 1);
	if (e != NULL) {
		e->values = NULL;
		pstr_assign (&e->len, name, len);
	}
	return e;
}

static void
entry_free (void *val)
{
	SpHttpEntry *e = val;

	size_t i;
	sp_vec_each (e->values, i) {
		pstr_free (e->values[i]);
	}
	sp_vec_free (e->values);
	sp_free (e, sizeof *e + e->len + 1);
}

static const SpType map_type = {
	.hash = sp_siphash_case,
	.iskey = entry_iskey,
	.free = entry_free
};

SpHttpMap *
sp_http_map_new (void)
{
	SpHttpMap *m = sp_malloc (sizeof *m);
	if (m != NULL) {
		sp_map_init (&m->map, 0, 0.0, &map_type);
		m->encode_size = 0;
		m->scatter_count = 0;
	}
	return m;
}

void
sp_http_map_free (SpHttpMap *m)
{
	if (m != NULL) {
		sp_map_final (&m->map);
		sp_free (m, sizeof *m);
	}
}

int
sp_http_map_put (SpHttpMap *m,
		const void *name, uint16_t nlen,
		const void *value, uint16_t vlen)
{
	assert (m != NULL);
	assert (name != NULL);
	assert (value != NULL);

	uint16_t *s = pstr_new (value, vlen);
	bool new = false;
	SpHttpEntry *e = NULL;
	void **loc;
	int err;

	if (s == NULL) {
		goto err;
	}

	loc = sp_map_reserve (&m->map, name, nlen, &new);
	if (loc == NULL) {
		goto err;
	}

	if (new) {
		e = entry_new (name, nlen);
		if (e == NULL) {
			goto err;
		}
	}
	else {
		e = *loc;
	}

	if (sp_vec_push (e->values, s) < 0) {
		goto err;
	}

	if (new) {
		sp_map_assign (&m->map, loc, e);
	}
	else {
		e = *loc;
	}
	m->scatter_count += 4;
	m->encode_size += nlen + vlen + 4;

	return 0;

err:
	err = errno;
	if (new && e != NULL) {
		entry_free (e);
	}
	pstr_free (s);
	return SP_ESYSTEM (err);
}

bool
sp_http_map_del (SpHttpMap *m, const void *name, uint16_t nlen)
{
	SpHttpEntry *e = sp_map_steal (&m->map, name, nlen);
	if (e == NULL) {
		return false;
	}
	m->scatter_count -= sp_vec_count (e->values) * 4;

	size_t i;
	sp_vec_each (e->values, i) {
		m->encode_size -= e->len + *e->values[i] + 4;
	}
	entry_free (e);
	return true;
}

void
sp_http_map_clear (SpHttpMap *m)
{
	sp_map_clear (&m->map);
	m->scatter_count = 0;
	m->encode_size = 0;
}

const SpHttpEntry *
sp_http_map_get (const SpHttpMap *m, const void *name, uint16_t nlen)
{
	return sp_map_get (&m->map, name, nlen);
}

size_t
sp_http_map_encode_size (const SpHttpMap *m)
{
	assert (m != NULL);

	return m->encode_size;
}

void
sp_http_map_encode (const SpHttpMap *m, void *buf)
{
	assert (m != NULL);
	assert (buf != NULL);

	char *p = buf;
	const SpMapEntry *me;

	sp_map_each (&m->map, me) {
		const SpHttpEntry *e = me->value;
		size_t i;

		sp_vec_each (e->values, i) {
			const uint16_t *s = e->values[i];

			pstr_copy (&e->len, p);
			p += e->len;
			memcpy (p, sep, sizeof sep - 1);
			p += sizeof sep - 1;
			pstr_copy (s, p);
			p += *s;
			memcpy (p, crlf, sizeof crlf - 1);
			p += sizeof crlf - 1;
		}
	}
}

size_t
sp_http_map_scatter_count (const SpHttpMap *m)
{
	assert (m != NULL);

	return m->scatter_count;
}

void
sp_http_map_scatter (const SpHttpMap *m, struct iovec *iov)
{
	assert (m != NULL);
	assert (iov != NULL);

	const SpMapEntry *me;

	sp_map_each (&m->map, me) {
		const SpHttpEntry *e = me->value;
		size_t i;

		sp_vec_each (e->values, i) {
			const uint16_t *s = e->values[i];

			iov[0].iov_base = (void *)(e+1);
			iov[0].iov_len = e->len;
			iov[1].iov_base = (void *)sep;
			iov[1].iov_len = sizeof sep - 1;
			iov[2].iov_base = (void *)(s+1);
			iov[2].iov_len = *s;
			iov[3].iov_base = (void *)crlf;
			iov[3].iov_len = sizeof crlf - 1;
			iov += 4;
		}
	}
}

void
sp_http_entry_name (const SpHttpEntry *e, struct iovec *iov)
{
	if (e == NULL) {
		iov->iov_len = 0;
	}
	else {
		pstr_set (&e->len, iov);
	}
}

size_t
sp_http_entry_count (const SpHttpEntry *e)
{
	return e == NULL ? 0 : sp_vec_count (e->values);
}

bool
sp_http_entry_value (const SpHttpEntry *e, size_t idx, struct iovec *iov)
{
	if (e != NULL && idx < sp_vec_count (e->values)) {
		pstr_set (e->values[idx], iov);
		return true;
	}
	return false;
}

void
sp_http_map_print (const SpHttpMap *m, FILE *out)
{
	if (out == NULL) {
		out = stderr;
	}

	if (m == NULL) {
		fprintf (out, "#<SpHttpMap:(null)>\n");
	}
	else {
		fprintf (out, "#<SpHttpMap:%p> {\n", (void *)m);
		const SpMapEntry *me;
		sp_map_each (&m->map, me) {
			const SpHttpEntry *e = me->value;
			size_t i;
			sp_vec_each (e->values, i) {
				const uint16_t *s = e->values[i];
				fprintf (out, "    %.*s: %.*s\n",
					(int)e->len, (char *)(e+1),
					(int)*s, (char *)(s+1));
			}
		}
		fprintf (out, "}\n");
	}
}
