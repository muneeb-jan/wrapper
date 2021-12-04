#include <stdio.h>
#include <dlfcn.h>
#include <setjmp.h>
#include <signal.h>
#include <iostream>

#include <list>
#include <set>

// obtain a list of currently open file handles by wrapping:
// fopen
// fdopen
// freopen
// fclose

typedef int (*Func_fputs)(const char* str, FILE* f);
typedef FILE* (*Func_fopen)(const char *path, const char* mode);
typedef FILE* (*Func_fdopen)(int fd, const char *mode);
typedef FILE* (*Func_freopen)(const char *path, const char *mode, FILE *f);
typedef int (*Func_fclose)(FILE* f);

using namespace std;
static jmp_buf jumpBuffer;

set<FILE*> setOfOpenedFiles;

FILE* fopen(const char *path, const char* mode)
{
    Func_fopen org_fopen = (Func_fopen)dlsym (RTLD_NEXT, "fopen");
    FILE* newFile = org_fopen(path, mode);
    setOfOpenedFiles.insert(newFile);
    return newFile;
}

FILE* fdopen(int fd, const char* mode)
{
    Func_fdopen org_fdopen = (Func_fdopen)dlsym (RTLD_NEXT, "fdopen");
    FILE* newFile = org_fdopen(fd, mode);
    setOfOpenedFiles.insert(newFile);
    return newFile;
}

FILE* freopen(const char *path, const char *mode, FILE *f)
{
    Func_freopen org_freopen = (Func_freopen)dlsym (RTLD_NEXT, "freopen");
    FILE* newFile = org_freopen(path, mode, f);
    setOfOpenedFiles.insert(newFile);
    return newFile;
}

int fclose(FILE* f)
{
    Func_fclose org_fclose = (Func_fclose)dlsym (RTLD_NEXT, "fclose");
    setOfOpenedFiles.erase(f);
    return org_fclose(f);
}

void signalHandler( int signum ) {
   longjmp(jumpBuffer, 1);
}

// check for readability of a string (using setjump/longjmp/signal approach)
static bool check_read_str(const char *s) {
    
    if (setjmp(jumpBuffer)==0)
    {
        int i=0;
        while(*s != '\0')
        {
            i++; ++s;
        }
        if (*s !='\0')
            raise(11);
    }
    else
    {
        signal(SIGSEGV,SIG_DFL);
        return false;
    }
    
    return true;

}

// check read-/writeability of a file handle (using setjump/longjmp/signal approach)
static bool check_readwrite_FILE(FILE *f) {
    

    char *buffer;
    if (setjmp(jumpBuffer) == 0)
    {
        long size;
        fseek(f,0, SEEK_END);
        size = ftell(f);
        fseek(f,0, SEEK_SET);
        buffer = (char*) malloc(sizeof(char)*size);
        long rsize = fread(buffer, 1, size, f);
        if(rsize != size)
            raise(11);
        
        long wsize = fwrite(buffer, 1, size, f);
        if(wsize != size)
            raise(11);
        
    }
    else
    {
        signal(SIGSEGV, SIG_DFL);
        return false;
    }
    return true;
}

// WRAP fputs to behave robust


int fputs(const char *str, FILE *f)
{
    signal(SIGSEGV,signalHandler);
    bool file_check, str_check;
    if (setOfOpenedFiles.find(f) != setOfOpenedFiles.end())
    {
        Func_fputs org_fputs = (Func_fputs)dlsym (RTLD_NEXT, "fputs");
        file_check = check_readwrite_FILE(f);
        str_check = check_read_str(str);

        if (file_check && str_check)
        {
            return org_fputs(str, f);
        }
    }

    return EOF;
}