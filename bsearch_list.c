#include <assert.h>
#include <memory.h>
#include <stdlib.h>

#include "bsearch.h"

struct bsearch_list_block {
    char* arr;
    int arr_count_;
};

struct bsearch_list {
    int element_size;
    int element_limit;
    cmpfunc element_cmp;
    int element_count;

    struct bsearch_list_block* block;    // 数组
    int block_count_;
    int block_limit_;
};

struct bsearch_list* bsearch_list_create(int block_limit_init, int block_element_limit, int element_size, cmpfunc cmp) {
    assert(block_element_limit % 2 == 0);
    struct bsearch_list* self = malloc(sizeof(struct bsearch_list));
    self->element_size = element_size;
    self->element_limit = block_element_limit;
    self->element_cmp = cmp;

    self->block_limit_ = block_limit_init;
    self->block_count_ = 0;
    self->block = malloc(sizeof(struct bsearch_list_block) * self->block_limit_);

    self->element_count = 0;
    return self;
}

struct block_find_compare_data_ {
    struct bsearch_list* self;
    const void* data;
};

static int block_put_cmpfunc_(const void* a, const void* b) {    // 判断是否存在，不存在需要获取可插入位置
    const struct bsearch_list_block* block = a;
    const struct block_find_compare_data_* _b = b;
    const struct bsearch_list* self = _b->self;
    const void* data = _b->data;
    assert(block->arr_count_ != 0);

    const void* min = block->arr;
    const void* max = block->arr + ((block->arr_count_ - 1) * self->element_size);
    const struct bsearch_list_block* next = block + 1;
    if (block->arr_count_ < self->element_limit && block != &self->block[self->block_count_ - 1] && next->arr_count_ > 0) {
        max = next->arr;    // 下一个块的最小就是这个块的最大，则选中
    }
    if (self->element_cmp(data, max) >= 0) {
        if (block->arr_count_ < self->element_limit && block == &self->block[self->block_count_ - 1]) {
            return 0;    // 本块是最后一个块，但是块没有满，则选中
        }
        return -1;
    }
    if (self->element_cmp(data, min) < 0) {
        if (block->arr_count_ < self->element_limit && block == self->block) {
            return 0;    // 本块是第一个块，但是块没有满，则选中
        }
        return 1;
    }
    return 0;
}

static int block_get_cmpfunc_(const void* a, const void* b) {    // 只需要快速判断是否存在
    const struct bsearch_list_block* block = a;
    const struct block_find_compare_data_* _b = b;
    const struct bsearch_list* self = _b->self;
    const void* data = _b->data;
    assert(block->arr_count_ != 0);
    const void* min = block->arr;
    const void* max = block->arr + ((block->arr_count_ - 1) * self->element_size);
    if (self->element_cmp(data, max) > 0) {
        return -1;
    }
    if (self->element_cmp(data, min) < 0) {
        return 1;
    }
    return 0;
}

void bsearch_list_put(struct bsearch_list* self, const void* data) {
    int lp = _binary_search(self->block, self->block_count_, sizeof(struct bsearch_list_block), block_put_cmpfunc_, &(struct block_find_compare_data_){self, data});
    struct bsearch_list_block newblock = {0};
    if (lp < 0) {
        lp = -lp - 1;
        if (lp != 0 && lp != self->block_count_) {
            assert(0);
        }
        newblock.arr = malloc(self->element_size * self->element_limit);
        _bsearch_put(newblock.arr, self->element_size, &newblock.arr_count_, self->element_limit, self->element_cmp, data);
    } else {
        struct bsearch_list_block* block = &self->block[lp];
        _bsearch_put(block->arr, self->element_size, &block->arr_count_, self->element_limit, self->element_cmp, data);

        if (block->arr_count_ == self->element_limit) {    // 达到容量上限, 二分block
            newblock.arr = malloc(self->element_size * self->element_limit);
            int move = self->element_limit / 2;
            memcpy(newblock.arr, block->arr + (move * self->element_size), move * self->element_size);
            newblock.arr_count_ = move;
            block->arr_count_ = move;
            lp += 1;    // 在lp后的后边插入
        }
    }
    if (newblock.arr) {
        if (self->block_count_ >= self->block_limit_) {
            self->block_limit_ *= 2;
            self->block = realloc(self->block, sizeof(struct bsearch_list_block) * self->block_limit_);
        }
        _bsearch_put_pos(self->block, sizeof(struct bsearch_list_block), &self->block_count_, self->block_limit_, lp, &newblock);
    }
    self->element_count += 1;
}

int bsearch_list_get_pos(struct bsearch_list* self, const void* data) {    // 这个可以先二分获得位置，然后直接加，避免多次比较
    int lp = _binary_search(self->block, self->block_count_, sizeof(struct bsearch_list_block), block_put_cmpfunc_, &(struct block_find_compare_data_){self, data});
    if (lp < 0) {
        lp = -lp - 1;
        if (lp == 0) {
            return -0 - 1;
        } else if (lp == self->block_count_) {
            return -self->element_count - 1;
        } else {
            assert(0);
        }
    }
    int pos = 0;
    for (int i = 0; i < lp; i++) {
        pos += self->block[i].arr_count_;
    }
    const struct bsearch_list_block* block = &self->block[lp];
    int p = _bsearch_get_pos(block->arr, self->element_size, block->arr_count_, self->element_cmp, data);
    if (p < 0) {
        pos += (-p - 1);
        return -pos;
    }
    return pos + p;
}

void* bsearch_list_get(struct bsearch_list* self, const void* data) {
    int pos = 0;
    int lp = _binary_search(self->block, self->block_count_, sizeof(struct bsearch_list_block), block_get_cmpfunc_, &(struct block_find_compare_data_){self, data});
    if (lp < 0) {
        return 0;
    }
    const struct bsearch_list_block* block = &self->block[lp];
    return _bsearch_get(block->arr, self->element_size, block->arr_count_, self->element_cmp, data);
}

static int bsearch_list_block_merge_(struct bsearch_list* self, int i, int p) {
    struct bsearch_list_block* last = &self->block[i];
    struct bsearch_list_block* block = &self->block[p];
    if (last && last->arr_count_ + block->arr_count_ < self->element_limit / 2) {
        memcpy(last->arr + (last->arr_count_ * self->element_size), block->arr, block->arr_count_ * self->element_size);
        last->arr_count_ += block->arr_count_;
        free(block->arr);
        _bsearch_del_pos(self->block, sizeof(struct bsearch_list_block), &self->block_count_, p);
    }
}

int bsearch_list_del(struct bsearch_list* self, const void* data) {
    int pos = 0;
    int lp = _binary_search(self->block, self->block_count_, sizeof(struct bsearch_list_block), block_get_cmpfunc_, &(struct block_find_compare_data_){self, data});
    if (lp < 0) {
        return -1;
    }
    struct bsearch_list_block* block = &self->block[lp];
    int ret = _bsearch_del(block->arr, self->element_size, &block->arr_count_, self->element_cmp, data);
    self->element_count -= 1;

    if (block->arr_count_ == 0) {
        _bsearch_del_pos(self->block, sizeof(struct bsearch_list_block), &self->block_count_, lp);
        return ret;
    }

    if (lp - 1 >= 0) {    // 合并
        const struct bsearch_list_block* last = &self->block[lp - 1];
        if (bsearch_list_block_merge_(self, lp - 1, lp)) {
            return ret;
        }
    }
    if (lp + 1 < self->block_count_) {    // 合并
        if (bsearch_list_block_merge_(self, lp, lp + 1)) {
            return ret;
        }
    }
    return ret;
}

void bsearch_list_free(struct bsearch_list* self) {
    for (int i = 0; i < self->block_count_; i++) {
        free(self->block[i].arr);
    }
    free(self->block);
    free(self);
}

int bsearch_list_count(struct bsearch_list* self) {
    return self->element_count;
}