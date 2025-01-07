/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#include "udp.h"



int CreateSocket(uint16_t bind_port, int retries) {
   int s;
   sockaddr_in sin;
   uint16_t port;
   int optval = 1;

   s = socket(AF_INET, SOCK_DGRAM, 0);
   if (s == -1) {
      perror("Failed to create socket");
      return -1;
   }

   setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

   // Set non-blocking mode
   int flags = fcntl(s, F_GETFL, 0);
   if (flags == -1 || fcntl(s, F_SETFL, flags | O_NONBLOCK) == -1) {
      perror("Failed to set non-blocking mode");
      close(s);
      return -1;
   }

   sin.sin_family = AF_INET;
   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   for (port = bind_port; port <= bind_port + retries; port++) {
      sin.sin_port = htons(port);
      if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) == 0) {
         printf("Udp bound to port: %d.\n", port);
         return s;
      }
   }
   perror("Failed to bind socket");
   close(s);
   return -1;
}

Udp::Udp() :
   _socket(-1),
   _callbacks(nullptr) {
}

Udp::~Udp(void) {
   if (_socket != -1) {
      close(_socket);
      _socket = -1;
   }
}

void Udp::Init(uint16_t port, Poll *poll, Callbacks *callbacks) {
   _callbacks = callbacks;

   _poll = poll;
   _poll->RegisterLoop(this);

   printf("Binding UDP socket to port %d.\n", port);
   _socket = CreateSocket(port, 0);
   if (_socket == -1) {
      throw std::runtime_error("Failed to initialize UDP socket");
   }
}

void Udp::SendTo(char *buffer, int len, int flags, struct sockaddr *dst, socklen_t destlen) {
   struct sockaddr_in *to = (struct sockaddr_in *)dst;

   int res = sendto(_socket, buffer, len, flags, dst, destlen);
   if (res == -1) {
      int err = errno;
      perror("Error in sendto");
      printf("sendto error: %d\n", err);
      throw std::runtime_error("sendto failed");
   }

   char dst_ip[INET_ADDRSTRLEN];
   printf("Sent packet length %d to %s:%d (ret:%d).\n", len,
          inet_ntop(AF_INET, &to->sin_addr, dst_ip, sizeof(dst_ip)),
          ntohs(to->sin_port), res);
}

bool Udp::OnLoopPoll(void *cookie) {
   uint8_t recv_buf[MAX_UDP_PACKET_SIZE];
   sockaddr_in recv_addr;
   socklen_t recv_addr_len;

   for (;;) {
      recv_addr_len = sizeof(recv_addr);
      int len = recvfrom(_socket, recv_buf, MAX_UDP_PACKET_SIZE, 0, (struct sockaddr *)&recv_addr, &recv_addr_len);

      if (len == -1) {
         if (errno != EWOULDBLOCK && errno != EAGAIN) {
            perror("recvfrom error");
         }
         break;
      } else if (len > 0) {
         char src_ip[INET_ADDRSTRLEN];
         printf("recvfrom returned (len:%d from:%s:%d).\n", len,
                inet_ntop(AF_INET, &recv_addr.sin_addr, src_ip, sizeof(src_ip)),
                ntohs(recv_addr.sin_port));
         UdpMsg *msg = (UdpMsg *)recv_buf;
         _callbacks->OnMsg(recv_addr, msg, len);
      }
   }
   return true;
}

void Udp::Log(const char *fmt, ...) {
   char buf[1024];
   size_t offset;
   va_list args;

   strcpy(buf, "udp | ");
   offset = strlen(buf);
   va_start(args, fmt);
   vsnprintf(buf + offset, sizeof(buf) - offset - 1, fmt, args);
   buf[sizeof(buf) - 1] = '\0';
   printf("%s\n", buf);
   va_end(args);
}
