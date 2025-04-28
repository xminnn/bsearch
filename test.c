#include <errno.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "bsearch.h"
#include "bsearch_list.h"

long long time_curruent_us() {
    long long now;
    struct timeval tv;
    gettimeofday(&tv, 0);
    now = tv.tv_sec;
    now = now * 1000000;
    now += tv.tv_usec;
    return now;
}

int util_rand(int min, int max) {
    static unsigned long long seed;
    static char seed_init = 0;
    static const unsigned long long a = 1703515245;
    static const unsigned long long c = 563871;
    static const unsigned long long m = 1 << 31;    // 2^31
    if (seed_init == 0) {
        seed_init = 1;
        seed = time_curruent_us();
    }
    seed = (a * seed + c) % m;    // 线性同余生成器
    if (max <= min) {
        return min;
    }
    return seed % (max - min + 1) + min;
}

struct element {
    long long uid;
    long long source;
};

int main(int argc, char const* argv[]) {
    int count = argc > 1 ? atoi(argv[1]) : 100000;
    int init_cap = argc > 2 ? atoi(argv[2]) : 6;
    int not_list_test = argc > 3 ? atoi(argv[3]) : 1;

    static struct element arr[10010000];
    for (long long i = 0; i < 10010000; i++) {
        arr[i].source = util_rand(100, 1000000000);
        arr[i].uid = util_rand(100, 1000000000);
    }
    int test_get[10];
    for (int i = 0; i < 10; i++) {
        test_get[i] = util_rand(100, count - 1);
    }
    struct element rand_put;
    rand_put.source = util_rand(100, 1000000000);
    rand_put.uid = util_rand(100, 1000000000);

    printf("start\n");

    if (not_list_test == 1) {
        static struct {
            struct element logined_users[5002400];    // 所有的活跃用户
            int logined_users_count_;
        } data = {0};
        long long start = time_curruent_us();
        for (int i = 0; i < count; i++) {
            bsearch_put(data.logined_users, cmp_2int64, &arr[i]);
        }
        long long end = time_curruent_us();
        printf("put time cost: %lldms, count=%lld. %gus/op\n", (end - start) / 1000, data.logined_users_count_, 1.0 * (end - start) / data.logined_users_count_);

        start = time_curruent_us();
        bsearch_put(data.logined_users, cmp_2int64, &rand_put);
        end = time_curruent_us();
        printf("put one time cost: %lldus, count=%d\n", end - start, data.logined_users_count_);

        start = time_curruent_us();
        bsearch_get(data.logined_users, cmp_2int64, &rand_put);
        end = time_curruent_us();
        printf("get one time cost: %lldus, count=%d\n", end - start, data.logined_users_count_);

        start = time_curruent_us();
        int error = 0;
        for (long long i = 0; i < count; i++) {
            int p = bsearch_get_pos(data.logined_users, cmp_2int64, &arr[i]);
            if (p < 0) {
                error += 1;
            }
        }
        end = time_curruent_us();
        printf("get time cost: %lldms, %gus/op, error=%d\n", (end - start) / 1000, 1.0 * (end - start) / data.logined_users_count_, error);

        printf("get test: ");
        for (int i = 0; i < 10; i++) {
            struct element* one = &arr[test_get[i]];
            int p = bsearch_get_pos(data.logined_users, cmp_2int64, one);
            printf("(%lld)%d ", one->uid, p);
        }
        printf("\n");
    }

    {
        struct bsearch_list* blist = bsearch_list_create(1000, 8000, sizeof(struct element), cmp_2int64);

        long long start = time_curruent_us();
        for (int i = 0; i < count; i++) {
            bsearch_list_put(blist, &arr[i]);
        }
        long long end = time_curruent_us();
        printf("put time cost: %lldms, count=%d. %gus/op\n", (end - start) / 1000, bsearch_list_count(blist), 1.0 * (end - start) / bsearch_list_count(blist));

        start = time_curruent_us();
        bsearch_list_put(blist, &rand_put);
        end = time_curruent_us();
        printf("put one time cost: %lldus, count=%d\n", end - start, bsearch_list_count(blist));

        start = time_curruent_us();
        bsearch_list_get_pos(blist, &rand_put);
        end = time_curruent_us();
        printf("get one time cost: %lldus, count=%d\n", end - start, bsearch_list_count(blist));

        start = time_curruent_us();
        int error = 0;
        for (long long i = 0; i < count; i++) {
            int p = bsearch_list_get_pos(blist, &arr[i]);
            if (p < 0) {
                error += 1;
            }
        }
        end = time_curruent_us();
        printf("get time cost: %lldms, %gus/op. error=%d\n", (end - start) / 1000, 1.0 * (end - start) / bsearch_list_count(blist), error);

        printf("get test: ");
        for (long long i = 0; i < 10; i++) {
            struct element* one = &arr[test_get[i]];
            int p = bsearch_list_get_pos(blist, one);
            printf("(%lld)%d ", one->uid, p);
        }
        printf("\n");

        start = time_curruent_us();
        error = 0;
        for (int i = 0; i < count; i++) {
            int p = bsearch_list_del(blist, &arr[i]);
            if (p < 0) {
                error += 1;
            }
        }
        end = time_curruent_us();
        printf("del time cost: %lldms, count=%d. %gus/op, error=%d\n", (end - start) / 1000, bsearch_list_count(blist), 1.0 * (end - start) / count, error);
        bsearch_list_free(blist);
    }
    return 0;
}

/**
gcc -O3 *.c -o app
 */