// A simple KeyValue DataBase
//
// Goals:
// - simple small C code
// - file format: key va lue\n
// - key lookup
// - key prefix (un-ordered)
// - range search (un-ordered)
// - append only filesystem based
// - many readers same machine
// - distributable ("tail -f")
// - single writer
// - multi reader concurrent
// - append transaction - A(CI)D
// - crash safe/recoverable
// - timestamped
//
// Non-Goals:
// - binary key/data with 0 NL?
// - thread safe
// - memory sharing
// - (multiple writers)
// - super efficiency

//  RECORD==line
//    key SPCS timestamp SPCS val u e\n

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

typedef struct kv {
  char *k, *v;
} kv;

#define KVSIZE 1024
// we own all values
kv kvstore[KVSIZE]= {0};
int kvn= 0, kvorder= 1;

// own: free k v later
void kv_put(char* k, char* v, int own) {
  assert(kvn<=KVSIZE);
  kvstore[kvn].k= own?k:strdup(k);
  kvstore[kvn].v= own?v:strdup(v);
  kvn++;
  // TODO: added is highest?
  kvorder= 0;
}

char* kv_get(char* k) {
  assert(k);
  
  for(int i=0; i<kvn; i++) {
    if (0==strcmp(kvstore[i].k, k)) return kvstore[i].v;
  }
  // no match
  return NULL;
}

// TODO: test
kv kv_read(FILE* f) {
  char *key=0, *val=0;
  long ts=0;
  int n= fscanf(f, "%ms %ld %ms\n", &key, &ts, &val);
  printf("READ:\n  key=%s\n  ts=%ld\n  val=%s\n", key, ts, val);
  // TODO: return ts?
  // TODO: deleted marker?
  return (kv){key, val};
}

void kv_readfile(FILE* f) {
  char* ln= 0; size_t z=0;
  while(getline(&ln, &z, f)>=0) {
    char *key=0, *val=0;
    long ts=0;
    int n= sscanf(ln, "%ms %ld %m[^\n]\n", &key, &ts, &val);
    printf("READ:\n  key=%s\n  ts=%ld\n  val=%s\n", key, ts, val);
    // TODO: return ts?
    // TODO: deleted marker?
    kv r= {key, val};
    assert(kvn<KVSIZE);
    kvstore[kvn]= r;
    kvn++;
  }
  free(ln);
}




// tests
int main(int argc, char** argv) {
  char* k= "0000001st";
  char* v= "first";
  kv_put(k, v, 0);
  kv_put("0000002nd", "second", 0);

  char* r= kv_get(k);
  char* kk= "0000002nd";
  char* rr= kv_get(kk);

  printf("%s => %s\n", k, r);
  printf("%s => %s\n", kk, rr);
  // get non-exist
  printf("%s => %s\n", "0000003nd", kv_get("0000003nd"));

  kv_readfile(stdin);

  return 0;
}
