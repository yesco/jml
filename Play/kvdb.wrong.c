// A simple KeyValue DataBase
//
// Goals:
// - simple small C code
// - range search
// - append only filesystem based
// - many readers same machine
// - distributable ("tail -f")
// - single writer
// - multi reader concurrent
// - transactions - ACID
// - crash safe/recoverable
// - timestamped
//
// Non-Goals:
// - binary key/data with 0 NL?
// - thread safe
// - memory sharing
// - multiple writers
// - concurrent writers
// - super efficiency

// Possible data formats:
// 1. plain "text" (no 0 no NL)
//    + can sort do unix stuff
//    + .sorted.4711 till offset
//    - quote 9 and NL
//    - => w mmap ret needs malloc
// 2. prefix compressed keys
//    - key return have to malloc
//    - non unix sortable
//    - "have to be sorted"
//
//  RECORD:
//    
//    checksum:int32 (post reclen)
//    reclen:int
//
//    <prefix n from prev>:int
//    UABytes:int
//    <Unique Array Bytes>
//
//    '=' just because
//
//    DABytes: int
//    <Data Array Bytes>
//
//    newline just because


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
// we own all values
kv kvstore[KVSIZE]= {0};
int kvn= 0, kvorder= 1;

// normalizes
// if d.len == 0 it's a string if have pointer, set len
data ddup(data d, int own) {
  // fix length
  d.len= d.v? (d.len?d.len:strlen(d.v)) : 0;
  if (!own && d.v)
    d.v= memcpy(malloc(d.len), d.v, d.len);
  return d;
}

void dfree(data *d) { free(d->v); d->v= NULL; }

// own: free k.d, v.d later
void kv_put(data k, data v, int own) {
  assert(kvn<=KVSIZE);
  kvstore[kvn].k= ddup(k, own);
  kvstore[kvn].v= ddup(v, own);
  kvn++;
  // TODO: added is highest?
  kvorder= 0;
}

void kv_puts(char* k, char* v, int own) {
  assert(k);
  assert(v); // TODO: == delete?
  data kd= {0, k}, vd= {0, v};
  kv_put(kd, vd, own);
}

data kv_get(data k) {
  k= ddup(k, 1); // normalize
  size_t len= k.len;
  // get NULL? == first? == min?
  assert(len);
  assert(k.v);
  
  for(int i=0; i<kvn; i++) {
    size_t clen= kvstore[i].k.len;
    if (clen==len && 0==memcmp(kvstore[i].k.v, k.v, len)) return kvstore[i].v;
  }
  // no match
  return (data){0, NULL};
}

char* kv_gets(char* k) {
  data kd= {0, k};
  data vd= kv_get(kd);
  return vd.v;
}

// read positive int
// 7 bits lo data, hi cont bit
// (varint encoding)
// NOTE: Not good for negatives...

// TODO: test
int readint(FILE* f) {
  int c= fgetc(f);
  assert(c!=EOF);
  return (c&128) + (c<128? 0: 128*readint(f));
}

// TODO: test
// TODO: howto calc CS? (write to  buffer?)
int writeint(FILE* f, int i) {
  assert(i>=0);
  int c= 0;
  do {
    int n= fputc((i&127)+(i>=128?128:0), f);
    assert(n==1);
    i>>= 7;
  } while(i);
  return 1;
}

// TODO: test
kv kv_read(FILE* f) {
  int cs= readint(f);
  int rlen= readint(f);

  int prefix= readint(f);
  int klen= readint(f);
  data k={prefix+klen, malloc(prefix+klen)};
  assert(k.v);

  // get prefix from prev key
  if (prefix) {
    assert(kvn);
    int plen= kvstore[kvn-1].k.len;
    assert(plen==prefix);
    memcpy(k.v, kvstore[kvn-1].k.v, prefix);
  }

  // must have as it's bigger
  assert(klen);
  int kn= fread(k.v+prefix, 1, klen, f);
  assert(kn==klen);

  // expect '='
  int eq= fgetc(f);
  assert(eq=='=');

  // read value
  int vlen= readint(f);
  // TODO: deleted marker? == 0
  assert(vlen);
  data v={vlen, malloc(vlen)};
  assert(v.v);
  int vn= fread(v.v, 1, vlen, f);
  assert(vn==vlen);

  // expect NL
  int nl= fgetc(f);
  assert(nl=='\n');

  // TODO: check checksum
  kv r= {k, v};
  return r;
}

void kv_readfile(FILE* f) {
  while(!feof(f)) {
    kv r= kv_read(f);
    if (!r.v.v) break;

    assert(kvn<KVSIZE);
    kvstore[kvn]= r;
    kvn++;
  }
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
