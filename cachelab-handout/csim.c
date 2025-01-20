#include "cachelab.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
static int totalSet; // Set的总数量
static int s;
static int E;
static int b;
static int hit = 0;
static int miss = 0;
static int evictions = 0;
static int SetSize;
// 我打算用链表做，因为C语言没有标准库可以直接使用queue

void option_analyze(int argc, char** agrv, char** filename){
	// linux> ./csim-ref -v -s 4 -E 1 -b 4 -t traces/yi.trace
    // 想办法获取 s=4; E=1; b=4; filename = traces/yi.trace;
    int option;
    while ( (option = getopt(argc, agrv, "s:E:b:t:")) != -1)
    {
        switch (option)
        {
        case 's':
            s = atoi(optarg);
						break;
        case 'E':
            E = atoi(optarg);
						break;
        case 'b':
            b = atoi(optarg);
						break;
        case 't':
            *filename = optarg;
						break;
        }
    }
   SetSize = 1 << s;
}

void initializeCache(LRU_line* set){
	for(int i = 0; i < SetSize; ++i)
	{
		set[i].head = malloc(sizeof(node));
		set[i].tail = malloc(sizeof(node));
		(set[i].size) = (int* )malloc(sizeof(int));

		set[i].head->prev = NULL; 
		set[i].head->next = set[i].tail; 
		set[i].tail->prev = set[i].head;
		set[i].tail->next = NULL;
		
		*(set[i].size) 		= 0;
	}
}


void whole_simulation(char* filename)
{
	// 初始化缓存
	LRU_line* set = malloc(SetSize * sizeof(LRU_line));
	initializeCache(set);
	// 打开文件并读入一行
	printf("Opening file: %s\n", filename);
	FILE* file = fopen(filename, "r");
	if (file == NULL) {
    perror("Error opening file");
    exit(EXIT_FAILURE);
	}
	char opration;
  unsigned address;
  int opration_size;

	// 解析trace文件中的每一行
	while(fscanf(file, " %c %x,%d", &opration, &address, &opration_size) > 0)
	{
		switch (opration)
		{
			case 'L':
				update_cache(set, opration, address, opration_size);
				break;
			case 'M': // 当我们执行M的时候，实际上我们执行的是L+S，这里执行M就相当于执行了L，所以接下来只需要执行S，因此case'M'没有break
				update_cache(set, opration, address, opration_size); // L
			case 'S':
				update_cache(set, opration, address, opration_size);
				break;
		}
	}

	fclose(file);
}

void update_cache(LRU_line* set, char opration, unsigned address, int opration_size)
{
		// 解析address
		unsigned target_tag_index = address >> (b + s);
		unsigned target_set_index = (address >> b) & ((1 << s) - 1);
		// 遍历set[target_set]，查询缓存中是否存在CPU请求的数据
		int found = 0;
		LRU_line* target_set = &set[target_set_index];
		found = GoThroughSet(target_set, target_tag_index);
		if(found){ // 如果找到了那自然是皆大欢喜了
			++hit;
			printf("hit in set[%u] \n",target_set_index);
			printf("%c %x,%d hit \n\n",opration, address, opration_size);
			// 将命中的节点放到链表头部（最高优先级）
			find_and_delete(target_set, target_tag_index);
			head_insert(target_set, target_tag_index);
		}else{ // 当所要寻找的数据不在缓存中的情况
			++miss;
			if(*(target_set->size) != E){ //缓存未满
				printf("miss in set[%u] \n",target_set_index);
				printf("%c %x,%d miss \n\n",opration, address, opration_size);
				head_insert(target_set, target_tag_index);
				++*(target_set->size);
			}else{ 										 //缓存已满
				++evictions;
				printf("miss and eviction in set[%u]\n",target_set_index);
				printf("%c %x,%d miss eviction \n\n",opration, address, opration_size);
				eviction(target_set);
				// --*(target_set->size);
				head_insert(target_set, target_tag_index);
			}
		}
}


int GoThroughSet(LRU_line* target_set, unsigned target_tag){
	if(target_set->head->next == target_set->tail)
	return 0;
	// 遍历该set中的所有line(也就是遍历双向链表)
	node* start = target_set->head->next;
	while(start != target_set->tail){
		if(start->tag == target_tag){
			return 1;
		}
		start = start->next;
	}
	return 0;
}

void find_and_delete(LRU_line* target_set, unsigned target_tag){
	// 遍历该set中的所有line(也就是遍历双向链表)
	node* start = target_set->head->next;
	while(start != target_set->tail){
		if(start->tag == target_tag){
			node* junk = start;
			junk->prev->next = junk->next;
			junk->next->prev = junk->prev;
			free(junk);
			return;
		}
		start = start->next;
	}
}


// 显然，我们还需要实现head_insert()和tail_delete()
void head_insert(LRU_line* target_set, unsigned target_tag){
	// 双链表头插入
	node* newnode = malloc(sizeof(node));
	if (!newnode) {
		perror("Memory allocation failed");
		exit(EXIT_FAILURE);
	}
	newnode->tag = target_tag;

	newnode->next = target_set->head->next;
	target_set->head->next->prev = newnode;
	target_set->head->next = newnode;
	newnode->prev = target_set->head;
}

void tail_delete(LRU_line* target_set){
	if (target_set->head->next == target_set->tail) {
		// 链表为空，不需要删除
		return;
	}
	// 双链表尾删除
	node* junk = target_set->tail->prev;
	junk->prev->next = target_set->tail;
	target_set->tail->prev = junk->prev;
	free(junk);
}
void eviction(LRU_line* target_set){
	// 双链表尾驱逐
	
	tail_delete(target_set);
}
int main(int argc, char** argv){
	char* fileName = malloc(100 * sizeof(char)); // 文件不可能比100个字符还长吧
	if (!fileName) {
			perror("Memory allocation failed");
			exit(EXIT_FAILURE);
	}	
	option_analyze(argc, argv, &fileName);
	// 查询缓存中是否存在CPU请求的数据
	whole_simulation(fileName);
	printSummary(hit, miss, evictions);
	return 0;
}