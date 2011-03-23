#ifndef YT_UTIL_H_
#define YT_UTIL_H_

enum{
  OP_LOCAL_MAGAZINE_INFO=1, OP_LOCAL_MAGAZINE_REQUEST,
  OP_REMOTE_MAGAZINE_CONTENT, OP_REMOTE_MAGAZINE_INFO,
  OP_INTER_TRANSFER_REQUEST, OP_LOCAL_SYS_INFO
};

// pages info tag
struct mag_tag{
  int isbn, issue;
  bool operator ==(const mag_tag &t){
    return (t.isbn==isbn)&&(t.issue==issue);
  }
};

struct mag_set{
  int isbn, min_issue, max_issue;
};

enum {
  MAGAZINE_FREE, MAGAZINE_CHARGE
};

// book transfer sync
bool wait_for_book(int isbn, int issue);
void wake_for_book(int isbn, int issue);

#endif
