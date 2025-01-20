/* 
 * cachelab.h - Prototypes for Cache Lab helper functions
 */

#ifndef CACHELAB_TOOLS_H
#define CACHELAB_TOOLS_H

#define MAX_TRANS_FUNCS 100

typedef struct trans_func{
  void (*func_ptr)(int M,int N,int[N][M],int[M][N]);
  char* description;
  char correct;
  unsigned int num_hits;
  unsigned int num_misses;
  unsigned int num_evictions;
} trans_func_t;

/* 
 * printSummary - This function provides a standard way for your cache
 * simulator * to display its final hit and miss statistics
 */ 
void printSummary(int hits,  /* number of  hits */
				  int misses, /* number of misses */
				  int evictions); /* number of evictions */

/* Fill the matrix with data */
void initMatrix(int M, int N, int A[N][M], int B[M][N]);

/* The baseline trans function that produces correct results. */
void correctTrans(int M, int N, int A[N][M], int B[M][N]);

/* Add the given function to the function list */
void registerTransFunction(
    void (*trans)(int M,int N,int[N][M],int[M][N]), char* desc);

//////////////// student function ////////////////
typedef struct _node{
	unsigned tag; // 嘿哥们，这个是关键啊
	struct _node* prev;
	struct _node* next;
}node;
typedef struct _LRU_line{
	node* head; // 最近被使用
	node* tail; // 最久未被使用
	int* size;
}LRU_line;

int GoThroughSet(LRU_line* target_set, unsigned target_tag);
void find_and_delete(LRU_line* target_set, unsigned target_tag);
void option_analyze(int argc, char** agrv, char** filename);
void initializeCache(LRU_line* set);
void whole_simulation(char* filename);
void head_insert(LRU_line* target_set, unsigned target_tag);
void tail_delete(LRU_line* target_set);
void eviction(LRU_line* target_set);
void update_cache(LRU_line* set, char opration, unsigned address, int opration_size);
#endif /* CACHELAB_TOOLS_H */
