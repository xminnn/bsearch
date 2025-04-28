#pragma once

#include "bsearch.h"

struct bsearch_list;

struct bsearch_list* bsearch_list_create(int block_limit_init, int block_element_limit, int element_size, cmpfunc cmp);
void bsearch_list_put(struct bsearch_list* self, const void* data);
int bsearch_list_get_pos(struct bsearch_list* self, const void* data);
void* bsearch_list_get(struct bsearch_list* self, const void* data);
int bsearch_list_del(struct bsearch_list* self, const void* data);
void bsearch_list_free(struct bsearch_list* self);
int bsearch_list_count(struct bsearch_list* self);