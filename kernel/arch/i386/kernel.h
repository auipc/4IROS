#pragma once

#define KB 1024
#define MB 1024 * KB

#define VIRTUAL_ADDRESS 0xC0000000
#define PAGE_SIZE 4096

// TODO this shouldn't be static
#define TOTAL_MEMORY 128 * MB

#define PACKED __attribute__((packed))
