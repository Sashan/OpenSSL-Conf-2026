/*
 * tls-echo-client.c - minimal blocking TLS 1.3 echo client using OpenSSL.
 *
 * Usage: tls-echo-client <host> <port> <message>
 *
 * Connects to the server, sends <message>, signals end of data with
 * close_notify and prints whatever the server sends back.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

static void die(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
}

/* Create a socket connected to host:port. */
static int do_sock_connect(const char *host, const char *port)
{
    struct addrinfo hints = { 0 }, *res, *ai;
    int fd = -1;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, port, &hints, &res) != 0)
        return -1;
    for (ai = res; ai != NULL; ai = ai->ai_next) {
        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd >= 0 && connect(fd, ai->ai_addr, ai->ai_addrlen) == 0)
            break;
        if (fd >= 0)
            close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    return fd;
}

int main(int argc, char *argv[])
{
    SSL_CTX *ctx;
    SSL *ssl;
    char buf[4096];
    size_t nread, nwritten;
    int fd;

    if (argc != 4) {
        fprintf(stderr, "usage: %s <host> <port> <message>\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    /* 1. Context: client method. */
    if ((ctx = SSL_CTX_new(TLS_client_method())) == NULL)
        die("SSL_CTX_new failed");
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

    /* 2. A socket "connected" to the server. */
    if ((fd = do_sock_connect(argv[1], argv[2])) < 0)
        die("cannot connect socket");

    /* 3. connection object: socket, SNI, hostname check. */
    if ((ssl = SSL_new(ctx)) == NULL
        || !SSL_set_fd(ssl, fd)
        || !SSL_set_tlsext_host_name(ssl, argv[1])
        || !SSL_set1_dnsname(ssl, argv[1]))
        die("cannot set up connection");

    /* 4. handshake. */
    if (SSL_connect(ssl) != 1)
        die("handshake failed");

    /* 5. Send request, then close_notify: "I have nothing more to send". */
    if (!SSL_write_ex(ssl, argv[3], strlen(argv[3]), &nwritten)
        || SSL_shutdown(ssl) < 0)
        die("write failed");

    /* 6. Read the echo until the server sends its close_notify. */
    while (SSL_read_ex(ssl, buf, sizeof(buf), &nread))
        fwrite(buf, 1, nread, stdout);
    putchar('\n');

    if (SSL_get_error(ssl, 0) != SSL_ERROR_ZERO_RETURN)
        die("read failed");

    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(fd);
    return EXIT_SUCCESS;
}
