/*
 * UDP Echo Server — User-Space Program for ToyOS
 *
 * THIS IS THE WHOLE POINT:
 * Previously, the echo server lived inside the kernel (in udp.c's
 * udp_echo() function). That worked, but it's not how real OSes do it.
 * Network services should run as user programs, isolated from the kernel.
 *
 * Now the echo logic is here, in user space. It uses system calls to:
 *   1. Create a UDP socket         (toyos_socket)
 *   2. Bind to port 7              (toyos_bind)
 *   3. Receive packets             (toyos_recvfrom)
 *   4. Send them back              (toyos_sendto)
 *
 * The kernel's job is just to deliver packets to the right socket and
 * send them out — it doesn't know or care what the application does
 * with the data.
 *
 * To test from the host:
 *   echo "hello" | nc -u -w1 10.0.2.15 7
 *
 * To run from the ToyOS shell:
 *   udpecho
 */

#include "udpecho.h"
#include "stdio.h"
#include "toyos.h"

#define ECHO_PORT 7
#define BUF_SIZE 1024

int main(int argc, char **argv) {
    print("UDP Echo Server starting on port 7...\n");

    /*
     * Step 1: Create a UDP socket.
     * SOCK_DGRAM (2) means "datagram" — connectionless, unreliable.
     * The kernel allocates a socket structure and returns a descriptor.
     */
    int sock = toyos_socket(SOCK_DGRAM);
    if (sock < 0) {
        print("Failed to create socket!\n");
        return 1;
    }

    /*
     * Step 2: Bind to port 7 (the echo port, RFC 862).
     * This tells the kernel: "any UDP packet arriving on port 7
     * should go into THIS socket's receive queue."
     * Without binding, packets on port 7 would be dropped.
     */
    if (toyos_bind(sock, ECHO_PORT) < 0) {
        print("Failed to bind to port 7!\n");
        return 1;
    }

    print("Listening on UDP port 7. Send packets with:\n");
    print("  echo \"hello\" | nc -u -w1 10.0.2.15 7\n\n");

    /*
     * Step 3: Main loop — receive and echo back.
     *
     * recvfrom() is non-blocking: it returns 0 if no packet is
     * available. So we poll in a loop. This is wasteful (burns CPU),
     * but we don't have a sleep/wakeup mechanism yet.
     *
     * A real OS would use select(), poll(), or epoll() to block
     * until data arrives. That requires the kernel to put the
     * process to sleep and wake it on packet arrival.
     */
    char buf[BUF_SIZE];
    struct recvfrom_args recv_args;
    recv_args.sockfd = sock;
    recv_args.buf = buf;
    recv_args.max_len = BUF_SIZE;

    while (1) {
        int n = toyos_recvfrom(&recv_args);

        if (n > 0) {
            /*
             * Got a packet! Echo it back to the sender.
             * recv_args.src_ip and recv_args.src_port were filled
             * by the kernel with the sender's address.
             */
            struct sendto_args send_args;
            send_args.sockfd = sock;
            send_args.buf = buf;
            send_args.len = n;
            send_args.dst_ip[0] = recv_args.src_ip[0];
            send_args.dst_ip[1] = recv_args.src_ip[1];
            send_args.dst_ip[2] = recv_args.src_ip[2];
            send_args.dst_ip[3] = recv_args.src_ip[3];
            send_args.dst_port = recv_args.src_port;

            toyos_sendto(&send_args);
        }
    }

    return 0;
}
