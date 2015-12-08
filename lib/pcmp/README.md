# pcmp

Simple library for utilizing the `pcmpestri` SSE 4.2 instruction. A non-sse variant is provided to simplify use across platforms.

## Testing

To run the test suite:

    make test

## Functions

### `pcmp_eq`

Compares the case-sensitive equality of two strings.

```c
int pcmp_eq (const uint8_t *restrict s1, const uint8_t *restrict s2, int n);
```

### `pcmp_leq`

Compares the equality of a mixed case string to a lower case string.

```c
int pcmp_leq (const uint8_t *restrict s1, const uint8_t *restrict s2, int n);
```

### `pcmp_range`

Finds the the first character matching the range string.

```c
const uint8_t *pcmp_range (const uint8_t *restrict cmp, int clen, const uint8_t *restrict rng, int rlen);
```

The range string is a sequence of character pairs where the range is a value inclusively between the pair of characters.

### `pcmp_set`

Finds the the first character matching any character in the set string.

```c
const uint8_t *pcmp_set (const uint8_t *restrict cmp, int clen, const uint8_t *restrict set, int slen);
```