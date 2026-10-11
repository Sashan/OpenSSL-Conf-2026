/*
 * tls-echo-server.c - minimal blocking TLS 1.3 echo server using OpenSSL.
 *
 * Usage: tls-echo-server <port> <cert.pem> <key.pem>
 *
 * Accepts TLS connections one at a time, reads everything the client
 * sends and writes it straight back.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

static void die(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    SSL_CTX *ctx;
    SSL *ssl;
    struct sockaddr_in sa = { 0 };
    unsigned char buf[4096];
    size_t nread, nwritten;
    int fd, cfd;

    if (argc != 4) {
        fprintf(stderr, "usage: %s <port> <cert.pem> <key.pem>\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* 1. Context: server method, certificate, key. */
    if ((ctx = SSL_CTX_new(TLS_server_method())) == NULL)
        die("SSL_CTX_new failed");
    if (SSL_CTX_use_certificate_chain_file(ctx, argv[2]) <= 0
        || SSL_CTX_use_PrivateKey_file(ctx, argv[3], SSL_FILETYPE_PEM) <= 0)
        die("cannot load certificate or key");

    /* 2. Create server socket. */
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        die("socket failed");
    sa.sin_family = AF_INET;
    sa.sin_port = htons((uint16_t)atoi(argv[1]));
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0
        || listen(fd, 8) < 0)
        die("bind/listen failed");

    printf("TLS echo server listening on TCP port %s\n", argv[1]);

    for (;;) {
        /* 3. The kernel accepts the TCP connection... */
        if ((cfd = accept(fd, NULL, NULL)) < 0)
            continue;

        /* 4. ...and OpenSSL runs the TLS handshake on top of it. */
        if ((ssl = SSL_new(ctx)) == NULL
            || !SSL_set_fd(ssl, cfd)
            || SSL_accept(ssl) != 1) {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(cfd);
            continue;
        }
        printf("client connected\n");

        /* 5. Echo: read from the client, write it back. */
        while (SSL_read_ex(ssl, buf, sizeof(buf), &nread)) {
            if (!SSL_write_ex(ssl, buf, nread, &nwritten))
                break;
            printf("echoed %zu bytes\n", nwritten);
        }

        /* 6. Client sent close_notify: answer with ours and close. */
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(cfd);
        printf("client disconnected\n");
    }

    /* not reached */
    SSL_CTX_free(ctx);
    close(fd);
    return EXIT_SUCCESS;
}
