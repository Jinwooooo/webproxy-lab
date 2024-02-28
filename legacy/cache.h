#ifndef __CACHE_H__
#define __CACHE_H__

#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct cache_node {
	char *index;
	char *content;
	struct cache_node *prev;
	struct cache_node *next;
	unsigned int length;
} cache_node;

typedef struct cache_list {
	cache_node *head;
	cache_node *tail;
	pthread_rwlock_t *lock;
	unsigned int bytes_left;
} cache_list;

cache_list *init_cache_list();
void init_node(cache_node *node);

void set_node(cache_node *node, char *index, unsigned int len);
void delete_node(cache_node *node);
void delete_list(cache_list *list);

cache_node *search_node(cache_list *list, char *index);
void add_node(cache_node *node, cache_list *list);
cache_node *remove_node(char *index, cache_list *list);
cache_node *evict_list(cache_list *list);

int read_node_content(cache_list *list, char *index, char *content, unsigned int *len);
int insert_content_node(cache_list *list, char *index, char *content, unsigned int len);


#endif