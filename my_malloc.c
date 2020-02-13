#include "my_malloc.h"
#define metadatasize 24

space* free_head=NULL;
space* free_tail=NULL;
__thread space * free_head_nolock = NULL;
__thread space * free_tail_nolock = NULL;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

space* find_best_space(size_t size){
  space* cur=free_head;
  space* miniblock=NULL;
  size_t minisize=INT_MAX;
  while(cur!=NULL){
    if(cur->size>=size){
      if(cur->size==size){
	miniblock=cur;
	break;
      }
      if(cur->size<minisize){
	miniblock=cur;
	minisize=cur->size;
      }
    }
    cur=cur->free_next;
  }
  return miniblock;
}

space* create_new_space(size_t size){
  space*cur;
  cur=sbrk(0);
  if(sbrk(metadatasize+size)==(void*)-1){
    return NULL;
  }
  else{
    cur->size=size;
    cur->free_prev=NULL;
    cur->free_next=NULL;
  }
  return cur;
}

void add_to_freelist(space* block){
  if(free_head==NULL){ //there is no free block
    free_head=block;
    free_tail=block;
    block->free_prev=NULL;
    block->free_next=NULL;
  }
  else if(block<free_head){
    block->free_next=free_head;
    block->free_prev=NULL;
    free_head->free_prev=block;
    free_head=block;
  }
  else if(block>free_tail){
    free_tail->free_next=block;
    block->free_prev=free_tail;
    block->free_next=NULL;
    free_tail=block;
  }
  else{
    space* cur=free_head;
    while(cur!=NULL){
      if(cur>block){
	break;
      }
      cur=cur->free_next;
    }
    block->free_next=cur;
    block->free_prev=cur->free_prev;
    cur->free_prev->free_next=block;
    cur->free_prev=block;
  }
}

void merge(space* block){
  space* left=block->free_prev;
  if(left!=NULL){
    if(((char*)left+metadatasize+left->size)==(char*)block){ //merge left can happen
      left->size+=metadatasize+block->size;
      left->free_next=block->free_next;
      if(left->free_next!=NULL){
	left->free_next->free_prev=left;
      }
      if(block==free_tail){
	free_tail=left;
      }
      block=left;
    }
  }
  space* right=block->free_next;
  if(right!=NULL){
    if(((char*)block+metadatasize+block->size)==(char*)right){ // merge right can happen
      block->size+=metadatasize+right->size;
      block->free_next=right->free_next;
      if(block->free_next!=NULL){
	block->free_next->free_prev=block;
      }
      if(free_tail==right){
	free_tail=block;
      }
    }
  }
}

space* split(space* region, size_t size){
  size_t move=(metadatasize+region->size)-(metadatasize+size);
  space* occupy = (space*)((char*)region+move);
  region->size=move-metadatasize;
  occupy->size=size;
  occupy->free_prev=NULL;
  occupy->free_next=NULL;
  return occupy;
}


void* bf_malloc(size_t size){
  space* region;
  if(free_head==NULL){  //there is no free block
    region=create_new_space(size);
    if(region==NULL){
      return NULL;
    }
  }
  else{
    region=find_best_space(size);
    if(region==NULL){
      region=create_new_space(size);
    }
    else{  
      if((region->size+metadatasize)>=2*(metadatasize+size)){
	region=split(region,size);
      }
      else{  //transfer one free block into not free block
        if(region->free_prev!=NULL){
	  region->free_prev->free_next=region->free_next;
	}
	if(region->free_next!=NULL){
	  region->free_next->free_prev=region->free_prev;
	}
	if(free_head==region){
	  free_head=region->free_next;
	}
	if(free_tail==region){
	  free_tail=region->free_prev;
	}
	region->free_next=NULL;
	region->free_prev=NULL;
      }
    }
  }
  char * res = &region->first_byte;
  return (void*)res;
}


void bf_free(void* ptr){
  space* cur;
  cur=(space*)((char*)ptr-metadatasize);
  add_to_freelist(cur);
  merge(cur);
  return;
}

void *ts_malloc_lock(size_t size){
    pthread_mutex_lock(&lock);
    space* ans=bf_malloc(size);
    pthread_mutex_unlock(&lock);
    return ans;
} 

// thread-safe free with lock
void ts_free_lock(void *ptr){
    pthread_mutex_lock(&lock);
    bf_free(ptr);
    pthread_mutex_unlock(&lock);
}


space* find_best_space_nolock(size_t size){
  space* cur=free_head_nolock;
  space* miniblock=NULL;
  size_t minisize=INT_MAX;
  while(cur!=NULL){
    if(cur->size>=size){
      if(cur->size==size){
	miniblock=cur;
	break;
      }
      if(cur->size<minisize){
	miniblock=cur;
	minisize=cur->size;
      }
    }
    cur=cur->free_next;
  }
  return miniblock;
}

space* create_new_space_nolock(size_t size){
  //space*cur;
  //cur=sbrk(0);
  pthread_mutex_lock(&lock);
  space* cur =sbrk(metadatasize+size);
  pthread_mutex_unlock(&lock);
  cur->size=size;
  cur->free_prev=NULL;
  cur->free_next=NULL;
  return cur;
}

void add_to_freelist_nolock(space* block){
  if(free_head_nolock==NULL){ //there is no free block
    free_head_nolock=block;
    free_tail_nolock=block;
    block->free_prev=NULL;
    block->free_next=NULL;
  }
  else if(block<free_head_nolock){
    block->free_next=free_head_nolock;
    block->free_prev=NULL;
    free_head_nolock->free_prev=block;
    free_head_nolock=block;
  }
  else if(block>free_tail_nolock){
    free_tail_nolock->free_next=block;
    block->free_prev=free_tail_nolock;
    block->free_next=NULL;
    free_tail_nolock=block;
  }
  else{
    space* cur=free_head_nolock;
    while(cur!=NULL){
      if(cur>block){
	break;
      }
      cur=cur->free_next;
    }
    block->free_next=cur;
    block->free_prev=cur->free_prev;
    cur->free_prev->free_next=block;
    cur->free_prev=block;
  }
}

void merge_nolock(space* block){
  space* left=block->free_prev;
  if(left!=NULL){
    if(((char*)left+metadatasize+left->size)==(char*)block){ //merge left can happen
      left->size+=metadatasize+block->size;
      left->free_next=block->free_next;
      if(left->free_next!=NULL){
	left->free_next->free_prev=left;
      }
      if(block==free_tail_nolock){
	free_tail_nolock=left;
      }
      block=left;
    }
  }
  space* right=block->free_next;
  if(right!=NULL){
    if(((char*)block+metadatasize+block->size)==(char*)right){ // merge right can happen
      block->size+=metadatasize+right->size;
      block->free_next=right->free_next;
      if(block->free_next!=NULL){
	block->free_next->free_prev=block;
      }
      if(free_tail_nolock==right){
	free_tail_nolock=block;
      }
    }
  }
}

space* split_nolock(space* region, size_t size){
  size_t move=(metadatasize+region->size)-(metadatasize+size);
  space* occupy = (space*)((char*)region+move);
  region->size=move-metadatasize;
  occupy->size=size;
  occupy->free_prev=NULL;
  occupy->free_next=NULL;
  return occupy;
}


void* ts_malloc_nolock(size_t size){
  space* region;
  if(free_head_nolock==NULL){  //there is no free block
    region=create_new_space_nolock(size);
    if(region==NULL){
      return NULL;
    }
  }
  else{
    region=find_best_space_nolock(size);
    if(region==NULL){
      region=create_new_space_nolock(size);
    }
    else{  
      if((region->size+metadatasize)>=2*(metadatasize+size)){
	region=split_nolock(region,size);
      }
      else{  //transfer one free block into not free block
        if(region->free_prev!=NULL){
	  region->free_prev->free_next=region->free_next;
	}
	if(region->free_next!=NULL){
	  region->free_next->free_prev=region->free_prev;
	}
	if(free_head_nolock==region){
	  free_head_nolock=region->free_next;
	}
	if(free_tail_nolock==region){
	  free_tail_nolock=region->free_prev;
	}
	region->free_next=NULL;
	region->free_prev=NULL;
      }
    }
  }
  char * res = &region->first_byte;
  return (void*)res;
}


void ts_free_nolock(void* ptr){
  space* cur;
  cur=(space*)((char*)ptr-metadatasize);
  add_to_freelist_nolock(cur);
  merge_nolock(cur);
  return;
}
