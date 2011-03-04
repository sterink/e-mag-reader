#ifndef YT_UTIL_H_
#define YT_UTIL_H_

enum{
  OP_LOCAL_MAGAZINE_INFO=1, OP_LOCAL_MAGAZINE_REQUEST,
  OP_REMOTE_MAGAZINE_CONTENT, OP_REMOTE_MAGAZINE_INFO,
  OP_INTER_TRANSFER_REQUEST, OP_LOCAL_SYS_INFO
};

// bit vector used 
struct bit_bool_set{
  int data[8];
  bit_bool_set(){
    for(int i=0;i<8;i++) data[i]=0;
  }

  void set_bit(int pos){
    int i=pos%32, j=pos/32;
    int &m=data[j];
    int mask = (1<<i);
    m |= mask;
  }
  void reset_bit(int pos){
    int i=pos%32, j=pos/32;
    int &m=data[j];
    int mask = (1<<i);
    m &= ~mask;
  }
  void flip_bit(int pos);
  bool operator[] (int pos ) const{
    int i=pos%32, j=pos/32;
    int m=data[j];
    int mask = (1<<i);
    m &= mask;
    return(m!=0);
  }
  void set_data(const void *dd){
    if(!dd) return;
    int *p = (int *)dd;
    for(int i=0;i<8;i++) data[i] = p[i];
  }
};

// pages info tag
struct pages_set_tag{
  int isbn, issue;
  bit_bool_set total_set;
};

struct page_info_tag{
  int isbn, issue, num;
};

enum {
  MAGAZINE_MAIN_CONTENT, MAGAZINE_DETAIL
};
#endif
