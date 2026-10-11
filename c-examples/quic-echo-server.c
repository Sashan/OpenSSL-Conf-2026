/*
 * quic-echo-server.c - minimal blocking QUIC echo server using OpenSSL.
 *
 * Usage: quic-echo-server <port> <cert.pem> <key.pem>
 *
 * Accepts QUIC connections one at a time, reads everything the client
 * sends on its stream and writes it straight back.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/quic.h>

/* ALPN is mandatory in QUIC; client and server must agree on it. */
static const unsigned char alpn[] = { 4, 'e', 'c', 'h', 'o' };

static int select_alpn(SSL *ssl, const unsigned char **out,
                       unsigned char *outlen, const unsigned char *in,
                       unsigned int inlen, void *arg)
{
    if (SSL_select_next_proto((unsigned char **)out, outlen, alpn,
                              sizeof(alpn), in, inlen) == OPENSSL_NPN_NEGOTIATED)
        return SSL_TLSEXT_ERR_OK;
    return SSL_TLSEXT_ERR_ALERT_FATAL;
}

static void die(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    SSL_CTX *ctx;
    SSL *listener, *conn;
    struct sockaddr_in sa = { 0 };
    unsigned char buf[4096];
    size_t nread, nwritten;
    int fd;

    if (argc != 4) {
        fprintf(stderr, "usage: %s <port> <cert.pem> <key.pem>\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* 1. Context: server method, certificate, ey + ALPN. */
    if ((ctx = SSL_CTX_new(OSSL_QUIC_server_method())) == NULL)
        die("SSL_CTX_new failed");
    if (SSL_CTX_use_certificate_chain_file(ctx, argv[2]) <= 0
        || SSL_CTX_use_PrivateKey_file(ctx, argv[3], SSL_FILETYPE_PEM) <= 0)
        die("cannot load certificate or key");
    SSL_CTX_set_alpn_select_cb(ctx, select_alpn, NULL);

    /* 2. Create server socket. */
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        die("socket failed");
    sa.sin_family = AF_INET;
    sa.sin_port = htons((uint16_t)atoi(argv[1]));
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
        die("bind failed");

    /* 3. A listener object owns the socket and accepts connections. */
    if ((listener = SSL_new_listener(ctx, 0)) == NULL
        || !SSL_set_fd(listener, fd)
        || !SSL_listen(listener))
        die("cannot start listener");

    printf("QUIC echo server listening on UDP port %s\n", argv[1]);

    for (;;) {
        /* 4. Block until a client completes the handshake. */
        if ((conn = SSL_accept_connection(listener, 0)) == NULL) {
            ERR_print_errors_fp(stderr);
            continue;
        }
        printf("client connected\n");

        /* 5. Echo: read from the client's stream, write it back. */
        while (SSL_read_ex(conn, buf, sizeof(buf), &nread)) {
            if (!SSL_write_ex(conn, buf, nread, &nwritten))
                break;
            printf("echoed %zu bytes\n", nwritten);
        }

        /* 6. Client sent FIN: send our FIN, then close the connection. */
        SSL_stream_conclude(conn, 0);
        while (SSL_shutdown(conn) != 1)
            continue;
        SSL_free(conn);
        printf("client disconnected\n");
    }

    /* not reached */
    SSL_free(listener);
    SSL_CTX_free(ctx);
    close(fd);
    return EXIT_SUCCESS;
}
