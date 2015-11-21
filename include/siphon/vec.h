#ifndef SIPHON_VEC_H
#define SIPHON_VEC_H

#include "common.h"
#include <stdlib.h>

#define sp_vec_count(v) \
	sp_vecp_count ((void **)&(v))

#define sp_vec_capacity(v) \
	sp_vecp_capacity ((void **)&(v))

#define sp_vec_clear(v) \
	sp_vecp_clear ((void **)&(v))

#define sp_vec_free(v) \
	sp_vecp_free ((void **)&(v), sizeof *(v))

#define sp_vec_ensure(v, n) \
	sp_vecp_ensure ((void **)&(v), sizeof *(v), n)

#define sp_vec_push(v, val) __extension__ ({                    \
	__typeof (*(v)) sp_sym(tmp) = val;                          \
	sp_vecp_push ((void **)&(v), sizeof *(v), &sp_sym(tmp), 1); \
})

#define sp_vec_pushn(v, in, n) \
	sp_vecp_push ((void **)&(v), sizeof *(v), in, n)

#define sp_vec_pop(v, def) __extension__ ({                    \
	__typeof (*(v)) sp_sym(tmp) = def;                         \
	sp_vecp_pop ((void **)&(v), sizeof *(v), &sp_sym(tmp), 1); \
	sp_sym(tmp);                                               \
})

#define sp_vec_popn(v, out, n) \
	sp_vecp_pop ((void **)&(v), sizeof *(v), out, n)

#define sp_vec_shift(v, def) __extension__ ({                    \
	__typeof (*(v)) sp_sym(tmp) = (def);                         \
	sp_vecp_shift ((void **)&(v), sizeof *(v), &sp_sym(tmp), 1); \
	sp_sym(tmp);                                                 \
})

#define sp_vec_shiftn(v, out, n) \
	sp_vecp_shift ((void **)&(v), sizeof *(v), out, n)

#define sp_vec_splice(v, start, end, in, n) \
	sp_vecp_splice ((void **)&(v), sizeof *(v), start, end, in, n)

#define sp_vec_insert(v, start, in, n) __extension__ ({ \
	size_t sp_sym(tmp) = start;                         \
	sp_vec_splice (v, sp_sym(tmp), sp_sym(tmp), in, n); \
})

#define sp_vec_remove(v, start, end) \
	sp_vec_splice (v, start, end, NULL, 0)

#define sp_vec_reverse(v) do {                          \
	__typeof (v) sp_sym(tmp) = (v);                     \
	size_t cnt = sp_vec_count (sp_sym(tmp)), n = cnt/2; \
	for (size_t a = 0, b = cnt-1; a < n; a++, b--) {    \
		__typeof (*v) tmp = sp_sym(tmp)[a];             \
		sp_sym(tmp)[a] = sp_sym(tmp)[b];                \
		sp_sym(tmp)[b] = tmp;                           \
	}                                                   \
} while (0)

#define sp_vec_sort(v, cmp) do {                                         \
	__typeof (v) sp_sym(tmp) = (v);                                      \
	qsort (sp_sym(tmp), sp_vec_count (sp_sym(tmp)), sizeof *(v), (cmp)); \
} while (0)

#define sp_vec_each(v, i)                                                 \
	for (size_t sp_sym(n)=((i)=0, sp_vec_count(v)); (i)<sp_sym(n); (i)++) \

SP_EXPORT size_t
sp_vecp_count (void **vec);

SP_EXPORT size_t
sp_vecp_capacity (void **vec);

SP_EXPORT void
sp_vecp_clear (void **vec);

SP_EXPORT void
sp_vecp_free (void **vec, size_t size);

SP_EXPORT int
sp_vecp_ensure (void **vec, size_t size, size_t nitems);

SP_EXPORT int
sp_vecp_push (void **vec, size_t size, const void *el, size_t nitems);

SP_EXPORT int
sp_vecp_pop (void **vec, size_t size, void *el, size_t nitems);

SP_EXPORT int
sp_vecp_shift (void **vec, size_t size, void *el, size_t nitems);

SP_EXPORT int
sp_vecp_splice (void **vec, size_t size,
		ssize_t start, ssize_t end, /* range to replace */
		void *el, size_t nitems);

SP_EXPORT void
sp_vecp_reverse (void **vec, size_t size, void *tmp);

#endif

