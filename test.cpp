#include <errno.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <map>

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
    static const unsigned long long a = 1103515245;
    static const unsigned long long c = 12345;
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

struct CustomCompare {
    bool operator()(const struct element& a, const struct element& b) const {
        if (a.source != b.source) return a.source < b.source;
        return a.uid < b.uid; 
    }
};

std::map<struct element, int, CustomCompare> data;

int main(int argc, char const *argv[]) {
    int count = argc > 1 ? atoi(argv[1]) : 1000000;

    static struct element arr[10010001];
    for (long long i = 0; i < 10010001; i++) {
        arr[i].source = util_rand(100, 1000000000);
        arr[i].uid = util_rand(100, 1000000000);
    }
    struct element rand_put;
    rand_put.source = util_rand(100, 1000000000);
    rand_put.uid = util_rand(100, 1000000000);

    printf("start\n");
    long long start = time_curruent_us();
    for (int i = 0; i < count; i++) {
        data[arr[i]] = i;
    }
    long long end = time_curruent_us();
    printf("put time cost: %lldms, count=%d. %gus/op\n", (end - start) / 1000, data.size(), 1.0 * (end - start) / data.size());

    start = time_curruent_us();
    data[rand_put] = 1;
    end = time_curruent_us();
    printf("put one cost: %lldus\n", (end - start));

    start = time_curruent_us();
    auto it = data.find(rand_put);
    int pos = std::distance(data.begin(), it);
    end = time_curruent_us();
    printf("get pos cost: %lldus, pos=%d\n", (end - start), pos);
    return 0;
}

/**
g++ -O3 test.cpp -o app
$ ./app 10000000
start
put time cost: 12645ms, count=10000000. 0.00126456ms/op
put one cost: 4us
get pos cost: 781887us, pos=6708311
 */