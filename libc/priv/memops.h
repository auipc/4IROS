#ifndef _LIBC_PRIV_MEMOPS_H
#define _LIBC_PRIV_MEMOPS_H
#pragma once

int memops_init();
void *memops_alloc(size_t size);
void memops_free(void *ptr);
int memops_end();

#endif
