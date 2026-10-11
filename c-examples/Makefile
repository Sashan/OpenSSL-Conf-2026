# Build against an OpenSSL >= 3.5 installation or build tree, e.g.:
#   make OPENSSL_DIR=$HOME/work.openssl/quic-demo/openssl
OPENSSL_DIR ?= /usr
CFLAGS = -g -Wall -O0 -I$(OPENSSL_DIR)/include
LDFLAGS += -L$(OPENSSL_DIR) -L$(OPENSSL_DIR)/lib -Wl,-rpath,$(OPENSSL_DIR)
LDLIBS = -lssl -lcrypto

PROGS = quic-echo-server quic-echo-client tls-echo-server tls-echo-client

all: $(PROGS)

# Self-signed certificate for localhost; the client uses it as its CA.
cert: cert.pem key.pem
cert.pem key.pem:
	openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:P-256 -nodes \
	    -keyout key.pem -out cert.pem -days 365 -subj /CN=localhost \
	    -addext subjectAltName=DNS:localhost

clean:
	rm -f $(PROGS) cert.pem key.pem

.PHONY: all cert clean
