# Siphon [![Build Status](https://travis-ci.org/imgix/siphon.png?branch=master)](https://travis-ci.org/imgix/siphon)

Siphon is a library of highly optimized parsers for common protocol and data
formats. The design goal is to give as much control as possible to the
caller while maintaining a minimal interface. Additionally, it should provide
a flexible interface for binding to from other languages. Most of this code
was developed while binding it to LuaJITs FFI.

Most of the parsers use a pull model: that is, the caller requests the next
value from an input buffer. This allows the caller to manage the continuation
of the parser as well as the memory used for the tokens.

The library aims to make little to no memory allocations and instead prefers that
the caller maintain the input buffer until a value may be extracted from it. The
parser will not extract the value itself, but instead provide the bounds for the
value within the buffer.

Similarly, for plain byte sequences (HTTP bodies, MsgPack strings, binary and
extension values), siphon will require that the caller handle the value. This
requires a little more work, but it enables much more powerful usages. 
Particularly this enables mixing of zero-copy primatives with the parser. For
example, a process could use `splice(2)` to transfer a msgpack string from a
back-end parser, to an HTTP response body, or splice an incoming HTTP request
directly to disk.

For a quick HTTP example:

```C
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <siphon/siphon.h>

static int fd = STDIN_FILENO;
static char buf[8192]; // buffer to read into
static size_t end = 0; // last byte position in the buffer
static size_t off = 0; // offset into the buffer

// reads more from fd into buffer
static void read_more (void);

// reads raw body bytes from the buffer and/or socket
static void read_raw (size_t len);

int
main (void)
{
	SpHttp p;
	sp_http_init_request (&p);

	while (!sp_http_is_done (&p)) {
		ssize_t rc = sp_http_next (&p, buf+off, end-off);
		// TODO: handle parser errors gracefully
		if (rc < 0) sp_exit (rc, EXIT_FAILURE);

		// could not parse a token so read more
		if (rc == 0) {
			read_more ();
			continue;
		}

		// TODO: do something with the token
		sp_http_print (&p, buf+off, stdout);

		// mark the used range of the buffer
		off += rc;

		// handle body values
		if (p.type == SP_HTTP_BODY_START) {
			if (!p.as.body_start.chunked) {
				read_raw (p.as.body_start.content_length);
			}
		}
		else if (p.type == SP_HTTP_BODY_CHUNK) {
			read_raw (p.as.body_chunk.length);
		}
	}

	return 0;
}

void
read_more (void)
{
	if (off == end) {
		// nothing to move so reset
		end = off = 0;
	}
	else if (end > sizeof (buf) / 2) {
		// reclaim buffer space if more than half is used
		memmove (buf, buf+off, end-off);
		end -= off;
		off = 0;
	}

	// read more at the end of the buffer
	ssize_t n = read (fd, buf+end, sizeof (buf) - end);
	if (n < 0) sp_exit (errno, EXIT_FAILURE);

	// push out end position
	end += n;
}

void
read_raw (size_t len)
{
	while (len > 0) {
		// only process the amount in the buffer
		size_t amt = len;
		if (amt > end - off) amt = end - off;

		// read more if nothing is availble
		if (amt == 0) {
			read_more ();
			continue;
		}

		// write out the raw bytes
		fwrite (buf+off, 1, amt, stdout);
		fflush (stdout);

		len -= amt; // update number of raw bytes remaining
		off += amt; // update buffer offset position
	}
}
```

# Building

Siphon currently uses [`cmake(1)`](https://cmake.org) for building and installing.

## Release Build

For a typical release build and install cycle, follow these steps:

```bash
cmake -H. -Bbuild/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
cd build/release
ctest --output-on-failure
cd ../..
cmake --build build/release --target install
```

### Build Options

#### Build Tool

The default build tool for unix systems is `make(1)`. While the build is
reasonably quick, it is faster to use a build tool like
[ninja](https://martine.github.io/ninja/). To use ninja, change the target
configuration command to:

```bash
cmake -H. -Bbuild/release -DCMAKE_BUILD_TYPE=Release -G Ninja
```

#### Project and Target Directories

This build assumes the current working directory is the root of the siphon
project. To work from a different directory, specifiy the path to the siphon
project root with the `-H` option.

For a different target directory, change the value of the `-B` and `--build`
options used the various commands.

#### Install Prefix

The default install prefix is `/usr/local`, to change this, define a value for
`CMAKE_INSTALL_PREFIX` in the target setup command. For example:

```bash
cmake -H. -Bbuild/release -DCMAKE_INSTALL_PREFIX=$HOME/opt -DCMAKE_BUILD_TYPE=Release
```

## Debug Build

The debug build is preferrable for devlopment and more thourough testing purposes.
These steps assume [`valgrind(1)`](http://valgrind.org) is installed.

```bash
cmake -H. -Bbuild/debug -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build/debug
cd build/debug
ctest --output-on-failure -D ExperimentalMemCheck
```

## Fuzz Build

In addition to the test suite, the are a set of test tools that simply take
input on `stdin` and and report back success or failure. The fuzz tests use
these tools to attempt producing some input that cuases the tool to either
crash or hang.

Additionally, the memory used for these tests is allocated using the allocator
defined in lib/alloc.c. This allocator is very wasteful on memory, but protects
against buffer over-reads and over-writes. If either occur, the process will
crash and fuzz tester will log the input that cuased the error condition.

Fuzzing is a *very* slow process. A typical run for each fuzz test is at least
24 hours, but for critical changes each test is run for approximately 4 days.

The fuzz tool used here is [`afl-fuzz`](http://lcamtuf.coredump.cx/afl/). If
installed to a custom location, make sure that both `afl-fuzz` and `afl-gcc`
are in your `$PATH`.

Now to build the test programs:

```bash
cmake -H. -Bbuild/fuzz -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_COMPILER=$(which afl-gcc)
# or cmake -H. -Bbuild/fuzz -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_COMPILER=$(which afl-clang)
cmake --build build/fuzz
```

### Run the Fuzz Test

Set the test type as an environment variable:

```bash
export FUZZ=http
```

Currently, this can be either `http`, `json`, `msgpack`, or `uri`. 

Create a directory to capture the results:

```bash
mkdir -p build/fuzz/$FUZZ
```

The `afl-fuzz` tool can be run in parallel. In order to both manage multiple
shells and desire to leave the session and return, it is highly recommended
to use `tmux` or `screen`.

First, start up the main test process:

```bash
afl-fuzz -i test/fuzz/$FUZZ -o build/fuzz/$FUZZ -M fuzz01 ./build/fuzz/test-$FUZZ-input
```

The `-M fuzz01` identifies this as the main process. For each secondary process,
run using the `-S` option instead, and use a unique number.

```bash
afl-fuzz -i test/fuzz/$FUZZ -o build/fuzz/$FUZZ -S fuzz02 ./build/fuzz/test-$FUZZ-input
```

I'll typically run 4-8 of these processes during the test run.

