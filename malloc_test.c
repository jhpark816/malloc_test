#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#ifdef USE_TCMALLOC
#include <google/tcmalloc.h>
#include <google/malloc_extension_c.h>
#else
#include <malloc.h>
#endif

#define ITEM_MAX_SIZE (1024*1024) 

#ifdef USE_TCMALLOC
#define malloc(size) tc_malloc(size)
#define free(ptr) tc_free(ptr)
#endif

#ifdef USE_TCMALLOC
#define TCMALLOC_PROPERTY_COUNT 7
char *prop_name[TCMALLOC_PROPERTY_COUNT] = { "generic.current_allocated_bytes",
                                             "generic.heap_size",
                                             "tcmalloc.pageheap_free_bytes",
                                             "tcmalloc.pageheap_unmapped_bytes",
                                             "tcmalloc.slack_bytes",
                                             "tcmalloc.max_total_thread_cache_bytes",
                                             "tcmalloc.current_total_thread_cache_bytes" };
size_t prop_data[TCMALLOC_PROPERTY_COUNT]; 
#endif

typedef struct _item {
    struct _item *prev;
    struct _item *next;
    int      nbytes;
    char     value[1]; 
} item_t;

item_t *item_head = NULL;
item_t *item_tail = NULL;
unsigned int item_count = 0;

unsigned int mem_limit = 0;
unsigned int mem_alloc = 0;

char sample_data[1024];

static void init_sample_data() {
    int i;
    for (i = 0; i < 1024; i++) {
        sample_data[i] = 'A' + (i % 26);
    }
}

static void init_item(item_t *item_ptr, int data_len) {
    int val_offset = 0;  
    int copy_len;

    item_ptr->nbytes = data_len;
    while (data_len > 0) {
        copy_len = (data_len > 1024 ? 1024 : data_len);
        memcpy(&item_ptr->value[val_offset], sample_data, copy_len);
        val_offset += copy_len;
        data_len   -= copy_len; 
    }
}

static void insert_item_into_head(item_t *item_ptr) {
    item_ptr->prev = NULL;
    item_ptr->next = item_head; 
    if (item_ptr->next != NULL)
        item_ptr->next->prev = item_ptr;
    item_head = item_ptr;
    if (item_tail == NULL) item_tail = item_ptr;
    item_count++; 
}

static item_t *remove_item_from_tail() {
    item_t *item_ptr = item_tail;
    if (item_ptr != NULL) {
        if (item_ptr->prev != NULL)
            item_ptr->prev->next = NULL;
        item_tail = item_ptr->prev;
        if (item_tail == NULL)
            item_head = NULL;
        item_ptr->prev = item_ptr->next = NULL; 
        item_count--;
    } else {
        assert(item_count == 0);
    } 
    return item_ptr;
}

#ifdef USE_TCMALLOC
static void dump_malloc_info()
{
    int i, blocks;
    int hist[64];
    size_t total;
    char buffer[200]; 

    /******
    for (i = 0; i < TCMALLOC_PROPERTY_COUNT; i++) {
        if (!MallocExtension_GetNumericProperty(prop_name[i], &prop_data[i])) {
            fprintf(stderr, "tcmalloc get propery error: name=%s\n", prop_name[i]);
            exit(1);
        }
        fprintf(stderr, "[tcmalloc] property: %s=%d\n", prop_name[i], prop_data[i]);
    } 
    *****/
    MallocExtension_GetStats(buffer, sizeof(buffer));
    fprintf(stderr, "%s\n", buffer);
    /*****
    fprintf(stderr, "[tcmalloc] get stats: buffer=%s\n", buffer);
    MallocExtension_MallocMemoryStats(&blocks, &total, hist);
    fprintf(stderr, "[tcmalloc] memory stats: blocks=%d total=%d hist=[\n", blocks, total);
    for (i = 0; i < 64; i++) {
        fprintf(stderr, "%d, ", hist[i]);
    }
    fprintf(stderr, "]\n");
    *****/
}
#else
static void dump_malloc_info()
{
    unsigned int total, used, free;
    char buffer[200];
    struct mallinfo info = mallinfo();
    used = (unsigned int) info.uordblks;
    free = (unsigned int) info.fordblks;
    total = used + free;
    sprintf(buffer, "-----------------------------------\n"
                    "MALLOC: %12u ( %6.1f MB) Heap size\n"
                    "MALLOC: %12u ( %6.1f MB) Bytes Used\n"
                    "MALLOC: %12d ( %6.1f MB) Bytes Free\n",
                    total, (float)total/(1024*1024),
                    used , (float)used/(1024*1024),
                    free,  (float)free/(1024*1024));
    fprintf(stderr, "%s", buffer);
    /*****
    fprintf(stderr, "arena=%d ordblks=%d hblks=%d hblkhd=%d uordblks=%d fordblks=%d keepcost=%d\n",
            info.arena, info.ordblks, info.hblks, info.hblkhd, info.uordblks, info.fordblks, info.keepcost);
    *****/ 
}
#endif

main(int argc, void **argv)
{
    unsigned int mlimit_MB;
    unsigned int max_count;
    unsigned int try_count;
    int data_len; 
    int item_len; 
    int free_len;    
    item_t *item_ptr;
#ifdef USE_TCMALLOC
    size_t heap_size;
    char   *exe_name = "tcmalloc_test"; 
#else
    char   *exe_name = "ptmalloc_test"; 
#endif

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <mem_limit(unit: MB)> <malloc_count>\n", exe_name);
        exit(1);
    }
    mlimit_MB = atoi(argv[1]);
    mem_limit = mlimit_MB * 1024 * 1024;
    max_count = atoi(argv[2]);

    init_sample_data();

    fprintf(stderr, "[%s] mem_limit=%u(%u MB) malloc_count=%u\n",
                    exe_name, mem_limit, mlimit_MB, max_count); 

#ifdef USE_TCMALLOC
#if 0
    /* set heap_size : impossible */
    if (!MallocExtension_SetNumericProperty("generic.heap_size", mem_limit)) {
        fprintf(stderr, "tcmalloc set propery generic.heapsize error\n");
        exit(1);
    }
#endif
#ifdef LIMIT_HEAP_SIZE
    if (!MallocExtension_GetNumericProperty("generic.heap_size", &heap_size)) {
        fprintf(stderr, "tcmalloc get propery generic.heapsize error\n");
        exit(1);
    }
#endif
#endif

    try_count = 0;
    while (try_count < max_count)
    {
        if ((try_count % 100) < 90) {
            data_len = random() % (4096-sizeof(item_t));
        } else {
            data_len = 4096 + (random() % (ITEM_MAX_SIZE-4095-sizeof(item_t)));
        }
        item_len = sizeof(item_t) + data_len;

#if defined(USE_TCMALLOC) && defined(LIMIT_HEAP_SIZE)
        while ((item_len + heap_size) > mem_limit) {
            item_ptr = remove_item_from_tail(); 
            if (item_ptr == NULL) {
                fprintf(stderr, "empty list: mem_alloc=%u\n", mem_alloc);
                exit(1);
            } 
            free_len = (sizeof(item_t) + item_ptr->nbytes);
            mem_alloc -= free_len;
            free(item_ptr);
            MallocExtension_ReleaseToSystem(free_len);
            MallocExtension_ReleaseFreeMemory();
            if (!MallocExtension_GetNumericProperty("generic.heap_size", &heap_size)) {
                fprintf(stderr, "tcmalloc get propery generic.heapsize error\n");
                exit(1);
            }
        }
#else
        while ((item_len + mem_alloc) > mem_limit) {
            item_ptr = remove_item_from_tail(); 
            if (item_ptr == NULL) {
                fprintf(stderr, "empty list: mem_alloc=%u\n", mem_alloc);
                exit(1);
            } 
            free_len = (sizeof(item_t) + item_ptr->nbytes);
            mem_alloc -= free_len;
            free(item_ptr);
        }
#endif

        item_ptr = (item_t*)malloc(item_len);
        if (item_ptr == NULL) {
            fprintf(stderr, "memory allocation fail: item_len=%d\n", item_len);
            exit(1);
        }
        mem_alloc += item_len;

        init_item(item_ptr, data_len);
        insert_item_into_head(item_ptr);

        if ((++try_count % 100000) == 0) {
#if 0
            fprintf(stderr, "execution(%u) mem_alloc=%u item_count=%u\n",
                            try_count, mem_alloc, item_count);
            /* dump_malloc_info(); */
#endif
        }
    }

    dump_malloc_info();

    while ((item_ptr = remove_item_from_tail()) != NULL) {
        mem_alloc -= (sizeof(item_t) + item_ptr->nbytes);
        free(item_ptr);
    }  
    assert(item_head == NULL);
    assert(item_tail == NULL);
    assert(item_count == 0);
    assert(mem_alloc == 0);

    exit(0);
}


