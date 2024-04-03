# File-Transfer-Application
A file transfer application over a UDP socket in a client- server mode.

UDP, by nature, is an unreliable protocol. Hence, the file transfer application is
will ensure reliability by retransmitting packets and insequencing out-of-order
packets. It can be achieved using Stop-n-Wait, Go-Back-N, and Selective-Repeat.
Technique used:
1. The standard TFTP (Trivial File Transfer Protocol) uses a Stop-n-Wait approach. A TFTP client is developed to work with the standard TFTP server.

