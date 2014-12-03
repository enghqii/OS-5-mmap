/*
 * OS Assignment #5
 *
 * @file person.c
 */

#include "person.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>

Person *    p_mmap = 0;
int         fd = -1;

static void print_usage (const char *prog)
{
    fprintf (stderr, "usage: %s [-f file] [-w] [-s value] attr_name\n", prog);
    fprintf (stderr, "  -f file  : person file, default is './person.dat'\n");
    fprintf (stderr, "  -w       : watch mode\n");
    fprintf (stderr, "  -s value : set the value for the given 'attr_name'\n");
}

int open_file(const char* filename)
{
    int fd = open(filename, O_RDWR, 0644);
    printf("file open : %s\n", filename);

    if( 0 >= fd )
    {
        Person* p = calloc(1, sizeof(Person));
        fd = open(filename, O_RDWR | O_CREAT, 0644);
        write(fd, p, sizeof(Person));

        printf("file fill with 0x00 : %s\n", filename);
    }

    return fd;
}

void setup(char * file_name)
{
    fd = open_file(file_name);

    p_mmap = (Person *)mmap(
            (void *)0,
            sizeof(Person),
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            fd,
            0);
}

void cleanup()
{
    munmap(p_mmap, sizeof(Person));
    close(fd);
}

void print_person(Person* p)
{
    printf("name \t%s\n",p->name);
    printf("age \t%d\n",p->age);
    printf("gender \t%d\n",p->gender);
    printf("phone \t%s\n",p->phone);
    printf("homepage \t%s\n",p->homepage);
    printf("twitter \t%s\n",p->twitter);
    printf("facebook \t%s\n",p->facebook);
}

void sig_handler(int signo, siginfo_t *si)
{
    switch(signo)
    {
        case SIGUSR1:
            printf("offset received [%d]\n",si->si_value.sival_int);
            break;

        case SIGINT:
        case SIGTERM:
            printf("sig int or term\n");

            cleanup();
            exit(0);
            break;
    }
}

void send_signal(int offset)
{
    int i = 0;
    union sigval val;
    val.sival_int = offset;

    for(i = 0; i < NOTIFY_MAX * sizeof(pid_t); i += sizeof(pid_t))
    {
        pid_t* p_pid = (pid_t *)((char *)p_mmap + i);
        if( *p_pid != 0 )
        {
            if( sigqueue(*p_pid, SIGUSR1, val) != 0)
            {
                // error!
                *p_pid = 0;
            }
        }
    }
}

int main (int    argc, char **argv)
{
    const char *file_name;
    const char *set_value;
    const char *attr_name;
    int         watch_mode;

    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags     = SA_SIGINFO;
    sigact.sa_restorer  = NULL;
    sigact.sa_sigaction = sig_handler;

    sigaction(SIGUSR1, &sigact, 0);
    sigaction(SIGINT, &sigact, 0);
    sigaction(SIGTERM, &sigact, 0);

    /* Parse command line arguments. */
    file_name  = "./person.dat";
    set_value  = NULL;
    watch_mode = 0;

    while (1)
    {
        int opt;

        opt = getopt (argc, argv, "ws:f:");
        if (opt < 0)
            break;

        switch (opt)
        {
            case 'w':
                watch_mode = 1;
                break;
            case 's':
                set_value = optarg;
                break;
            case 'f':
                file_name = optarg;
                break;
            default:
                print_usage (argv[0]);
                return -1;
        }
    }
    if (!watch_mode && optind >= argc)
    {
        print_usage (argv[0]);
        return -1;
    }
    attr_name = argv[optind];

    /* ###################################################### */

    setup(file_name);

    if(watch_mode) // watch mode 
    {
        int i = 0;

        printf("waiting...\n");
        printf("my pid is %x\n", getpid());

        // add self on watchers list
        for(i = 0; i < NOTIFY_MAX * sizeof(pid_t); i += sizeof(pid_t))
        {
            pid_t* p_pid = (pid_t *)((char *)p_mmap + i);

            if( *p_pid == 0 )
            {
                *p_pid = getpid();
                break;
            }
        }

        if(i/sizeof(pid_t) == NOTIFY_MAX)
        {
            *((pid_t *)p_mmap) = getpid();
        }

        while(1){
            sleep(2);
        }
    }

    else // not watch_mode
    {
        int         off;
        char *      data;

        if( 0 > (off = person_get_offset_of_attr(attr_name)) )
        {
            fprintf(stderr, "invalid attr name \'%s\'\n", attr_name);
            return -1;
        }

        if(set_value != NULL) // set mode
        {
            printf("attr name is %s(%d)\n", attr_name, off);

            if( person_attr_is_integer(attr_name) )
            {
                int * addr = (int *)(((char *)p_mmap) + off);

                *addr = atoi(set_value);
                msync(p_mmap, sizeof(int), MS_SYNC);
            }
            else
            {
                char* addr = ((char *)p_mmap) + off;
                int len = strlen(set_value) * sizeof(char) + 1;

                memcpy(addr, set_value, len);
                msync(p_mmap, sizeof(Person), MS_SYNC);
            }
            send_signal(off);
        }

        else // print mode
        {
            if( person_attr_is_integer(attr_name) )
            {
                int * addr = (int *)(((char *)p_mmap) + off);
                printf("%d\n", *addr);
            }
            else
            {
                char* addr = ((char *)p_mmap) + off;
                printf("%s\n", addr);
            }
        }
        print_person(p_mmap);
    }

    cleanup();
    return 0;
}