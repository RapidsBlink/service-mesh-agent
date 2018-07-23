#include <time.h>
#include <sys/time.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "jvm_cmd.h"
#include "utils.h"
#include "log.h"


int64_t get_wall_time() {
    struct timeval time;
    if (gettimeofday(&time, NULL)) {
        //  Handle error
        return 0;
    }
    return time.tv_sec * 1000000 + time.tv_usec;
}

//input: /dubbomesh/com.alibaba.dubbo.performance.demo.provider.IHelloService/[S,M,L]:192.168.1.2:8200
void str_to_server_uri(char *str, char *server_addr, int *port, int *scale) {
    //log_info("raw server addr: %s", str);
    char *pch;
    pch = strtok(str + 1, "/");
    char *server_uri = NULL;
    while (pch != NULL) {
        server_uri = pch;
        //printf ("%s\n",pch);
        pch = strtok(NULL, "/");
    }
    //log_info("server_uri: %s", server_uri);
    pch = strtok(server_uri, ":");
    if (pch[0] == '0') {
        *scale = 0;
    } else if (pch[0] == '1') {
        *scale = 1;
    } else if (pch[0] == '2') {
        *scale = 2;
    } else {
        log_fatal("unknown provider scale %s", pch);
        exit(-1);
    }
    strcpy(server_addr, strtok(NULL, ":"));
    *port = (int) strtol(strtok(NULL, ":"), NULL, 10);
}

char *hostIPaddr(char *eth_name) {
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];
    char *res = malloc(sizeof(char) * NI_MAXHOST);
    res[0] = '\0';
    if (getifaddrs(&ifaddr) == -1) {
        log_fatal("getifaddrs error");
        exit(EXIT_FAILURE);
    }

    /* Walk through linked list, maintaining head pointer so we
        can free list later */

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

//        /* Display interface name and family (including symbolic
//            form of the latter for the common families) */
//
//        printf("%s  address family: %d%s\n",
//               ifa->ifa_name, family,
//               (family == AF_PACKET) ? " (AF_PACKET)" :
//               (family == AF_INET) ? " (AF_INET)" :
//               (family == AF_INET6) ? " (AF_INET6)" : "");

        /* For an AF_INET* interface address, display the address */

        if (family == AF_INET || family == AF_INET6) {
            s = getnameinfo(ifa->ifa_addr,
                            (family == AF_INET) ? sizeof(struct sockaddr_in) :
                            sizeof(struct sockaddr_in6),
                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                log_fatal("getnameinfo() failed: %s", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            if ((family == AF_INET) && (strcmp(eth_name, ifa->ifa_name) == 0)) {
                strcpy(res, host);
            }
            //log_info("%s\taddress: <%s>", ifa->ifa_name, host);
        }
    }
    freeifaddrs(ifaddr);
    return res;
}

int four_char_to_int(char *length) {
    //log_info("12 byte hex: %d", ((length[0]&0xff)<<24));
    int64_t len = 0;
    len |= ((length[0] & 0xff) << 24);
    len |= ((length[1] & 0xff) << 16);
    len |= ((length[2] & 0xff) << 8);
    len |= (length[3] & 0xff);
    //log_info("length: %d", len);
    return len;
}

void int_to_four_char(int32_t integer, char *bytes) {
    bytes[0] = (char) ((integer >> 24) & 0xFF);
    bytes[1] = (char) ((integer >> 16) & 0xFF);
    bytes[2] = (char) ((integer >> 8) & 0xFF);
    bytes[3] = (char) ((integer >> 0) & 0xFF);
}

void long_to_8bytes(int64_t integer, void *bytes) {
    unsigned char *b = (unsigned char *) bytes;
    unsigned char *p = (unsigned char *) &integer;
    memcpy(b, p, sizeof(unsigned char) * 8);

}

int64_t bytes8_to_long(void *bytes) {
    return *((int64_t *) bytes);
}

float unpackFloat(const void *buf) {
    return *((float *) buf);
}

int packFloat(void *buf, float x) {
    unsigned char *b = (unsigned char *) buf;
    unsigned char *p = (unsigned char *) &x;
#if defined (_M_IX86) || (defined (CPU_FAMILY) && (CPU_FAMILY == I80X86))
    b[0] = p[3];
    b[1] = p[2];
    b[2] = p[1];
    b[3] = p[0];
#else
    b[0] = p[0];
    b[1] = p[1];
    b[2] = p[2];
    b[3] = p[3];
#endif
    return 4;
}

void int_to_string(char *str, int num) {
    if (num < 10) {
        str[0] = num + '0';
    } else {
        int new_val = num / 10;
        str[0] = new_val + '0';
        str[1] = num - new_val * 10 + '0';
    }
}

void get_cpu_info() {
    uv_cpu_info_t *uv_cpus_info;
    int cpu_count;
    uv_cpu_info(&uv_cpus_info, &cpu_count);
    log_info("System has %d cpus", cpu_count);
    log_info("###################################CPU_INFO###################################");
    for (int i = 0; i < 1; i++) {
        uv_cpu_info_t cpu_info = uv_cpus_info[i];
        log_info("CPU model: %s, speed: %dMHZ", cpu_info.model, cpu_info.speed);
    }
    log_info("#############################################################################");
    uv_free_cpu_info(uv_cpus_info, cpu_count);
}

/*
  Converts integer to its string representation in decimal notation.

  SYNOPSIS
    int10_to_str()
      val     - value to convert
      dst     - points to buffer where string representation should be stored
      radix   - flag that shows whenever val should be taken as signed or not
  DESCRIPTION
    This is version of int2str() function which is optimized for normal case
    of radix 10/-10. It takes only sign of radix parameter into account and
    not its absolute value.
  RETURN VALUE
    Pointer to ending NUL character.
*/
char *int10_to_str(long int val, char *dst, int radix) {
    char buffer[65];
    char *p;
    long int new_val;
    unsigned long int uval = (unsigned long int) val;

    if (radix < 0)                /* -10 */
    {
        if (val < 0) {
            *dst++ = '-';
            /* Avoid integer overflow in (-val) for LLONG_MIN (BUG#31799). */
            uval = (unsigned long int) 0 - uval;
        }
    }

    p = &buffer[sizeof(buffer) - 1];
    *p = '\0';
    new_val = (long) (uval / 10);
    *--p = '0' + (char) (uval - (unsigned long) new_val * 10);
    val = new_val;

    while (val != 0) {
        new_val = val / 10;
        *--p = '0' + (char) (val - new_val * 10);
        val = new_val;
    }
    while ((*dst++ = *p++) != 0);
    return dst - 1;
}

char *resolveToIp4Str(char *hostname) {
    char *res = malloc(sizeof(char) * 16);//for ipv4 only e.g. '255.255.255.255\0', max length = 16
    memset(res, 0, sizeof(char) * 16);
    struct hostent *ghbn = gethostbyname(hostname);
    if (!ghbn) {
        free(res);
        log_error("cannot resolve %s", hostname);
        return NULL;
    }
    char *address = inet_ntoa(*(struct in_addr *) ghbn->h_addr);
    strcpy(res, address);
    return res;
}

int manual_jvm_gc(int java_pid) {
    static int nspid = -1;
    if (nspid < 0) {
        nspid = init_jvm_comm_socket(java_pid);
    }
    int argc = 2;
    char *argv[2] = {"jcmd", "GC.run"};
    return jvm_run_cmd(nspid, argc, argv);
}

void save_to_binary_file(char *filename, char *buf, int size) {
    FILE *ptr = fopen(filename, "wb");
    fwrite(buf, sizeof(char), size, ptr);
    fclose(ptr);
}


void increase_long_by_one(unsigned char bytes[8]){
    for(int i = 0; i < 8; i++){
        bytes[i] += 1;
        if(bytes[i] != 0)
            break;
    }
}