#include <stdio.h>
#include <dlfcn.h>
#include <setjmp.h>
#include <signal.h>

#include <list>

// obtain a list of currently open file handles by wrapping:
// fopen
// fdopen
// freopen
// fclose

// check for readability of a string (using setjump/longjmp/signal approach)
static bool check_read_str(const char *s) {
}

// check read-/writeability of a file handle (using setjump/longjmp/signal approach)
static bool check_readwrite_FILE(FILE *f) {
}

// WRAP fputs to behave robust
