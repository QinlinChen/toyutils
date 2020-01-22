#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <assert.h>
#include <regex.h>
#include <signal.h>

#define MAXLINE 1024
#define MAX_SYSCALL_INFO_SIZE 1024

typedef long long MICROSEC;

typedef struct {
    char *name;
    MICROSEC microsec;
} SYSCALL_INFO;

/* global */
struct {
    SYSCALL_INFO syscall_info[MAX_SYSCALL_INFO_SIZE];
    int ninfo;
} G;

/* utilities */
char *mystrdup(const char *s);
MICROSEC to_microsec(char *s);
char *get_substring(char *str, int size, regmatch_t *match);

/* syscall_info handle functions */
int find_syscall_info(const char *name);
void add_syscall_info(const char *name, MICROSEC microsec);
int cmp_syscall_info(const void *lhs, const void *rhs);
void print_syscall_info();

/* main functions */
void child_do(int writefd, int argc, char *argv[]);
void parent_do(int readfd);

int main(int argc, char *argv[]) {
    int pid, pipefd[2];

    if (argc < 2) {
        printf("Usage: %s COMMAND [ARG]...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* initialize array */
    G.ninfo = 0;

    /* initialize pipe */
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        close(pipefd[0]);       /* close unused read end */
        child_do(pipefd[1], argc, argv);
    } else {
        close(pipefd[1]);       /* close unused write end */
        parent_do(pipefd[0]);
    }

    exit(EXIT_SUCCESS);
}

void child_do(int writefd, int argc, char *argv[]) {
    char **newargv;
    int newargc, i;

    /* redirect STDERR to write end */
    dup2(writefd, STDERR_FILENO);

    /* construct args */
    newargc = 0;
    newargv = (char **)malloc((argc + 2) * sizeof(char *));
    newargv[newargc++] = mystrdup("strace");
    newargv[newargc++] = mystrdup("-T");
    for (i = 1; i < argc; ++i)
        newargv[newargc++] = mystrdup(argv[i]);
    newargv[newargc++] = NULL;
    assert(newargc == argc + 2);

    /* execute */
    execvp(newargv[0], newargv);

    /* should not reach here */
    perror("execvp");
    exit(EXIT_FAILURE);
}

void parent_do(int readfd) {
    char buf[MAXLINE], errbuf[MAXLINE];
    int rc;
    regex_t reg;
    regmatch_t match[3];
    
    /* initialize regex */
    rc = regcomp(&reg, 
        "(\\w+)\\(.*\\)\\s*=.+<([0-9]+\\.[0-9]+)>",
        REG_EXTENDED);
    if (rc != 0) {
        regerror(rc, &reg, errbuf, MAXLINE);
        printf("regex compilation failed: %s\n", errbuf);
        exit(EXIT_FAILURE);
    }

    /* redirect read end to stdin to use stdio functions */
    dup2(readfd, STDIN_FILENO);

    /* read syscall info */
    while (fgets(buf, MAXLINE, stdin) != NULL) 
        if (regexec(&reg, buf, 3, match, 0) == 0) 
            add_syscall_info(get_substring(buf, MAXLINE, &match[1]), 
                to_microsec(get_substring(buf, MAXLINE, &match[2])));   
    
    qsort(G.syscall_info, G.ninfo, sizeof(SYSCALL_INFO), cmp_syscall_info);
    print_syscall_info();
}

char *mystrdup(const char *s) {
    assert(s);
    char *ret = (char *)malloc((strlen(s) + 1) * sizeof(char));
    assert(ret);
    strcpy(ret, s);
    return ret;
}

MICROSEC to_microsec(char *s) {
    MICROSEC us = 0;

    while (*s) {
        if (*s > '0' && *s < '9') {
            us *= 10;
            us += *s - '0';
        }
        s++;
    }

    return us;
}

/* It's a bad implementation just for convenience */
char *get_substring(char *str, int size, regmatch_t *match) {
    assert(match->rm_so != -1);
    assert(match->rm_eo < size);
    str[match->rm_eo] = '\0';
    return str + match->rm_so;
}

int find_syscall_info(const char *name) {
    int i;
    for (i = 0; i < G.ninfo; ++i)
        if (!strcmp(G.syscall_info[i].name, name))
            return i;
    return -1;
}

void add_syscall_info(const char *name, MICROSEC microsec) {
    int pos;

    if ((pos = find_syscall_info(name)) == -1) {
        assert(G.ninfo < MAX_SYSCALL_INFO_SIZE);
        G.syscall_info[G.ninfo].name = mystrdup(name);
        G.syscall_info[G.ninfo].microsec = microsec;
        G.ninfo++;
    } else {
        G.syscall_info[pos].microsec += microsec;
    }
}

int cmp_syscall_info(const void *lhs, const void *rhs) {
    SYSCALL_INFO *l = (SYSCALL_INFO *)lhs;
    SYSCALL_INFO *r = (SYSCALL_INFO *)rhs;

    if (l->microsec < r->microsec)
        return 1;
    if (l->microsec > r->microsec)
        return -1;
    return strcmp(l->name, r->name);
}

void print_syscall_info() {
    int i, maxlen, len;
    MICROSEC sum;

    sum = 0;
    maxlen = 0;
    for (i = 0; i < G.ninfo; ++i) {
        len = strlen(G.syscall_info[i].name);
        maxlen = (len > maxlen ? len : maxlen);
        sum += G.syscall_info[i].microsec;
    }
 
    printf("%-*s %% time\n", maxlen, "syscall");
    for (int i = 0; i < maxlen; ++i)
        putchar('-');
    printf(" ------\n");
    for (i = 0; i < G.ninfo; ++i) 
        printf("%-*s %6.2f\n", maxlen, G.syscall_info[i].name,
            (double)G.syscall_info[i].microsec * 100 / sum);
}