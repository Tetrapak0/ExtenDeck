#pragma once

#define MCAST_IP   "239.69.69.69" // 239.0.0.0 - 239.255.255.255
#define MCAST_PORT 38489
//username@hostname (ip:port)+'\0'
#define BUFSIZE    (256+1+255+2+15+1+5+1+1) // 537 // MAX
