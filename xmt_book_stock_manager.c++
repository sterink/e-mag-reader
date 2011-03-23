#include "xmt_book_stock_manager.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

struct issue_table{
  struct issue_table *up, *down;
  int issue;
  bit_bool_set total_set;
};

struct isbn_table{
  struct isbn_table *next, *prev;
  int isbn;
  issue_table *issue_set;
};

struct book_store{
  isbn_table *bk_store;

  book_store(){ bk_store = NULL; }

  //init(); // initialize from database

  void get_available_task();

  void deselect_isbn_info(int i, int s, int p){
    isbn_table *store = NULL;
    for(store=bk_store;store;store=store->next){
      if(store->isbn == i) break;
    }
    if(store && store->isbn == i){ // found, then rule out the page num
      issue_table *i_t = NULL;
      for(i_t=store->issue_set;i_t;i_t=i_t->down){
        if(i_t->issue==s) break;
      }
      if(i_t&&i_t->issue==s) // found 
        i_t->total_set.reset_bit(p);
    }
  }
};

book_stock_manager::book_stock_manager(){}

void book_stock_manager::set_channel(int f){
  channel = f;
}

void book_stock_manager::handle_request(local_sys_info *pkg){
  assert(pkg);

  int ii[] = {1234, 1345, 1432, 1721};
  const char *nn[] = {"看天下", "锋绘", "周刊", "时尚"};
  remote_magazine_info info;
  for(int i=0;i<sizeof(ii)/sizeof(int);i++){
    info.isbn = ii[i];
    strcpy(info.title, nn[i]);
    info.sendit(channel);
  }
}

void book_stock_manager::handle_request(local_magazine_info *pkg){
  assert(pkg);

  int ii[] = {100, 110};
  mag_tag pp[] = {{1234, 163}, {1234, 164}};
  remote_magazine_content content;
  content.set_type(MAGAZINE_FREE);
  for(int i=0;i<sizeof(ii)/sizeof(int);i++){
    content.set_tag(pp[i]);
    content.set_size(ii[i]);
    content.sendit(channel);
  }
}

void book_stock_manager::handle_request(local_magazine_request *pkg){
  assert(pkg);
  remote_magazine_content content;
  content.set_tag(pkg->tag);
  content.set_type(MAGAZINE_CHARGE);
  content.sendit(channel);
  //int isbn = pkg->isbn;
  //int issue = pkg->issue;
  //int page = pkg->page;
}

void book_stock_manager::try_push_content(int fd){

}
