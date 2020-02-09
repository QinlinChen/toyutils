#define _POXIS_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <signal.h>
#include <errno.h>

#define MAXLINE 1024

/* utilities */
void unix_error(const char *msg);
void app_error(const char *msg);
void dl_error(const char *msg);
char *readline(const char *prompt, char *buf, int size, FILE *stream);

/* dynamic load */
typedef int (*func_t)();
int is_func(const char *text);
char *expr_to_func(const char *expr, char *func);
void *load(const char *text, const char *workdir);
int execute(void *handle, const char *symbol);

/* tmp directory */
static const char *tmpdir = "./tmp666";
void mk_tmpdir();
void rm_tmpdir();
void signal_handler(int signum) { rm_tmpdir(); }

int main() {
    char text[MAXLINE], *funcname;
    void *handle;

    mk_tmpdir();
    /* make sure tmpdir will be cleaned up */
    signal(SIGINT, signal_handler);
    atexit(rm_tmpdir);

    while (readline(">>> ", text, MAXLINE, stdin) != NULL) {
        if (strstr(text, "exit"))
            break;

        if (!is_func(text)) {
            funcname = expr_to_func(text, text);
            if ((handle = load(text, tmpdir)) != NULL)
                printf("%d\n", execute(handle, funcname));
        } else {
            handle = load(text, tmpdir);
        }
        if (handle == NULL)
            printf("Failed: invaild input\n");
    }

    return 0;
}

void unix_error(const char *msg) {
    perror(msg);
    exit(1);
}

void app_error(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

void dl_error(const char *msg) {
    fprintf(stderr, "%s: %s\n", msg, dlerror());
    exit(1);
}

char *readline(const char *prompt, char *buf, int size, FILE *stream) {
    char *ret_val, *find;
	struct stat sbuf;

	if (fstat(fileno(stream), &sbuf) != 0)
		unix_error("fstat error");

	if (!S_ISREG(sbuf.st_mode))
        printf("%s", prompt);

    if (((ret_val = fgets(buf, size, stream)) == NULL) && ferror(stream))
        app_error("readline error");

    if (ret_val) {
        find = strchr(buf, '\n');
        if (find)
            *find = '\0';
        else 
            while (fgetc(stream) != '\n')
                continue;
    }

    return ret_val;
}

int is_func(const char *text) {
    char buf[MAXLINE];
    sscanf(text, "%s", buf);
    if (strcmp(buf, "int") != 0) 
        return 0;
    return 1;
} 

char *expr_to_func(const char *expr, char *func) {
    static int id = 0;
    static char name[MAXLINE];
    char body[MAXLINE];

    sprintf(name, "_expr_warp_%04d", id++);
    sprintf(body, "int %s() { return (%s); }", name, expr);
    strcpy(func, body);

    return name;
}

void *load(const char *text, const char *workdir) {
    static int id = 0;
    char template[MAXLINE], filename[MAXLINE], libname[MAXLINE];
    pid_t pid;
    FILE *fp;
    int fd;

    /* create temporary filename and libname */
    sprintf(template, "tmp.%04d", id++);
    sprintf(filename, "%s/%s.c", workdir, template);
    sprintf(libname, "%s/lib%s.so", workdir, template);

    /* write file */
    if ((fp = fopen(filename, "w")) == NULL)
        unix_error("fopen");
    fprintf(fp, "%s", text);
    if (fclose(fp) != 0)
        unix_error("fclose");

    /* compile */
    pid = fork();
    if (pid < 0)
        unix_error("fork");
    if (pid == 0) {
        if ((fd = open("/dev/null", O_WRONLY)) == -1)
            unix_error("open");
        if (dup2(fd, STDERR_FILENO) == -1) 
            unix_error("dup2");
        execlp("gcc", "gcc", "-shared", "-fPIC", 
            "-Wno-implicit-function-declaration",
            "-o", libname, filename, (char *)NULL);
        unix_error("execlp");
    } else {
        wait(NULL);
    }
	
    /* Attention: leave other temp files for rm_tmpdir to clean up */
    if (unlink(filename) == -1)
        unix_error("unlink");
    
    return dlopen(libname, RTLD_GLOBAL | RTLD_LAZY);
}

int execute(void *handle, const char *symbol) {
    func_t func;
    func = (func_t)dlsym(handle, symbol);
    if (dlerror() != NULL)
        dl_error("dlsym");
	return func();
}

void mk_tmpdir() {
    if (mkdir(tmpdir, 0700) == -1 && errno != EEXIST)
        unix_error("mkdir");
}

void rm_tmpdir() {
    char command[MAXLINE];
    sprintf(command, "rm -rf %s", tmpdir);
    if (system(command) != 0) 
        app_error("fail to remove tmpdir");
}
