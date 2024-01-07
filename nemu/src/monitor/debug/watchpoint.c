#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
WP* new_wp(){
  if(free_==NULL){
    printf("No more watchpoints!\n");
    assert(0);
  }
  WP* new=free_;
  free_=free_->next;
  new->next=head;
  head=new;
  return new;
}

void free_wp(WP* wp){
  if(wp==NULL){
    printf("No such watchpoint!\n");
    assert(0);
  }
  if(wp==head){
    head=head->next;
  }
  else{
    WP* tmp=head;
    while(tmp->next!=wp){
      tmp=tmp->next;
    }
    tmp->next=tmp->next->next;
  }
  wp->next=free_;
  free_=wp;
}


