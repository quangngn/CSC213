#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <time.h>

#define PAGE_SIZE 0x1000
#define ROUND_UP(x, y) ((x) % (y) == 0 ? (x) : (x) + ((y) - (x) % (y)))

/// The number of times we've seen each letter in the input, initially zero
size_t letter_counts[26] = {0};

typedef struct args {
    void *start;
    void *end;
} myargs_t;

/**
 * Thread worker. This function handle reading a chunk of the text in file_data from args->start to (exclude) args->end
 */
void *count_letters_worker(void *args) {
    int *worker_counts = malloc(sizeof(int) * 26);
    void *start = ((myargs_t *) args)->start;
    void *end = ((myargs_t *) args)->end;

    /// count letters from start to end (exclude)
    while (start < end) {
        // read char at start and count accordingly
        char c = *((char *) start);
        if (c >= 'a' && c <= 'z') {
            worker_counts[c - 'a']++;
        } else if (c >= 'A' && c <= 'Z') {
            worker_counts[c - 'A']++;
        }
        // update start pointer;
        start = (void *) ((char *) start + 1);
    }
    printf("f is %d\n", worker_counts[5]);
    return worker_counts;
}


/**
 * This function should divide up the file_data between the specified number of
 * threads, then have each thread count the number of occurrences of each letter
 * in the input data. Counts should be written to the letter_counts array. Make
 * sure you count only the 26 different letters, treating upper- and lower-case
 * letters as the same. Skip all other characters.
 *
 * \param num_threads   The number of threads your program must use to count
 *                      letters. Divide work evenly between threads
 * \param file_data     A pointer to the beginning of the file data array
 * \param file_size     The number of bytes in the file_data array
 */
void count_letters(int num_threads, char *file_data, off_t file_size) {
    /// create an array of threads
    pthread_t threads[num_threads];
    myargs_t args_arr[num_threads];

    /// distribute the work to each worker
    // we divide the work equally to each worker. if the file_size < the amount of thread, we don't use all the thread.
    // instead, we set num_threads = file_size and each thread will work with only 1 character.
    off_t work_load = file_size / num_threads;
    off_t leftover = file_size % num_threads;
    if (work_load == 0) {
        num_threads = file_size;
        work_load = 1;
        leftover = 0;
    }
    void *start = file_data;
    for (int i = 0; i < num_threads; i++) {
        // set the start point
        args_arr[i].start = start;
        if (i == num_threads - 1) {
            args_arr[i].end = (void *) (start + work_load + leftover);
        } else {
            args_arr[i].end = (void *) (start + work_load);
        }

        // start a thread
        pthread_create(((pthread_t *) threads) + i, NULL, count_letters_worker, ((myargs_t *) args_arr) + i);

        // update start pointer
        start = args_arr[i].end;
    }

    /// wait the worker to finish the job
    for (int i = 0; i < num_threads; i++) {
        pthread_mutex_t lock;
        pthread_mutex_lock(&lock);
        int *worker_counts;
        pthread_join(threads[i], (void **) &worker_counts);
        for (int j = 0; j < 26; j++) {
            letter_counts[j] += worker_counts[j];
        }

        // free worker_count
        free(worker_counts);
        worker_counts = NULL;
        pthread_mutex_unlock(&lock);
    }

}

/**
 * Show instructions on how to run the program.
 * \param program_name  The name of the command to print in the usage info
 */
void show_usage(char *program_name) {
    fprintf(stderr, "Usage: %s <N> <input file>\n", program_name);
    fprintf(stderr, "    where <N> is the number of threads (1, 2, 4, or 8)\n");
    fprintf(stderr, "    and <input file> is a path to an input text file.\n");
}

int main(int argc, char **argv) {
    // Check parameter count
    if (argc != 3) {
        show_usage(argv[0]);
        exit(1);
    }

    // Read thread count
    int num_threads = atoi(argv[1]);
    if (num_threads != 1 && num_threads != 2 && num_threads != 4 && num_threads != 8) {
        fprintf(stderr, "Invalid number of threads: %s\n", argv[1]);
        show_usage(argv[0]);
        exit(1);
    }

    // Open the input file
    int fd = open(argv[2], O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Unable to open input file: %s\n", argv[2]);
        show_usage(argv[0]);
        exit(1);
    }

    // Get the file size
    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == -1) {
        fprintf(stderr, "Unable to seek to end of file\n");
        exit(2);
    }

    // Seek back to the start of the file
    if (lseek(fd, 0, SEEK_SET)) {
        fprintf(stderr, "Unable to seek to the beginning of the file\n");
        exit(2);
    }

    // Load the file with mmap
    char *file_data = mmap(NULL, ROUND_UP(file_size, PAGE_SIZE), PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_data == MAP_FAILED) {
        fprintf(stderr, "Failed to map file\n");
        exit(2);
    }

    // Call the function to count letter frequencies
    count_letters(num_threads, file_data, file_size);

    // Print the letter counts
    for (int i = 0; i < 26; i++) {
        printf("%c: %lu\n", 'a' + i, letter_counts[i]);
    }

    return 0;
}
