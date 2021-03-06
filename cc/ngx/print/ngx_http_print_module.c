#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static char *ngx_http_print(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_command_t  ngx_http_print_commands[] = {
    { ngx_string("print"),
      NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
      ngx_http_print,
      0,
      0,
      NULL },

      ngx_null_command
};

static ngx_http_module_t  ngx_http_print_module_ctx = {
    NULL,                          /* preconfiguration */
    NULL,                          /* postconfiguration */
    NULL,                          /* create main configuration */
    NULL,                          /* init main configuration */
    NULL,                          /* create server configuration */
    NULL,                          /* merge server configuration */
    NULL,                          /* create location configuration */
    NULL                           /* merge location configuration */
};

ngx_module_t  ngx_http_print_module = {
    NGX_MODULE_V1,
    &ngx_http_print_module_ctx,      /* module context */
    ngx_http_print_commands,         /* module directives */
    NGX_HTTP_MODULE,               /* module type */
    NULL,                          /* init master */
    NULL,                          /* init module */
    NULL,                          /* init process */
    NULL,                          /* init thread */
    NULL,                          /* exit thread */
    NULL,                          /* exit process */
    NULL,                          /* exit master */
    NGX_MODULE_V1_PADDING
};

static char html_content[] = 
"<html>\n"
"<head>\n"
"<title>print module</title>\n"
"</head>\n"
"<body>nginx_print</body>\n"
"</html>\n";

static char html_content0[] = "<html>\n";
static char html_content1[] = "<head>\n";
static char html_content2[] = "<title>print module</title>\n";
static char html_content3[] = "</head>\n";
static char html_content4[] = "<body>nginx_print</body>\n";
static char html_content5[] = "</html>\n";

//static void ngx_print(ngx_http_request_t *r, ngx_chain_t* out, char* src, size_t n);
static ngx_chain_t *ngx_print(ngx_http_request_t *r, ngx_chain_t *last, char *src, size_t n, ngx_int_t *ret);

static ngx_int_t
ngx_http_print_handler(ngx_http_request_t *r)
{
	ngx_int_t rc;
	ngx_int_t err;
	ngx_chain_t *out;
	ngx_chain_t *next;
	ngx_chain_t *prev;

    if (!(r->method & (NGX_HTTP_GET|NGX_HTTP_HEAD))) {
        return NGX_HTTP_NOT_ALLOWED;
    }

	rc = ngx_http_discard_request_body(r);
	if (rc != NGX_OK && rc != NGX_AGAIN) {
		return rc;
	}

	r->headers_out.status = NGX_HTTP_OK;
	r->headers_out.content_type.len = sizeof("text/html") - 1;
	r->headers_out.content_type.data = (u_char *) "text/html";
	r->headers_out.content_length_n = sizeof(html_content) - 1;

	//ngx_print(r, &out, html_content, sizeof(html_content) - 1);

	prev = NULL;

	next = ngx_print(r, NULL, html_content0, sizeof(html_content0) - 1, &err);
	out = prev = next;

	next = ngx_print(r, prev, html_content1, sizeof(html_content1) - 1, &err);
	prev = next;

	next = ngx_print(r, prev, html_content2, sizeof(html_content2) - 1, &err);
	prev = next;

	next = ngx_print(r, prev, html_content3, sizeof(html_content3) - 1, &err);
	prev = next;

	next = ngx_print(r, prev, html_content4, sizeof(html_content4) - 1, &err);
	prev = next;

	next = ngx_print(r, prev, html_content5, sizeof(html_content5) - 1, &err);

	rc = ngx_http_send_header(r);
	if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
		return rc;
	}

    //return ngx_http_output_filter(r, &out);
    return ngx_http_output_filter(r, out);
}

static char *
ngx_http_print(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_print_handler;

    return NGX_CONF_OK;
}

#if 0
static void
ngx_print(ngx_http_request_t *r, ngx_chain_t* out, char* src, size_t n)
{
	ngx_buf_t* buf;
	unsigned char *b;

    buf = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    if (buf == NULL) {
        return;
    }

	b = ngx_palloc(r->pool, n);
	if (b == NULL) {
		return;
	}
	ngx_memcpy(b, src, n);

	buf->pos = b;
	buf->last = b + n;
	buf->memory = 1;
	buf->last_buf = 1;
    buf->last_in_chain = 1;

	out->buf = buf;
	out->next = NULL;

	return;
}
#endif

static ngx_chain_t *
ngx_print(ngx_http_request_t *r, ngx_chain_t *last, char *src, size_t n, ngx_int_t *ret)
{
    u_char    *p;
    ngx_buf_t *b;
	ngx_chain_t *cl;

	p = ngx_palloc(r->pool, n);
	if (p == NULL) {
		*ret = NGX_ERROR;
		return NULL;
	}
	ngx_memcpy(p, src, n);

	b = ngx_calloc_buf(r->pool);
	if (b == NULL) {
		*ret = NGX_ERROR;
		return NULL;
	}

	cl = ngx_alloc_chain_link(r->pool);
	if (cl == NULL) {
		*ret = NGX_ERROR;
		return NULL;
	}

	b->memory = 1;
	b->last_buf = 1;
	b->pos = p;
	b->last = p + n;

	cl->buf = b;
	cl->next = NULL;

	*ret = NGX_OK;

	if (last != NULL) {
		last->next = cl;
	}

	return cl;
}

