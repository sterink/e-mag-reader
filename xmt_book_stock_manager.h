#ifndef BOOK_STOCK_H_
#define BOOK_STOCK_H_
#include <queue>
#include "ebp_server.h"

class book_stock_manager{
  int channel;

  public:
    book_stock_manager();

  public:
    void handle_request(local_sys_info *);
    void handle_request(local_magazine_info *);
    void handle_request(local_magazine_request *);
  public:
    void set_channel(int f);
    void try_push_content(int);
};
#endif
