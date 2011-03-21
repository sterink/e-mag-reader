#ifndef E_BOOK_H_
#define E_BOOK_H_
#include <unistd.h>

class e_book{
  int isbn;
  int issue;
  int page_numb; // positive is official version, negative is preedition
  //char save_path[256];

  int pos; // current page

  public:
  e_book(int, int, int);
  public:
  int go_absolute_page(int, char name[]);
  int go_relative_page(int, char name[]);

  public:
  int get_history_page(char name[]);
  int get_main_cover(char name[]);
};
#endif // 
