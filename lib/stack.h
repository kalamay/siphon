#define STACK_MAX(p) \
	(sizeof (p->stack)*8 - 1)

#define STACK_PUSH_FALSE(p, e) do {                 \
	if (p->depth == STACK_MAX (p)) YIELD_ERROR (e); \
	p->stack[p->depth/8] &= ~(1 << (p->depth%8));   \
	p->depth++;                                     \
} while (0)

#define STACK_PUSH_TRUE(p, e) do {                  \
	if (p->depth == STACK_MAX (p)) YIELD_ERROR (e); \
	p->stack[p->depth/8] |= 1 << (p->depth%8);      \
	p->depth++;                                     \
} while (0)

#define STACK_TOP(p) \
	(!!(p->stack[(p->depth-1)/8] & (1 << ((p->depth-1)%8))))

#define STACK_POP(p, c) \
	(p->depth > 0 && STACK_TOP(p) == (c) ? (--p->depth, 1) : 0)

