#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <dlfcn.h>

#define LIBC_PATH       "/lib/libc.so.6"

DIR     *opendir(const char *name) //our hijacked opendir
{
  void  *handle; 
  void  *(*sym)(const char *name);  //function pointer

  handle = dlopen(LIBC_PATH, RTLD_LAZY); //get the handle from libc.so.6
  sym = (void *) dlsym(handle, "opendir"); //get the address of opendir symbol in libc.so.6
  printf("OPENDIR HIJACKED -orig- = %08X .::. -param- = %s \n", sym, name);
  return (sym(name)); //return real opendir
}
