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

void dfree(data *d) { free(d->v); d->v= NULL; }

// own: free k.d, v.d later
void kv_put(data k, data v, int own) {
  assert(kvn<=KVSIZE);
  kvstore[kvn].k= own?k:ddup(k);
  kvstore[kvn].v= own?v:ddup(v);
  kvn++;
}

void kv_puts(char* k, char* v, int own) {
  data kd= {0, k}, vd= {0, v};
  kv_put(kd, vd, own);
}

data kv_get(data k) {
  for(int i=0; i<kvn; i++)
    // TODO: len
    if (0==strcmp(kvstore[i].k.v,k.v)) return kvstore[i].v;
  return (data){0, NULL};
}

char* kv_gets(char* k) {
  data kd= {0, k};
  data vd= kv_get(kd);
  return vd.v;
}

// tests
int main(int argc, char** argv) {
  data k={0},v={0};
  k.v= "0000001st";
  v.v= "first";
  kv_put(k, v, 0);
  kv_puts("0000002nd", "second", 0);

  data r= kv_get(k);
  data kk= {0, "0000002nd"};
  data rr= kv_get(k);

  // value get
  printf("%s => %s\n", k.v, r.v);
  printf("%s => %s\n", kk.v, rr.v);
  // string get
  printf("%s => %s\n", "0000001st", kv_gets("0000001st"));
  printf("%s => %s\n", "0000002nd", kv_gets("0000002nd"));
  // get non-exist
  printf("%s => %s\n", "0000003nd", kv_gets("0000003nd"));

  // short key
  data ks= {3, "0000002nd"};
  data vs= {0, "nullth"};
  kv_put(ks, vs, 0);

  printf("%s => %s\n", "000", kv_gets("000"));

  return 9;
}
