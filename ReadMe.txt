This repository contains demos I present during OpenSSL 2026 conference

c-examples directory contains echo client/servers for TLS and quic.
it just demonstrates the libssl API to QUIC protocol. Just to show
how much close to existing TLS application it is.

flowd-stats.sh is shell scripts that processes firewall log from
pflow(4). It generates pie charts showing volumes of HTTP/HTTPS/QUIC/other
traffic.

quic-demo-sites contains configuration files for OpenBSD server
I used to to deploy nginx serving https (https://quic-demo.sashan.net)
and QUIT (https://quic-demo-h3.sashan.net) sites.

Note when also demonstrating packet drop you need to patch
ip_output() so it does not let sendmsg() fail with EACCESS
for local outbound packets.

----8<----8<----8<----8<----8<----8<----8<----
diff --git a/sys/netinet/ip_output.c b/sys/netinet/ip_output.c
index 2fa2f3f0d8a..d00bbfc0b37 100644
--- a/sys/netinet/ip_output.c
+++ b/sys/netinet/ip_output.c
@@ -400,7 +400,7 @@ reroute:
 #if NPF > 0
 	if (pf_test(AF_INET, (flags & IP_FORWARDING) ? PF_FWD : PF_OUT,
 	    ifp, &m) != PF_PASS) {
-		error = EACCES;
+		error = 0;
 		goto bad;
 	}
 	if (m == NULL)
----8<----8<----8<----8<----8<----8<----8<----


more details on this here [1]

[1] https://github.com/nginx/nginx/pull/1835
