#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kthread.h>
#include <linux/sched/signal.h>
#include <linux/tcp.h>

#include "http_parser.h"
#include "http_server.h"
#include "bignum.h"

#define CRLF "\r\n"

#define HTTP_RESPONSE_200_DUMMY                                \
    ""                                                         \
    "HTTP/1.1 200 OK" CRLF "Server: " KBUILD_MODNAME CRLF      \
    "Content-Type: text/plain" CRLF "Content-Length: %zu" CRLF \
    "Connection: Close" CRLF CRLF "%s" CRLF

#define HTTP_RESPONSE_200_KEEPALIVE_DUMMY                      \
    ""                                                         \
    "HTTP/1.1 200 OK" CRLF "Server: " KBUILD_MODNAME CRLF      \
    "Content-Type: text/plain" CRLF "Content-Length: %zu" CRLF \
    "Connection: Keep-Alive" CRLF CRLF "%s" CRLF

#define HTTP_RESPONSE_501                                              \
    ""                                                                 \
    "HTTP/1.1 501 Not Implemented" CRLF "Server: " KBUILD_MODNAME CRLF \
    "Content-Type: text/plain" CRLF "Content-Length: 21" CRLF          \
    "Connection: Close" CRLF CRLF "501 Not Implemented" CRLF

#define HTTP_RESPONSE_501_KEEPALIVE                                    \
    ""                                                                 \
    "HTTP/1.1 501 Not Implemented" CRLF "Server: " KBUILD_MODNAME CRLF \
    "Content-Type: text/plain" CRLF "Content-Length: 21" CRLF          \
    "Connection: KeepAlive" CRLF CRLF "501 Not Implemented" CRLF

#define RECV_BUFFER_SIZE 4096

struct http_request {
    struct socket *socket;
    enum http_method method;
    char request_url[128];
    int complete;
};

static int http_server_recv(struct socket *sock, char *buf, size_t size)
{
    struct kvec iov = {.iov_base = (void *) buf, .iov_len = size};
    struct msghdr msg = {.msg_name = 0,
                         .msg_namelen = 0,
                         .msg_control = NULL,
                         .msg_controllen = 0,
                         .msg_flags = 0};
    return kernel_recvmsg(sock, &msg, &iov, 1, size, msg.msg_flags);
}

static int http_server_send(struct socket *sock, const char *buf, size_t size)
{
    struct msghdr msg = {.msg_name = NULL,
                         .msg_namelen = 0,
                         .msg_control = NULL,
                         .msg_controllen = 0,
                         .msg_flags = 0};
    int done = 0;
    while (done < size) {
        struct kvec iov = {
            .iov_base = (void *) ((char *) buf + done), .iov_len = size - done,
        };
        int length = kernel_sendmsg(sock, &msg, &iov, 1, iov.iov_len);
        if (length < 0) {
            pr_err("write error: %d\n", length);
            break;
        }
        done += length;
    }
    return done;
}

char *respmsg_edition(char *msg, int keep_alive)
{
    char *rpmsg = NULL;
    int msgl;
    size_t rpmsglen;

    // Calculate space needed for response message return
    // KEEPALIVE_DUMMY (105) vs DUMMY(100) -> for avoiding cppcheck false
    // trigger
    rpmsglen = keep_alive ? 105 : 100;

    // Add msg length into needed space
    rpmsglen += strlen(msg);

    // Probing the length of strlen(msg) and add into needed space
    rpmsg = (char *) kcalloc(strlen(msg) + 1, sizeof(char), GFP_KERNEL);

    if (rpmsg == NULL) {
        pr_err("Allocate space for response message fail...");
        return NULL;
    }

    msgl = snprintf(rpmsg, strlen(msg), "%zu", strlen(msg));
    if (msgl > 0) {
        rpmsglen += msgl;
    }
    kfree(rpmsg);

    /* Formal allocation for response needed */
    rpmsg = (char *) kcalloc(rpmsglen, sizeof(char), GFP_KERNEL);

    /* Compose formal HTTP response */
    keep_alive
        ? snprintf(rpmsg, rpmsglen, HTTP_RESPONSE_200_KEEPALIVE_DUMMY,
                   strlen(msg), msg)
        : snprintf(rpmsg, rpmsglen, HTTP_RESPONSE_200_DUMMY, strlen(msg), msg);

    // pr_info("HTTP response snprintf result: %d", msgl);

    return rpmsg;
}

static int http_server_response(struct http_request *request, int keep_alive)
{
    char *response, *url = NULL, *ptr_n, *ptr_i, /*fib_s,*/ *rpmsg = NULL;
    long long fib_input;
    int kres;
    bignum_t *bn_res;

    /* Allocate string space for url copying */
    url = (char *) kcalloc(strlen(request->request_url), sizeof(char),
                           GFP_KERNEL);

    /* Prevent allocation fail case*/
    if (url == NULL) {
        pr_err("Allocate url space fail!");

        rpmsg = (char *) kcalloc(sizeof("Allocate url space fail!\n"),
                                 sizeof(char), GFP_KERNEL);
        strncat(rpmsg, "Allocate url space fail!\n",
                sizeof("Allocate url space fail!\n"));
        goto rsp;  // Response allocation error directly
    }

    /* Copying URL */
    strncpy(url, request->request_url + 1, strlen(request->request_url));
    ptr_n = url;

    /* Seperate instruction pattern and requested number pattern */
    /* Note: ptr_i for instruction pattern, ptr_n for number pattern */
    ptr_i = strsep(&ptr_n, "/");

    pr_info("sep / -> number: %s, instruction: %s\n", ptr_n, ptr_i);

    /* Check if the instruction pattern is matched... */
    if (strncmp(ptr_i, "fib", 4) == 0) {
        /* Transfer input number (dec.) to type long long (fit bn_fibonacci(long
         * long))
         */
        kres = kstrtoll(ptr_n, 10, &fib_input);

        /* Calculate fibonacci number while return success */
        if (kres == 0) {
            /* CPU bound task, disable preemption for better performance */
            // preempt_disable();

            /* Calculate fibonacci number */
            bn_res = bn_fibonacci_fd(fib_input);

            /* Casting bignum_t to string */
            rpmsg = bn_tostring(&bn_res);

            bn_free(&bn_res);

            /* Enable preemption */
            // preempt_enable();

        } else {
            // pr_err("Input to long long fail, fail code: %d", kres);

            rpmsg = (char *) kcalloc(
                sizeof("Input to long long fail, fail code: ") + 5,
                sizeof(char), GFP_KERNEL);
            snprintf(rpmsg, sizeof("Input to long long fail, fail code: ") + 5,
                     "Input to long long fail, fail code: %d\n", kres);
        }

    } else {
        rpmsg =
            (char *) kcalloc(sizeof("Instruction pattern is NOT matched!\n"),
                             sizeof(char), GFP_KERNEL);
        strncat(rpmsg, "Instruction pattern is NOT matched!\n",
                sizeof("Instruction pattern is NOT matched!"));
    }

// Integrate response message to formal HTTP response!
rsp:

    if (request->method != HTTP_GET)
        response = keep_alive ? HTTP_RESPONSE_501_KEEPALIVE : HTTP_RESPONSE_501;
    else
        response = respmsg_edition(rpmsg, keep_alive);

    /* Response to client while response is not NULL */
    if (response != NULL)
        http_server_send(request->socket, response, strlen(response));

    /* Free allocated memory space */
    if (url != NULL)
        kfree(url);
    if (rpmsg != NULL)
        kfree(rpmsg);
    if (request->method == HTTP_GET && response != NULL)
        kfree(response);
    return 0;
}

static int http_parser_callback_message_begin(http_parser *parser)
{
    struct http_request *request = parser->data;
    struct socket *socket = request->socket;
    memset(request, 0x00, sizeof(struct http_request));
    request->socket = socket;
    return 0;
}

static int http_parser_callback_request_url(http_parser *parser,
                                            const char *p,
                                            size_t len)
{
    struct http_request *request = parser->data;
    strncat(request->request_url, p, len);
    return 0;
}

static int http_parser_callback_header_field(http_parser *parser,
                                             const char *p,
                                             size_t len)
{
    return 0;
}

static int http_parser_callback_header_value(http_parser *parser,
                                             const char *p,
                                             size_t len)
{
    return 0;
}

static int http_parser_callback_headers_complete(http_parser *parser)
{
    struct http_request *request = parser->data;
    request->method = parser->method;
    return 0;
}

static int http_parser_callback_body(http_parser *parser,
                                     const char *p,
                                     size_t len)
{
    return 0;
}

static int http_parser_callback_message_complete(http_parser *parser)
{
    struct http_request *request = parser->data;
    http_server_response(request, http_should_keep_alive(parser));
    request->complete = 1;
    return 0;
}

static int http_server_worker(void *arg)
{
    char *buf;
    struct http_parser parser;
    struct http_parser_settings setting = {
        .on_message_begin = http_parser_callback_message_begin,
        .on_url = http_parser_callback_request_url,
        .on_header_field = http_parser_callback_header_field,
        .on_header_value = http_parser_callback_header_value,
        .on_headers_complete = http_parser_callback_headers_complete,
        .on_body = http_parser_callback_body,
        .on_message_complete = http_parser_callback_message_complete};
    struct http_request request;
    struct socket *socket = (struct socket *) arg;

    allow_signal(SIGKILL);
    allow_signal(SIGTERM);

    buf = kmalloc(RECV_BUFFER_SIZE, GFP_KERNEL);
    if (!buf) {
        pr_err("can't allocate memory!\n");
        return -1;
    }

    request.socket = socket;
    http_parser_init(&parser, HTTP_REQUEST);
    parser.data = &request;
    /* Blocking receiving */
    while (!kthread_should_stop()) {
        int ret = http_server_recv(socket, buf, RECV_BUFFER_SIZE - 1);
        if (ret <= 0) {
            if (ret)
                pr_err("recv error: %d\n", ret);
            break;
        }
        http_parser_execute(&parser, &setting, buf, ret);
        if (request.complete && !http_should_keep_alive(&parser))
            break;
    }
    kernel_sock_shutdown(socket, SHUT_RDWR);
    sock_release(socket);
    kfree(buf);
    return 0;
}

int http_server_daemon(void *arg)
{
    struct socket *socket;
    struct task_struct *worker;
    struct http_server_param *param = (struct http_server_param *) arg;

    allow_signal(SIGKILL);
    allow_signal(SIGTERM);

    while (!kthread_should_stop()) {
        // Accept connection via kernel socket API
        int err = kernel_accept(param->listen_socket, &socket, 0);
        if (err < 0) {
            if (signal_pending(current))
                break;
            pr_err("kernel_accept() error: %d\n", err);
            continue;
        }
        // Create a kernel thread for request handling - callback:
        // http_server_worker
        worker = kthread_run(http_server_worker, socket, KBUILD_MODNAME);
        if (IS_ERR(worker)) {
            pr_err("can't create more worker process\n");
            continue;
        }
    }
    return 0;
}
