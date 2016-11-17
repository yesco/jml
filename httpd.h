int wget(void* data, char* url, int out(void* data, char* s));

typedef void (*httpd_header)(char* buffer, char* method, char* path); // will be called for each header line, last time NULL
typedef void (*httpd_body)(char* buffer, char* method, char* path); // may be called several times, last time NULL
typedef void (*httpd_response)(int req, char* method, char* path); // you can write(req, ... don't close it, it'll be closed for you

int httpd_init(int port);

// all callbacks are optional
int httpd_next(int s, httpd_header emit_header, httpd_body emit_body, httpd_response emit_response);

// call default printer for testing, never returns
void httpd_loop(int s);

