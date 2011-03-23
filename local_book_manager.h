#ifndef LOCAL_BOOK_MANAGER_H_
#define LOCAL_BOOK_MANAGER_H_

#include "xmt_book.h"

class local_book_manager{
  private:
    local_book_manager();

  public:
    static local_book_manager *instance(){
      static local_book_manager _instance;
      return &_instance;
    }
public:
    e_book *get_book(int, int, int);
    // return value
    // -1 mean no more books
    int read_book_shelf(char name[], int &i, int &s, int &num);
    int read_book_catalogue(char name[], int &i);
};
#endif
