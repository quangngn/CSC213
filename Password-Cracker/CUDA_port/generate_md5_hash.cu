#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gpu_md5.cu"

#define PASSWORD_LENGTH 6

typedef union hcu {
    unsigned char b[MD5_UNSIGNED_HASH_LEN];
} HCunion;

// This program take a string and return a hashcode for it. 

__global__ void get_md5_hashcode(unsigned char *password, int password_len, unsigned char *hash_code) {
    md5((unsigned char*) password, PASSWORD_LENGTH, hash_code);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("The program require an argument, a string, to work!\n");
        exit(1);
    }

    if (strlen(argv[1]) != PASSWORD_LENGTH) {
        printf("The password's length must be %d\n", PASSWORD_LENGTH);
        exit(1);
    }

    unsigned char *hash_code;
    cudaMallocManaged(&hash_code, sizeof(unsigned char) * (MD5_UNSIGNED_HASH_LEN + 1));

    char *gpu_password;
    cudaMalloc(&gpu_password, sizeof(char) * (PASSWORD_LENGTH + 1));
    cudaMemcpy(gpu_password, argv[1], sizeof(char) * (PASSWORD_LENGTH + 1), cudaMemcpyHostToDevice);

    get_md5_hashcode<<<1,1>>>((unsigned char *) gpu_password, PASSWORD_LENGTH, hash_code);
    cudaDeviceSynchronize();

    // print the pass code in hex form
    HCunion h;
    memcpy(&(h.b), (const char *)hash_code, sizeof(unsigned char) * strlen(hash_code));

    for (int i = 0; i < MD5_UNSIGNED_HASH_LEN; i++) {
        printf("%02x", h.b[i]);
    }
    printf("\n");

    cudaFree(hash_code);
    cudaFree(gpu_password);

    return 0;
}