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

int main (int    argc, char **argv)
{
    const char *file_name;
    const char *set_value;
    const char *attr_name;
    int         watch_mode;

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

    /* not yet implemented */

    {
        Person * p_mmap;
        int fd;

        int off;
        char * data = "what";

        fd = open_file(file_name);

        // obtain memory map here (after this, we don't use fd)
        p_mmap = (Person *)mmap(
            (void *)0, 
            sizeof(Person), 
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            fd,
            0);

        // modify struct's data with mcpy, etc
        // notify watchers (noted in person.watchers) something changed
        off = person_get_offset_of_attr("name");
        memcpy( ((char *)p_mmap) + off, data, strlen(data));

        // msync
        msync(p_mmap, sizeof(Person), MS_SYNC);

        // munmap
        munmap(p_mmap, sizeof(Person));

        close(fd);
    }

    // in watching mode, changed property info is given.

    return 0;
}

