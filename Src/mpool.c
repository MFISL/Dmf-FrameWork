/* 
    *  Copyright 2023 Ajax
    *
    *  Licensed under the Apache License, Version 2.0 (the "License");
    *  you may not use this file except in compliance with the License.
    *
    *  You may obtain a copy of the License at
    *
    *    http://www.apache.org/licenses/LICENSE-2.0
    *    
    *  Unless required by applicable law or agreed to in writing, software
    *  distributed under the License is distributed on an "AS IS" BASIS,
    *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    *  See the License for the specific language governing permissions and
    *  limitations under the License. 
    *
    */

/*  
    *                       MPOOL FOR SERVER    
    *
    *   This model is for server memory allocate.
    *   You should call pool_init first.
    *   
    *   If it show message "no free memory left"
    *   you should consider total_size when you init the model.
    */

#include <dmfserver/mpool.h>

void pool_init(pool_t * pool, int block_size, int total_size) 
{
    pool->block_size = block_size;
    pool->total_size = total_size;
    pool->mem_pool = malloc(total_size);
    
    pool->head = NULL;
    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->cond, NULL);
    pool->count = 0;

    char *ptr = pool->mem_pool;
    int num_blocks = total_size / block_size;
    for (int i = 0; i < num_blocks; i++) {
        node_t *node = malloc(sizeof(node_t));
        node->data = ptr;
        node->used = 0; // Set the used flag to 0
        node->next = pool->head;
        pool->head = node;
        ptr += block_size;
        pool->count++;
    }
}


void pool_destroy(pool_t * pool)
{
    node_t *current = pool->head;
    while (current != NULL) {
        node_t *temp = current;
        current = current->next;
        free(temp);
    }
    free(pool->mem_pool);
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->cond);
}


void * pool_alloc(pool_t * pool) 
{
    pthread_mutex_lock(&pool->lock);

    if ( pool->count == 0 ) {
        printf("[Mpool: Warn] no more memory to allocate \n");
        pthread_cond_wait(&pool->cond, &pool->lock);
    }
    // printf("%d\n", pool->count);

    node_t *current = pool->head;
    
    while (current != NULL) {
        if(current->used == 0)
        break;
        current = current->next;
    }

    if (current == NULL) {
        printf(" No free block found (should not happen) \n");
        pthread_mutex_unlock(&pool->lock);
        return NULL;
    }

    current->used = 1;
    pool->count--;
    pthread_mutex_unlock(&pool->lock);
    return current->data;
}


void pool_free(pool_t * pool, void * data) 
{
    pthread_mutex_lock(&pool->lock);

    node_t *current = pool->head;

    while (current != NULL) {
        if(current->data == data)
        break;
        current = current->next;
    }

    if (current == NULL) {
        printf("Invalid data pointer\n");
        pthread_mutex_unlock(&pool->lock);
        return;
    }

    current->used = 0;
    pool->count++;

    pthread_cond_signal(&pool->cond);
    pthread_mutex_unlock(&pool->lock);
}

