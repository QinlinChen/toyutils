#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <dirent.h>

#define MAX_COMM_LEN 270
#define MAXLINE 1024
#define MAXSIZE 512

struct child;

typedef struct proc {
    char comm[MAX_COMM_LEN];
    pid_t pid;
    struct child *children;
    struct proc *parent;
    struct proc *next;
} PROC;

typedef struct child {
    PROC *child;
    struct child *next;
} CHILD;

struct {
    /* options indicator */
    int show_pids;
    int numeric_sort;
    /* process pool */
    PROC *list;
    /* stacks used for print tree */
    int comm_len[MAXSIZE];
    int is_last_child[MAXSIZE];
    int size;
} global;

/* proc handle functions */
PROC *new_proc(const char *comm, pid_t pid);
PROC *find_proc(pid_t pid);
int proc_comp(PROC *left, PROC *right);
void add_child(PROC *parent, PROC *child);
void add_proc(const char *comm, pid_t pid, pid_t ppid);
void recursive_read_proc(const char *name, pid_t parent, int is_thread);
void recursive_read_procs(const char *path, pid_t parent, int is_thread);
void read_procs() { recursive_read_procs("/proc", 0, 0); }

/* indent stack handle functions */
void push_indent(int comm_len, int is_last_child);
void pop_indent();

/* main functions */
void show_usage(FILE *stream, const char *comm);
void show_version();
void recursive_show_tree(PROC *node);
void show_tree() { global.size = 0; recursive_show_tree(find_proc(1)); }

int main(int argc, char *argv[]) {
    int opt;

    /* initialize global */
    global.show_pids = 0;
    global.numeric_sort = 0;
    global.list = NULL;

    /* process args */
    while (1) {
        static const char *optstring = "pnV";
        static const struct option longopts[] = {
            { "show-pids", no_argument, NULL, 'p' },
            { "numeric-sort", no_argument, NULL, 'n' },
            { "version", no_argument, NULL, 'V' },
            { NULL, no_argument, NULL, 0 }
        };

        opt = getopt_long(argc, argv, optstring, longopts, NULL);
        if (opt == -1)
            break;

        switch (opt) {
            case 'p': global.show_pids = 1; break;
            case 'n': global.numeric_sort = 1; break;
            case 'V': show_version(); break;
            /* 0, ?, etc. */
            default: show_usage(stderr, argv[0]);
        }
    }

    read_procs();
    show_tree();

    /* leave resource for OS to free */
    /* such as global.list */
    exit(EXIT_SUCCESS);
}

PROC *find_proc(pid_t pid) {
    PROC *walk;

    for (walk = global.list; walk; walk = walk->next)
        if (walk->pid == pid)
            break;

    return walk;
}

PROC *new_proc(const char *comm, pid_t pid) {
    PROC *proc = (PROC *)malloc(sizeof(*proc));

    strcpy(proc->comm, comm);
    proc->pid = pid;
    proc->parent = NULL;
    proc->children = NULL;
    proc->next = global.list;
    global.list = proc;

    return proc;
}

int proc_comp(PROC *left, PROC *right) {
    /* pid first */
    if (global.numeric_sort) {
        if (left->pid < right->pid)
            return -1;
        if (left->pid > right->pid)
            return 1;
        return strcmp(left->comm, right->comm);
    }
    /* strcmp first */
    int cmp = strcmp(left->comm, right->comm);
    if (cmp == 0) {
        if (left->pid < right->pid)
            return -1;
        if (left->pid > right->pid)
            return 1;
        return 0;
    }
    return cmp;
}

void add_child(PROC *parent, PROC *child) {
    CHILD *new, **walk;

    new = malloc(sizeof(*new));
    new->child = child;
    for (walk = &parent->children; *walk; walk = &(*walk)->next)
        if (proc_comp((*walk)->child, child) >= 0)
            break;

    new->next = *walk;
    *walk = new;
}

void add_proc(const char *comm, pid_t pid, pid_t ppid) {
    PROC *this, *parent;

    this = find_proc(pid);
    if (!this)
        this = new_proc(comm, pid);
    else
        strcpy(this->comm, comm);

    if (pid == ppid)
        ppid = 0;

    parent = find_proc(ppid);
    if (!parent)
        parent = new_proc("?", ppid); /* '?' is a placeholder */

    add_child(parent, this);
    this->parent = parent;
}

void recursive_read_proc(const char *path, pid_t parent, int is_thread) {
    FILE *stat_file;
    int pid, ppid;
    char state;     /* ignored */
    char comm[MAX_COMM_LEN];
    char buf[MAXLINE];

    sprintf(buf, "%s/stat", path);
    if (!access(buf, F_OK)) {

        if ((stat_file = fopen(buf, "r")) == NULL) {
            perror("fopen error");
            exit(EXIT_FAILURE);
        }

        assert(fgets(buf, MAXLINE, stat_file) != NULL);

        if (fclose(stat_file) != 0) {
            perror("fclose error");
            exit(EXIT_FAILURE);
        }

        sscanf(buf, "%d %s %c %d", &pid, comm, &state, &ppid);
        // printf("%s %d %d %d\n", path, pid, parent, is_thread);
        /* ignore main thread */
        if (!is_thread) {
            /* discard parentheses */
            comm[strlen(comm) - 1] = '\0';
            add_proc(comm + 1, pid, ppid);
        }
        else if(pid != parent) {
            comm[0] = '{';
            comm[strlen(comm) - 1] = '}';
            add_proc(comm, pid, parent);
        }
    }

    sprintf(buf, "%s/task", path);
    if (!access(buf, F_OK))
        recursive_read_procs(buf, pid, 1);
}

void recursive_read_procs(const char *path, pid_t parent, int is_thread) {
    char buf[MAXLINE];
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(path)) == NULL) {
        perror("opendir error");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (!(entry->d_name[0] >= '0' && entry->d_name[0] <= '9'))
            continue;
        sprintf(buf, "%s/%s", path, entry->d_name);
        recursive_read_proc(buf, parent, is_thread);
    }

    if (closedir(dir) < 0) {
        perror("closedir error");
        exit(EXIT_FAILURE);
    }
}

void push_indent(int comm_len, int is_last_child) {
    assert(global.size < MAXSIZE);
    global.comm_len[global.size] = comm_len;
    global.is_last_child[global.size] = is_last_child;
    global.size++;
}

void pop_indent() {
    assert(global.size > 0);
    global.size--;
}

void show_usage(FILE *stream, const char *comm) {
    fprintf(stream, "Usage: %s [-p, --show-pids] "
        "[-n, --numeric-sort] [-V, --version]\n", comm);
    exit(EXIT_FAILURE);
}

void show_version() {
    fprintf(stderr, "pstree 0.1\n"
        "Copyright (C) 2018 Qinlin Chen\n\n"
        "This is free software with NO WARRANTY, but you can't copy it "
        "for your PROGRAMMING ASSIGNMENT of any courses!\n");
    exit(EXIT_SUCCESS);
}

void recursive_show_tree(PROC *node) {
    assert(node);
    char comm[MAX_COMM_LEN];
    int commlen, is_last_child, count, i;
    CHILD *walk;

    if (global.show_pids)
        sprintf(comm, "%s(%d)", node->comm, node->pid);
    else
        sprintf(comm, "%s", node->comm);
    commlen = strlen(comm);
    printf("%s", comm);

    count = 0;
    for (walk = node->children; walk; walk = walk->next) {
        ++count;
        is_last_child = (walk->next == NULL);

        if (count == 1)
            printf(is_last_child ? "---" : "-+-");
        else {
            for (i = 0; i < global.size; ++i)
                printf("%*s", global.comm_len[i] + 3,
                    (global.is_last_child[i] ? "   " : " | "));
            printf("%*s", commlen + 3,
                (is_last_child ? " `-" : " |-"));
        }

        push_indent(commlen, is_last_child);
        recursive_show_tree(walk->child);
        pop_indent();
    }
    if (count == 0)
        printf("\n");
}
