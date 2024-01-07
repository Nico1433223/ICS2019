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
    WP* p=head;
    while(p->next!=wp){
      p=p->next;
    }
    p->next=wp->next;
  }
  wp->next=free_;
  free_=wp;
}

void print_wp(){
  if(head==NULL){
    printf("No watchpoints!\n");
    return;
  }
  WP* p=head;
  printf("Num\tValue\tWhat\n");
  while(p!=NULL){
    printf("%d\t%d\t%s\n",p->NO,p->value,p->expr);
    p=p->next;
  }
}

bool check_wp(){
  if(head==NULL){
    return true;
  }
  bool success=true;
  WP* p=head;
  while(p!=NULL){
    bool success_=true;
    uint32_t new_value=expr(p->expr,&success_);
    if(!success_){
      printf("Invalid expression!\n");
      assert(0);
    }
    if(new_value!=p->value){
      printf("Watchpoint %d: %s\n",p->NO,p->expr);
      printf("Old value = %d\n",p->value);
      printf("New value = %d\n",new_value);
      p->value=new_value;
      success=false;
    }
    p=p->next;
  }
  return success;
}

WP* find_wp(int NO){
  WP* p=head;
  while(p!=NULL){
    if(p->NO==NO){
      return p;
    }
    p=p->next;
  }
  return NULL;
}

void delete_wp(int NO){
  WP* p=find_wp(NO);
  if(p==NULL){
    printf("No such watchpoint!\n");
    assert(0);
  }
  free_wp(p);
}

void delete_all_wp(){
  while(head!=NULL){
    free_wp(head);
  }
}


