// A simple KeyValue DataBase
//
// Goals:
// - simple small C code
// - filesystem based
// - many readers same machine
// - distributable ("tail -f")
// - single writer
// - multi reader concurrent
// - transactions - ACID
// - crash safe/recoverable
//
// Non-Goals:
// - thread safe
// - memory sharing
// - multiple writers
// - concurrent writers
// - super efficiency

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

typedef struct data {
  // if len=0, val is zstring
  size_t len;
  char* v;
} data;

typedef struct kv {
  data k, v;
} kv;

#define KVSIZE 1024
kv kvstore[KVSIZE]= {0};
int kvn= 0;

data ddup(data d) {
  if (d.len)
    d.v= memcpy(malloc(d.len), d.v, d.len);
  else
    d.v= strdup(d.v);
  return d;
}

// own: free k.d, v.d later
void kv_add(data k, data v, int own) {
  assert(kvn<=KVSIZE);
  kvstore[kvn].k= own?k:ddup(k);
  kvstore[kvn].v= own?k:ddup(v);
}

// tests
int main(int argc, char** argv) {
  data k={0},v={0};
  k.v= "0000001st";
  v.v= "first";
  kv_add(k, v, 0);

  return 9;
}

