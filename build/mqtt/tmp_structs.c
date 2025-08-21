# 0 "temp_no_pp.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "temp_no_pp.c"
# 1 "../../utils/fake_headers/mongoose.h" 1
# 332 "../../utils/fake_headers/mongoose.h"
# 1 "../../utils/fake_headers/arpa/inet.h" 1
# 1 "../../utils/fake_headers/_fake_defines.h" 1
# 2 "../../utils/fake_headers/arpa/inet.h" 2
# 1 "../../utils/fake_headers/_fake_typedefs.h" 1



typedef int size_t;
typedef int __builtin_va_list;
typedef int __gnuc_va_list;
typedef int va_list;
typedef int __int8_t;
typedef int __uint8_t;
typedef int __int16_t;
typedef int __uint16_t;
typedef int __int_least16_t;
typedef int __uint_least16_t;
typedef int __int32_t;
typedef int __uint32_t;
typedef int __int64_t;
typedef int __uint64_t;
typedef int __int_least32_t;
typedef int __uint_least32_t;
typedef int __s8;
typedef int __u8;
typedef int __s16;
typedef int __u16;
typedef int __s32;
typedef int __u32;
typedef int __s64;
typedef int __u64;
typedef int _LOCK_T;
typedef int _LOCK_RECURSIVE_T;
typedef int _off_t;
typedef int __dev_t;
typedef int __uid_t;
typedef int __gid_t;
typedef int _off64_t;
typedef int _fpos_t;
typedef int _ssize_t;
typedef int wint_t;
typedef int _mbstate_t;
typedef int _flock_t;
typedef int _iconv_t;
typedef int __ULong;
typedef int __FILE;
typedef int ptrdiff_t;
typedef int wchar_t;
typedef int char16_t;
typedef int char32_t;
typedef int __off_t;
typedef int __pid_t;
typedef int __loff_t;
typedef int u_char;
typedef int u_short;
typedef int u_int;
typedef int u_long;
typedef int ushort;
typedef int uint;
typedef int clock_t;
typedef int time_t;
typedef int daddr_t;
typedef int caddr_t;
typedef int ino_t;
typedef int off_t;
typedef int dev_t;
typedef int uid_t;
typedef int gid_t;
typedef int pid_t;
typedef int key_t;
typedef int ssize_t;
typedef int mode_t;
typedef int nlink_t;
typedef int fd_mask;
typedef int _types_fd_set;
typedef int clockid_t;
typedef int timer_t;
typedef int useconds_t;
typedef int suseconds_t;
typedef int FILE;
typedef int fpos_t;
typedef int cookie_read_function_t;
typedef int cookie_write_function_t;
typedef int cookie_seek_function_t;
typedef int cookie_close_function_t;
typedef int cookie_io_functions_t;
typedef int div_t;
typedef int ldiv_t;
typedef int lldiv_t;
typedef int sigset_t;
typedef int __sigset_t;
typedef int _sig_func_ptr;
typedef int sig_atomic_t;
typedef int __tzrule_type;
typedef int __tzinfo_type;
typedef int mbstate_t;
typedef int sem_t;
typedef int pthread_t;
typedef int pthread_attr_t;
typedef int pthread_mutex_t;
typedef int pthread_mutexattr_t;
typedef int pthread_cond_t;
typedef int pthread_condattr_t;
typedef int pthread_key_t;
typedef int pthread_once_t;
typedef int pthread_rwlock_t;
typedef int pthread_rwlockattr_t;
typedef int pthread_spinlock_t;
typedef int pthread_barrier_t;
typedef int pthread_barrierattr_t;
typedef int jmp_buf;
typedef int rlim_t;
typedef int sa_family_t;
typedef int sigjmp_buf;
typedef int stack_t;
typedef int siginfo_t;
typedef int z_stream;


typedef int int8_t;
typedef int uint8_t;
typedef int int16_t;
typedef int uint16_t;
typedef int int32_t;
typedef int uint32_t;
typedef int int64_t;
typedef int uint64_t;


typedef int int_least8_t;
typedef int uint_least8_t;
typedef int int_least16_t;
typedef int uint_least16_t;
typedef int int_least32_t;
typedef int uint_least32_t;
typedef int int_least64_t;
typedef int uint_least64_t;


typedef int int_fast8_t;
typedef int uint_fast8_t;
typedef int int_fast16_t;
typedef int uint_fast16_t;
typedef int int_fast32_t;
typedef int uint_fast32_t;
typedef int int_fast64_t;
typedef int uint_fast64_t;


typedef int intptr_t;
typedef int uintptr_t;


typedef int intmax_t;
typedef int uintmax_t;


typedef _Bool bool;



typedef int __int128_t;
typedef int __uint128_t;



typedef void* MirEGLNativeWindowType;
typedef void* MirEGLNativeDisplayType;
typedef struct MirConnection MirConnection;
typedef struct MirSurface MirSurface;
typedef struct MirSurfaceSpec MirSurfaceSpec;
typedef struct MirScreencast MirScreencast;
typedef struct MirPromptSession MirPromptSession;
typedef struct MirBufferStream MirBufferStream;
typedef struct MirPersistentId MirPersistentId;
typedef struct MirBlob MirBlob;
typedef struct MirDisplayConfig MirDisplayConfig;


typedef struct xcb_connection_t xcb_connection_t;
typedef uint32_t xcb_window_t;
typedef uint32_t xcb_visualid_t;


typedef void* DIR;


typedef __uint32_t __socklen_t;
typedef __socklen_t socklen_t;


typedef _Atomic(_Bool) atomic_bool;
typedef _Atomic(char) atomic_char;
typedef _Atomic(signed char) atomic_schar;
typedef _Atomic(unsigned char) atomic_uchar;
typedef _Atomic(short) atomic_short;
typedef _Atomic(unsigned short) atomic_ushort;
typedef _Atomic(int) atomic_int;
typedef _Atomic(unsigned int) atomic_uint;
typedef _Atomic(long) atomic_long;
typedef _Atomic(unsigned long) atomic_ulong;
typedef _Atomic(long long) atomic_llong;
typedef _Atomic(unsigned long long) atomic_ullong;
typedef _Atomic(uint_least16_t) atomic_char16_t;
typedef _Atomic(uint_least32_t) atomic_char32_t;
typedef _Atomic(wchar_t) atomic_wchar_t;
typedef _Atomic(int_least8_t) atomic_int_least8_t;
typedef _Atomic(uint_least8_t) atomic_uint_least8_t;
typedef _Atomic(int_least16_t) atomic_int_least16_t;
typedef _Atomic(uint_least16_t) atomic_uint_least16_t;
typedef _Atomic(int_least32_t) atomic_int_least32_t;
typedef _Atomic(uint_least32_t) atomic_uint_least32_t;
typedef _Atomic(int_least64_t) atomic_int_least64_t;
typedef _Atomic(uint_least64_t) atomic_uint_least64_t;
typedef _Atomic(int_fast8_t) atomic_int_fast8_t;
typedef _Atomic(uint_fast8_t) atomic_uint_fast8_t;
typedef _Atomic(int_fast16_t) atomic_int_fast16_t;
typedef _Atomic(uint_fast16_t) atomic_uint_fast16_t;
typedef _Atomic(int_fast32_t) atomic_int_fast32_t;
typedef _Atomic(uint_fast32_t) atomic_uint_fast32_t;
typedef _Atomic(int_fast64_t) atomic_int_fast64_t;
typedef _Atomic(uint_fast64_t) atomic_uint_fast64_t;
typedef _Atomic(intptr_t) atomic_intptr_t;
typedef _Atomic(uintptr_t) atomic_uintptr_t;
typedef _Atomic(size_t) atomic_size_t;
typedef _Atomic(ptrdiff_t) atomic_ptrdiff_t;
typedef _Atomic(intmax_t) atomic_intmax_t;
typedef _Atomic(uintmax_t) atomic_uintmax_t;
typedef struct atomic_flag { atomic_bool _Value; } atomic_flag;
typedef enum memory_order {
  memory_order_relaxed,
  memory_order_consume,
  memory_order_acquire,
  memory_order_release,
  memory_order_acq_rel,
  memory_order_seq_cst
} memory_order;
# 3 "../../utils/fake_headers/arpa/inet.h" 2
# 333 "../../utils/fake_headers/mongoose.h" 2
# 1 "../../utils/fake_headers/ctype.h" 1
# 1 "../../utils/fake_headers/_fake_defines.h" 1
# 2 "../../utils/fake_headers/ctype.h" 2
# 1 "../../utils/fake_headers/_fake_typedefs.h" 1
# 3 "../../utils/fake_headers/ctype.h" 2
# 334 "../../utils/fake_headers/mongoose.h" 2
# 1 "../../utils/fake_headers/dirent.h" 1
# 335 "../../utils/fake_headers/mongoose.h" 2
# 1 "../../utils/fake_headers/errno.h" 1
# 336 "../../utils/fake_headers/mongoose.h" 2
# 1 "../../utils/fake_headers/fcntl.h" 1
# 337 "../../utils/fake_headers/mongoose.h" 2
# 1 "../../utils/fake_headers/inttypes.h" 1
# 338 "../../utils/fake_headers/mongoose.h" 2
# 1 "../../utils/fake_headers/limits.h" 1
# 339 "../../utils/fake_headers/mongoose.h" 2
# 1 "../../utils/fake_headers/netdb.h" 1
# 340 "../../utils/fake_headers/mongoose.h" 2
# 1 "../../utils/fake_headers/netinet/in.h" 1
# 341 "../../utils/fake_headers/mongoose.h" 2
# 1 "../../utils/fake_headers/netinet/tcp.h" 1
# 342 "../../utils/fake_headers/mongoose.h" 2
# 1 "../../utils/fake_headers/signal.h" 1
# 343 "../../utils/fake_headers/mongoose.h" 2
# 1 "../../utils/fake_headers/stdarg.h" 1
# 344 "../../utils/fake_headers/mongoose.h" 2
# 1 "../../utils/fake_headers/stdbool.h" 1
# 345 "../../utils/fake_headers/mongoose.h" 2
# 1 "../../utils/fake_headers/stddef.h" 1
# 346 "../../utils/fake_headers/mongoose.h" 2
# 1 "../../utils/fake_headers/stdint.h" 1
# 347 "../../utils/fake_headers/mongoose.h" 2
# 1 "../../utils/fake_headers/stdio.h" 1
# 348 "../../utils/fake_headers/mongoose.h" 2
# 1 "../../utils/fake_headers/stdlib.h" 1
# 349 "../../utils/fake_headers/mongoose.h" 2
# 1 "../../utils/fake_headers/string.h" 1
# 350 "../../utils/fake_headers/mongoose.h" 2


# 1 "/usr/include/x86_64-linux-gnu/sys/epoll.h" 1 3 4
# 21 "/usr/include/x86_64-linux-gnu/sys/epoll.h" 3 4
# 1 "../../utils/fake_headers/stdint.h" 1 3 4
# 22 "/usr/include/x86_64-linux-gnu/sys/epoll.h" 2 3 4
# 1 "../../utils/fake_headers/sys/types.h" 1 3 4
# 23 "/usr/include/x86_64-linux-gnu/sys/epoll.h" 2 3 4

# 1 "/usr/include/x86_64-linux-gnu/bits/types/sigset_t.h" 1 3 4



# 1 "/usr/include/x86_64-linux-gnu/bits/types/__sigset_t.h" 1 3 4





# 5 "/usr/include/x86_64-linux-gnu/bits/types/__sigset_t.h" 3 4
typedef struct
{
  unsigned long int __val[(1024 / (8 * sizeof (unsigned long int)))];
} __sigset_t;
# 5 "/usr/include/x86_64-linux-gnu/bits/types/sigset_t.h" 2 3 4


typedef __sigset_t sigset_t;
# 25 "/usr/include/x86_64-linux-gnu/sys/epoll.h" 2 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/types/struct_timespec.h" 1 3 4




# 1 "/usr/include/x86_64-linux-gnu/bits/types.h" 1 3 4
# 26 "/usr/include/x86_64-linux-gnu/bits/types.h" 3 4
# 1 "../../utils/fake_headers/features.h" 1 3 4
# 27 "/usr/include/x86_64-linux-gnu/bits/types.h" 2 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 28 "/usr/include/x86_64-linux-gnu/bits/types.h" 2 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/timesize.h" 1 3 4
# 19 "/usr/include/x86_64-linux-gnu/bits/timesize.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 20 "/usr/include/x86_64-linux-gnu/bits/timesize.h" 2 3 4
# 29 "/usr/include/x86_64-linux-gnu/bits/types.h" 2 3 4


typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;


typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;

typedef signed long int __int64_t;
typedef unsigned long int __uint64_t;






typedef __int8_t __int_least8_t;
typedef __uint8_t __uint_least8_t;
typedef __int16_t __int_least16_t;
typedef __uint16_t __uint_least16_t;
typedef __int32_t __int_least32_t;
typedef __uint32_t __uint_least32_t;
typedef __int64_t __int_least64_t;
typedef __uint64_t __uint_least64_t;



typedef long int __quad_t;
typedef unsigned long int __u_quad_t;







typedef long int __intmax_t;
typedef unsigned long int __uintmax_t;
# 141 "/usr/include/x86_64-linux-gnu/bits/types.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/typesizes.h" 1 3 4
# 142 "/usr/include/x86_64-linux-gnu/bits/types.h" 2 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/time64.h" 1 3 4
# 143 "/usr/include/x86_64-linux-gnu/bits/types.h" 2 3 4


typedef unsigned long int __dev_t;
typedef unsigned int __uid_t;
typedef unsigned int __gid_t;
typedef unsigned long int __ino_t;
typedef unsigned long int __ino64_t;
typedef unsigned int __mode_t;
typedef unsigned long int __nlink_t;
typedef long int __off_t;
typedef long int __off64_t;
typedef int __pid_t;
typedef struct { int __val[2]; } __fsid_t;
typedef long int __clock_t;
typedef unsigned long int __rlim_t;
typedef unsigned long int __rlim64_t;
typedef unsigned int __id_t;
typedef long int __time_t;
typedef unsigned int __useconds_t;
typedef long int __suseconds_t;
typedef long int __suseconds64_t;

typedef int __daddr_t;
typedef int __key_t;


typedef int __clockid_t;


typedef void * __timer_t;


typedef long int __blksize_t;




typedef long int __blkcnt_t;
typedef long int __blkcnt64_t;


typedef unsigned long int __fsblkcnt_t;
typedef unsigned long int __fsblkcnt64_t;


typedef unsigned long int __fsfilcnt_t;
typedef unsigned long int __fsfilcnt64_t;


typedef long int __fsword_t;

typedef long int __ssize_t;


typedef long int __syscall_slong_t;

typedef unsigned long int __syscall_ulong_t;



typedef __off64_t __loff_t;
typedef char *__caddr_t;


typedef long int __intptr_t;


typedef unsigned int __socklen_t;




typedef int __sig_atomic_t;
# 6 "/usr/include/x86_64-linux-gnu/bits/types/struct_timespec.h" 2 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/endian.h" 1 3 4
# 35 "/usr/include/x86_64-linux-gnu/bits/endian.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/endianness.h" 1 3 4
# 36 "/usr/include/x86_64-linux-gnu/bits/endian.h" 2 3 4
# 7 "/usr/include/x86_64-linux-gnu/bits/types/struct_timespec.h" 2 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/types/time_t.h" 1 3 4
# 10 "/usr/include/x86_64-linux-gnu/bits/types/time_t.h" 3 4
typedef __time_t time_t;
# 8 "/usr/include/x86_64-linux-gnu/bits/types/struct_timespec.h" 2 3 4



struct timespec
{



  __time_t tv_sec;




  __syscall_slong_t tv_nsec;
# 31 "/usr/include/x86_64-linux-gnu/bits/types/struct_timespec.h" 3 4
};
# 26 "/usr/include/x86_64-linux-gnu/sys/epoll.h" 2 3 4


# 1 "/usr/include/x86_64-linux-gnu/bits/epoll.h" 1 3 4
# 23 "/usr/include/x86_64-linux-gnu/bits/epoll.h" 3 4
enum
  {
    EPOLL_CLOEXEC = 02000000

  };
# 29 "/usr/include/x86_64-linux-gnu/sys/epoll.h" 2 3 4






enum EPOLL_EVENTS
  {
    EPOLLIN = 0x001,

    EPOLLPRI = 0x002,

    EPOLLOUT = 0x004,

    EPOLLRDNORM = 0x040,

    EPOLLRDBAND = 0x080,

    EPOLLWRNORM = 0x100,

    EPOLLWRBAND = 0x200,

    EPOLLMSG = 0x400,

    EPOLLERR = 0x008,

    EPOLLHUP = 0x010,

    EPOLLRDHUP = 0x2000,

    EPOLLEXCLUSIVE = 1u << 28,

    EPOLLWAKEUP = 1u << 29,

    EPOLLONESHOT = 1u << 30,

    EPOLLET = 1u << 31

  };
# 76 "/usr/include/x86_64-linux-gnu/sys/epoll.h" 3 4
typedef union epoll_data
{
  void *ptr;
  int fd;
  uint32_t u32;
  uint64_t u64;
} epoll_data_t;

struct epoll_event
{
  uint32_t events;
  epoll_data_t data;
} ;








extern int epoll_create (int __size) ;



extern int epoll_create1 (int __flags) ;
# 110 "/usr/include/x86_64-linux-gnu/sys/epoll.h" 3 4
extern int epoll_ctl (int __epfd, int __op, int __fd,
        struct epoll_event *__event) ;
# 124 "/usr/include/x86_64-linux-gnu/sys/epoll.h" 3 4
extern int epoll_wait (int __epfd, struct epoll_event *__events,
         int __maxevents, int __timeout)
 __attr_access ((__write_only__, 2, 3));







extern int epoll_pwait (int __epfd, struct epoll_event *__events,
   int __maxevents, int __timeout,
   const __sigset_t *__ss)
 __attr_access ((__write_only__, 2, 3));






extern int epoll_pwait2 (int __epfd, struct epoll_event *__events,
    int __maxevents, const struct timespec *__timeout,
    const __sigset_t *__ss)
 __attr_access ((__write_only__, 2, 3));
# 161 "/usr/include/x86_64-linux-gnu/sys/epoll.h" 3 4

# 353 "../../utils/fake_headers/mongoose.h" 2






# 1 "../../utils/fake_headers/sys/socket.h" 1
# 360 "../../utils/fake_headers/mongoose.h" 2
# 1 "../../utils/fake_headers/sys/stat.h" 1
# 361 "../../utils/fake_headers/mongoose.h" 2
# 1 "../../utils/fake_headers/sys/time.h" 1
# 362 "../../utils/fake_headers/mongoose.h" 2
# 1 "../../utils/fake_headers/sys/types.h" 1
# 363 "../../utils/fake_headers/mongoose.h" 2
# 1 "../../utils/fake_headers/time.h" 1
# 364 "../../utils/fake_headers/mongoose.h" 2
# 1 "../../utils/fake_headers/unistd.h" 1
# 365 "../../utils/fake_headers/mongoose.h" 2
# 799 "../../utils/fake_headers/mongoose.h"

# 799 "../../utils/fake_headers/mongoose.h"
struct mg_str {
  const char *ptr;
  size_t len;
};
# 813 "../../utils/fake_headers/mongoose.h"
struct mg_str mg_str_s(const char *s);
struct mg_str mg_str_n(const char *s, size_t n);
int mg_lower(const char *s);
int mg_ncasecmp(const char *s1, const char *s2, size_t len);
int mg_casecmp(const char *s1, const char *s2);
int mg_vcmp(const struct mg_str *s1, const char *s2);
int mg_vcasecmp(const struct mg_str *str1, const char *str2);
int mg_strcmp(const struct mg_str str1, const struct mg_str str2);
struct mg_str mg_strstrip(struct mg_str s);
struct mg_str mg_strdup(const struct mg_str s);
const char *mg_strstr(const struct mg_str haystack, const struct mg_str needle);
bool mg_match(struct mg_str str, struct mg_str pattern, struct mg_str *caps);
bool mg_globmatch(const char *pattern, size_t plen, const char *s, size_t n);
bool mg_commalist(struct mg_str *s, struct mg_str *k, struct mg_str *v);
bool mg_split(struct mg_str *s, struct mg_str *k, struct mg_str *v, char delim);
char *mg_hex(const void *buf, size_t len, char *dst);
void mg_unhex(const char *buf, size_t len, unsigned char *to);
unsigned long mg_unhexn(const char *s, size_t len);
int mg_check_ip_acl(struct mg_str acl, uint32_t remote_ip);
int64_t mg_to64(struct mg_str str);
uint64_t mg_tou64(struct mg_str str);
char *mg_remove_double_dots(char *s);






struct mg_queue {
  char *buf;
  size_t size;
  volatile size_t tail;
  volatile size_t head;
};

void mg_queue_init(struct mg_queue *, char *, size_t);
size_t mg_queue_book(struct mg_queue *, char **buf, size_t);
void mg_queue_add(struct mg_queue *, size_t);
size_t mg_queue_next(struct mg_queue *, char **);
void mg_queue_del(struct mg_queue *, size_t);




typedef void (*mg_pfn_t)(char, void *);
typedef size_t (*mg_pm_t)(mg_pfn_t, void *, va_list *);

size_t mg_vxprintf(void (*)(char, void *), void *, const char *fmt, va_list *);
size_t mg_xprintf(void (*fn)(char, void *), void *, const char *fmt, ...);







size_t mg_vsnprintf(char *buf, size_t len, const char *fmt, va_list *ap);
size_t mg_snprintf(char *, size_t, const char *fmt, ...);
char *mg_vmprintf(const char *fmt, va_list *ap);
char *mg_mprintf(const char *fmt, ...);
size_t mg_queue_vprintf(struct mg_queue *, const char *fmt, va_list *);
size_t mg_queue_printf(struct mg_queue *, const char *fmt, ...);


size_t mg_print_base64(void (*out)(char, void *), void *arg, va_list *ap);
size_t mg_print_esc(void (*out)(char, void *), void *arg, va_list *ap);
size_t mg_print_hex(void (*out)(char, void *), void *arg, va_list *ap);
size_t mg_print_ip(void (*out)(char, void *), void *arg, va_list *ap);
size_t mg_print_ip_port(void (*out)(char, void *), void *arg, va_list *ap);
size_t mg_print_ip4(void (*out)(char, void *), void *arg, va_list *ap);
size_t mg_print_ip6(void (*out)(char, void *), void *arg, va_list *ap);
size_t mg_print_mac(void (*out)(char, void *), void *arg, va_list *ap);


void mg_pfn_iobuf(char ch, void *param);
void mg_pfn_stdout(char c, void *param);
# 898 "../../utils/fake_headers/mongoose.h"
enum { MG_LL_NONE, MG_LL_ERROR, MG_LL_INFO, MG_LL_DEBUG, MG_LL_VERBOSE };
void mg_log(const char *fmt, ...);
bool mg_log_prefix(int ll, const char *file, int line, const char *fname);
void mg_log_set(int log_level);
void mg_hexdump(const void *buf, size_t len);
void mg_log_set_fn(mg_pfn_t fn, void *param);
# 925 "../../utils/fake_headers/mongoose.h"
struct mg_timer {
  unsigned long id;
  uint64_t period_ms;
  uint64_t expire;
  unsigned flags;



  void (*fn)(void *);
  void *arg;
  struct mg_timer *next;
};

void mg_timer_init(struct mg_timer **head, struct mg_timer *timer,
                   uint64_t milliseconds, unsigned flags, void (*fn)(void *),
                   void *arg);
void mg_timer_free(struct mg_timer **head, struct mg_timer *);
void mg_timer_poll(struct mg_timer **head, uint64_t new_ms);
bool mg_timer_expired(uint64_t *expiration, uint64_t period, uint64_t now);





enum { MG_FS_READ = 1, MG_FS_WRITE = 2, MG_FS_DIR = 4 };
# 958 "../../utils/fake_headers/mongoose.h"
struct mg_fs {
  int (*st)(const char *path, size_t *size, time_t *mtime);
  void (*ls)(const char *path, void (*fn)(const char *, void *), void *);
  void *(*op)(const char *path, int flags);
  void (*cl)(void *fd);
  size_t (*rd)(void *fd, void *buf, size_t len);
  size_t (*wr)(void *fd, const void *buf, size_t len);
  size_t (*sk)(void *fd, size_t offset);
  bool (*mv)(const char *from, const char *to);
  bool (*rm)(const char *path);
  bool (*mkd)(const char *path);
};

extern struct mg_fs mg_fs_posix;
extern struct mg_fs mg_fs_packed;
extern struct mg_fs mg_fs_fat;


struct mg_fd {
  void *fd;
  struct mg_fs *fs;
};

struct mg_fd *mg_fs_open(struct mg_fs *fs, const char *path, int flags);
void mg_fs_close(struct mg_fd *fd);
char *mg_file_read(struct mg_fs *fs, const char *path, size_t *size);
bool mg_file_write(struct mg_fs *fs, const char *path, const void *, size_t);
bool mg_file_printf(struct mg_fs *fs, const char *path, const char *fmt, ...);
# 999 "../../utils/fake_headers/mongoose.h"
void mg_random(void *buf, size_t len);
char *mg_random_str(char *buf, size_t len);
uint16_t mg_ntohs(uint16_t net);
uint32_t mg_ntohl(uint32_t net);
uint32_t mg_crc32(uint32_t crc, const char *buf, size_t len);
uint64_t mg_millis(void);
# 1041 "../../utils/fake_headers/mongoose.h"
unsigned short mg_url_port(const char *url);
int mg_url_is_ssl(const char *url);
struct mg_str mg_url_host(const char *url);
struct mg_str mg_url_user(const char *url);
struct mg_str mg_url_pass(const char *url);
const char *mg_url_uri(const char *url);




struct mg_iobuf {
  unsigned char *buf;
  size_t size;
  size_t len;
  size_t align;
};

int mg_iobuf_init(struct mg_iobuf *, size_t, size_t);
int mg_iobuf_resize(struct mg_iobuf *, size_t);
void mg_iobuf_free(struct mg_iobuf *);
size_t mg_iobuf_add(struct mg_iobuf *, size_t, const void *, size_t);
size_t mg_iobuf_del(struct mg_iobuf *, size_t ofs, size_t len);

int mg_base64_update(unsigned char p, char *to, int len);
int mg_base64_final(char *to, int len);
int mg_base64_encode(const unsigned char *p, int n, char *to);
int mg_base64_decode(const char *src, int n, char *dst);




typedef struct {
  uint32_t buf[4];
  uint32_t bits[2];
  unsigned char in[64];
} mg_md5_ctx;

void mg_md5_init(mg_md5_ctx *c);
void mg_md5_update(mg_md5_ctx *c, const unsigned char *data, size_t len);
void mg_md5_final(mg_md5_ctx *c, unsigned char[16]);




typedef struct {
  uint32_t state[5];
  uint32_t count[2];
  unsigned char buffer[64];
} mg_sha1_ctx;

void mg_sha1_init(mg_sha1_ctx *);
void mg_sha1_update(mg_sha1_ctx *, const unsigned char *data, size_t len);
void mg_sha1_final(unsigned char digest[20], mg_sha1_ctx *);


struct mg_connection;
typedef void (*mg_event_handler_t)(struct mg_connection *, int ev,
                                   void *ev_data, void *fn_data);
void mg_call(struct mg_connection *c, int ev, void *ev_data);
void mg_error(struct mg_connection *c, const char *fmt, ...);

enum {
  MG_EV_ERROR,
  MG_EV_OPEN,
  MG_EV_POLL,
  MG_EV_RESOLVE,
  MG_EV_CONNECT,
  MG_EV_ACCEPT,
  MG_EV_TLS_HS,
  MG_EV_READ,
  MG_EV_WRITE,
  MG_EV_CLOSE,
  MG_EV_HTTP_MSG,
  MG_EV_HTTP_CHUNK,
  MG_EV_WS_OPEN,
  MG_EV_WS_MSG,
  MG_EV_WS_CTL,
  MG_EV_MQTT_CMD,
  MG_EV_MQTT_MSG,
  MG_EV_MQTT_OPEN,
  MG_EV_SNTP_TIME,
  MG_EV_USER
};
# 1133 "../../utils/fake_headers/mongoose.h"
struct mg_dns {
  const char *url;
  struct mg_connection *c;
};

struct mg_addr {
  uint16_t port;
  uint32_t ip;
  uint8_t ip6[16];
  bool is_ip6;
};

struct mg_mgr {
  struct mg_connection *conns;
  struct mg_dns dns4;
  struct mg_dns dns6;
  int dnstimeout;
  bool use_dns6;
  unsigned long nextid;
  unsigned long timerid;
  void *userdata;
  uint16_t mqtt_id;
  void *active_dns_requests;
  struct mg_timer *timers;
  int epoll_fd;
  void *priv;
  size_t extraconnsize;



};

struct mg_connection {
  struct mg_connection *next;
  struct mg_mgr *mgr;
  struct mg_addr loc;
  struct mg_addr rem;
  void *fd;
  unsigned long id;
  struct mg_iobuf recv;
  struct mg_iobuf send;
  mg_event_handler_t fn;
  void *fn_data;
  mg_event_handler_t pfn;
  void *pfn_data;
  char data[32];
  void *tls;
  unsigned is_listening : 1;
  unsigned is_client : 1;
  unsigned is_accepted : 1;
  unsigned is_resolving : 1;
  unsigned is_arplooking : 1;
  unsigned is_connecting : 1;
  unsigned is_tls : 1;
  unsigned is_tls_hs : 1;
  unsigned is_udp : 1;
  unsigned is_websocket : 1;
  unsigned is_mqtt5 : 1;
  unsigned is_hexdumping : 1;
  unsigned is_draining : 1;
  unsigned is_closing : 1;
  unsigned is_full : 1;
  unsigned is_resp : 1;
  unsigned is_readable : 1;
  unsigned is_writable : 1;
};

void mg_mgr_poll(struct mg_mgr *, int ms);
void mg_mgr_init(struct mg_mgr *);
void mg_mgr_free(struct mg_mgr *);

struct mg_connection *mg_listen(struct mg_mgr *, const char *url,
                                mg_event_handler_t fn, void *fn_data);
struct mg_connection *mg_connect(struct mg_mgr *, const char *url,
                                 mg_event_handler_t fn, void *fn_data);
struct mg_connection *mg_wrapfd(struct mg_mgr *mgr, int fd,
                                mg_event_handler_t fn, void *fn_data);
void mg_connect_resolved(struct mg_connection *);
bool mg_send(struct mg_connection *, const void *, size_t);
size_t mg_printf(struct mg_connection *, const char *fmt, ...);
size_t mg_vprintf(struct mg_connection *, const char *fmt, va_list *ap);
bool mg_aton(struct mg_str str, struct mg_addr *addr);
int mg_mkpipe(struct mg_mgr *, mg_event_handler_t, void *, bool udp);


struct mg_connection *mg_alloc_conn(struct mg_mgr *);
void mg_close_conn(struct mg_connection *c);
bool mg_open_listener(struct mg_connection *c, const char *url);


struct mg_timer *mg_timer_add(struct mg_mgr *mgr, uint64_t milliseconds,
                              unsigned flags, void (*fn)(void *), void *arg);


enum { MG_IO_ERR = -1, MG_IO_WAIT = -2, MG_IO_RESET = -3 };
long mg_io_send(struct mg_connection *c, const void *buf, size_t len);
long mg_io_recv(struct mg_connection *c, void *buf, size_t len);
# 1238 "../../utils/fake_headers/mongoose.h"
struct mg_http_header {
  struct mg_str name;
  struct mg_str value;
};

struct mg_http_message {
  struct mg_str method, uri, query, proto;
  struct mg_http_header headers[30];
  struct mg_str body;
  struct mg_str head;
  struct mg_str chunk;
  struct mg_str message;
};


struct mg_http_serve_opts {
  const char *root_dir;
  const char *ssi_pattern;
  const char *extra_headers;
  const char *mime_types;
  const char *page404;
  struct mg_fs *fs;
};


struct mg_http_part {
  struct mg_str name;
  struct mg_str filename;
  struct mg_str body;
};

int mg_http_parse(const char *s, size_t len, struct mg_http_message *);
int mg_http_get_request_len(const unsigned char *buf, size_t buf_len);
void mg_http_printf_chunk(struct mg_connection *cnn, const char *fmt, ...);
void mg_http_write_chunk(struct mg_connection *c, const char *buf, size_t len);
void mg_http_delete_chunk(struct mg_connection *c, struct mg_http_message *hm);
struct mg_connection *mg_http_listen(struct mg_mgr *, const char *url,
                                     mg_event_handler_t fn, void *fn_data);
struct mg_connection *mg_http_connect(struct mg_mgr *, const char *url,
                                      mg_event_handler_t fn, void *fn_data);
void mg_http_serve_dir(struct mg_connection *, struct mg_http_message *hm,
                       const struct mg_http_serve_opts *);
void mg_http_serve_file(struct mg_connection *, struct mg_http_message *hm,
                        const char *path, const struct mg_http_serve_opts *);
void mg_http_reply(struct mg_connection *, int status_code, const char *headers,
                   const char *body_fmt, ...);
struct mg_str *mg_http_get_header(struct mg_http_message *, const char *name);
struct mg_str mg_http_var(struct mg_str buf, struct mg_str name);
int mg_http_get_var(const struct mg_str *, const char *name, char *, size_t);
int mg_url_decode(const char *s, size_t n, char *to, size_t to_len, int form);
size_t mg_url_encode(const char *s, size_t n, char *buf, size_t len);
void mg_http_creds(struct mg_http_message *, char *, size_t, char *, size_t);
bool mg_http_match_uri(const struct mg_http_message *, const char *glob);
long mg_http_upload(struct mg_connection *c, struct mg_http_message *hm,
                    struct mg_fs *fs, const char *path, size_t max_size);
void mg_http_bauth(struct mg_connection *, const char *user, const char *pass);
struct mg_str mg_http_get_header_var(struct mg_str s, struct mg_str v);
size_t mg_http_next_multipart(struct mg_str, size_t, struct mg_http_part *);
int mg_http_status(const struct mg_http_message *hm);
void mg_hello(const char *url);


void mg_http_serve_ssi(struct mg_connection *c, const char *root,
                       const char *fullpath);






struct mg_tls_opts {
  const char *ca;
  const char *crl;
  const char *cert;
  const char *certkey;
  const char *ciphers;
  struct mg_str srvname;
  struct mg_fs *fs;
};

void mg_tls_init(struct mg_connection *, const struct mg_tls_opts *);
void mg_tls_free(struct mg_connection *);
long mg_tls_send(struct mg_connection *, const void *buf, size_t len);
long mg_tls_recv(struct mg_connection *, void *buf, size_t len);
size_t mg_tls_pending(struct mg_connection *);
void mg_tls_handshake(struct mg_connection *);
# 1368 "../../utils/fake_headers/mongoose.h"
struct mg_ws_message {
  struct mg_str data;
  uint8_t flags;
};

struct mg_connection *mg_ws_connect(struct mg_mgr *, const char *url,
                                    mg_event_handler_t fn, void *fn_data,
                                    const char *fmt, ...);
void mg_ws_upgrade(struct mg_connection *, struct mg_http_message *,
                   const char *fmt, ...);
size_t mg_ws_send(struct mg_connection *, const void *buf, size_t len, int op);
size_t mg_ws_wrap(struct mg_connection *, size_t len, int op);
size_t mg_ws_printf(struct mg_connection *c, int op, const char *fmt, ...);
size_t mg_ws_vprintf(struct mg_connection *c, int op, const char *fmt,
                     va_list *);




struct mg_connection *mg_sntp_connect(struct mg_mgr *mgr, const char *url,
                                      mg_event_handler_t fn, void *fn_data);
void mg_sntp_request(struct mg_connection *c);
int64_t mg_sntp_parse(const unsigned char *buf, size_t len);
# 1440 "../../utils/fake_headers/mongoose.h"
enum {
  MQTT_PROP_TYPE_BYTE,
  MQTT_PROP_TYPE_STRING,
  MQTT_PROP_TYPE_STRING_PAIR,
  MQTT_PROP_TYPE_BINARY_DATA,
  MQTT_PROP_TYPE_VARIABLE_INT,
  MQTT_PROP_TYPE_INT,
  MQTT_PROP_TYPE_SHORT
};

enum { MQTT_OK, MQTT_INCOMPLETE, MQTT_MALFORMED };

struct mg_mqtt_prop {
  uint8_t id;
  uint32_t iv;
  struct mg_str key;
  struct mg_str val;
};

struct mg_mqtt_opts {
  struct mg_str user;
  struct mg_str pass;
  struct mg_str client_id;
  struct mg_str topic;
  struct mg_str message;
  uint8_t qos;
  uint8_t version;
  uint16_t keepalive;
  bool retain;
  bool clean;
  struct mg_mqtt_prop *props;
  size_t num_props;
  struct mg_mqtt_prop *will_props;
  size_t num_will_props;
};

struct mg_mqtt_message {
  struct mg_str topic;
  struct mg_str data;
  struct mg_str dgram;
  uint16_t id;
  uint8_t cmd;
  uint8_t qos;
  uint8_t ack;
  size_t props_start;
  size_t props_size;
};

struct mg_connection *mg_mqtt_connect(struct mg_mgr *, const char *url,
                                      const struct mg_mqtt_opts *opts,
                                      mg_event_handler_t fn, void *fn_data);
struct mg_connection *mg_mqtt_listen(struct mg_mgr *mgr, const char *url,
                                     mg_event_handler_t fn, void *fn_data);
void mg_mqtt_login(struct mg_connection *c, const struct mg_mqtt_opts *opts);
void mg_mqtt_pub(struct mg_connection *c, const struct mg_mqtt_opts *opts);
void mg_mqtt_sub(struct mg_connection *, const struct mg_mqtt_opts *opts);
int mg_mqtt_parse(const uint8_t *, size_t, uint8_t, struct mg_mqtt_message *);
void mg_mqtt_send_header(struct mg_connection *, uint8_t cmd, uint8_t flags,
                         uint32_t len);
void mg_mqtt_ping(struct mg_connection *);
void mg_mqtt_pong(struct mg_connection *);
void mg_mqtt_disconnect(struct mg_connection *, const struct mg_mqtt_opts *);
size_t mg_mqtt_next_prop(struct mg_mqtt_message *, struct mg_mqtt_prop *,
                         size_t ofs);
# 1513 "../../utils/fake_headers/mongoose.h"
struct mg_dns_message {
  uint16_t txnid;
  bool resolved;
  struct mg_addr addr;
  char name[256];
};

struct mg_dns_header {
  uint16_t txnid;
  uint16_t flags;
  uint16_t num_questions;
  uint16_t num_answers;
  uint16_t num_authority_prs;
  uint16_t num_other_prs;
};


struct mg_dns_rr {
  uint16_t nlen;
  uint16_t atype;
  uint16_t aclass;
  uint16_t alen;
};

void mg_resolve(struct mg_connection *, const char *url);
void mg_resolve_cancel(struct mg_connection *);
bool mg_dns_parse(const uint8_t *buf, size_t len, struct mg_dns_message *);
size_t mg_dns_parse_rr(const uint8_t *buf, size_t len, size_t ofs,
                       bool is_question, struct mg_dns_rr *);
# 1552 "../../utils/fake_headers/mongoose.h"
enum { MG_JSON_TOO_DEEP = -1, MG_JSON_INVALID = -2, MG_JSON_NOT_FOUND = -3 };
int mg_json_get(struct mg_str json, const char *path, int *toklen);

bool mg_json_get_num(struct mg_str json, const char *path, double *v);
bool mg_json_get_bool(struct mg_str json, const char *path, bool *v);
long mg_json_get_long(struct mg_str json, const char *path, long dflt);
char *mg_json_get_str(struct mg_str json, const char *path);
char *mg_json_get_hex(struct mg_str json, const char *path, int *len);
char *mg_json_get_b64(struct mg_str json, const char *path, int *len);





struct mg_rpc_req {
  struct mg_rpc **head;
  struct mg_rpc *rpc;
  mg_pfn_t pfn;
  void *pfn_data;
  void *req_data;
  struct mg_str frame;
};


struct mg_rpc {
  struct mg_rpc *next;
  struct mg_str method;
  void (*fn)(struct mg_rpc_req *);
  void *fn_data;
};

void mg_rpc_add(struct mg_rpc **head, struct mg_str method_pattern,
                void (*handler)(struct mg_rpc_req *), void *handler_data);
void mg_rpc_del(struct mg_rpc **head, void (*handler)(struct mg_rpc_req *));
void mg_rpc_process(struct mg_rpc_req *);


void mg_rpc_ok(struct mg_rpc_req *, const char *fmt, ...);
void mg_rpc_vok(struct mg_rpc_req *, const char *fmt, va_list *ap);
void mg_rpc_err(struct mg_rpc_req *, int code, const char *fmt, ...);
void mg_rpc_verr(struct mg_rpc_req *, int code, const char *fmt, va_list *);
void mg_rpc_list(struct mg_rpc_req *r);
# 1666 "../../utils/fake_headers/mongoose.h"
struct mg_tcpip_driver_imxrt1020_data {
# 1677 "../../utils/fake_headers/mongoose.h"
  int mdc_cr;
};


struct mg_tcpip_driver_stm32_data {
# 1693 "../../utils/fake_headers/mongoose.h"
  int mdc_cr;
};


struct mg_tcpip_driver_stm32h_data {
# 1709 "../../utils/fake_headers/mongoose.h"
  int mdc_cr;
};


struct mg_tcpip_driver_tm4c_data {
# 1723 "../../utils/fake_headers/mongoose.h"
  int mdc_cr;
};
# 2 "temp_no_pp.c" 2





static const char *s_listen_on = "mqtt://0.0.0.0:1883";


struct sub {
  struct sub *next;
  struct mg_connection *c;
  struct mg_str topic;
  uint8_t qos;
};
static struct sub *s_subs = 0;


static int s_signo;
static void signal_handler(int signo) {
  s_signo = signo;
}


static size_t mg_mqtt_next_topic(struct mg_mqtt_message *msg,
                                 struct mg_str *topic, uint8_t *qos,
                                 size_t pos) {
  unsigned char *buf = (unsigned char *) msg->dgram.ptr + pos;
  size_t new_pos;
  if (pos >= msg->dgram.len) return 0;

  topic->len = (size_t) (((unsigned) buf[0]) << 8 | buf[1]);
  topic->ptr = (char *) buf + 2;
  new_pos = pos + 2 + topic->len + (qos == 0 ? 0 : 1);
  if ((size_t) new_pos > msg->dgram.len) return 0;
  if (qos != 0) *qos = buf[2 + topic->len];
  return new_pos;
}

size_t mg_mqtt_next_sub(struct mg_mqtt_message *msg, struct mg_str *topic,
                        uint8_t *qos, size_t pos) {
  uint8_t tmp;
  return mg_mqtt_next_topic(msg, topic, qos == 0 ? &tmp : qos, pos);
}

size_t mg_mqtt_next_unsub(struct mg_mqtt_message *msg, struct mg_str *topic,
                          size_t pos) {
  return mg_mqtt_next_topic(msg, topic, 0, pos);
}


static void process_mqtt_message(struct mg_connection *c, struct mg_mqtt_message *mm) {
  switch (mm->cmd) {
    case 1: {

      do { if (mg_log_prefix((MG_LL_INFO), "temp_no_pp.c", 56, __func__)) mg_log ("CONNECT from %p", c->fd); } while (0);
      if (mm->dgram.len < 9) {
        mg_error(c, "Malformed MQTT frame");
      } else {

        uint8_t version = mm->dgram.ptr[8];
        do { if (mg_log_prefix((MG_LL_INFO), "temp_no_pp.c", 62, __func__)) mg_log ("MQTT version %d", version); } while (0);


        uint8_t response[] = {0, 0};
        mg_mqtt_send_header(c, 2, 0, sizeof(response));
        mg_send(c, response, sizeof(response));


        c->data[0] = version;
      }
      break;
    }

    case 8: {

      do { if (mg_log_prefix((MG_LL_INFO), "temp_no_pp.c", 77, __func__)) mg_log ("SUBSCRIBE from %p", c->fd); } while (0);
      size_t pos = 4;
      uint8_t qos, resp[256];
      struct mg_str topic;
      int num_topics = 0;

      while ((pos = mg_mqtt_next_sub(mm, &topic, &qos, pos)) > 0) {
        struct sub *sub = calloc(1, sizeof(*sub));
        sub->c = c;
        sub->topic = mg_strdup(topic);
        sub->qos = qos;
        do { (sub)->next = (*&s_subs); *(&s_subs) = (sub); } while (0);
        do { if (mg_log_prefix((MG_LL_INFO), "temp_no_pp.c", 89, __func__)) mg_log ("SUB %p [%.*s] QoS %d", c->fd, (int) sub->topic.len, sub->topic.ptr, qos); } while (0);


        for (size_t i = 0; i < sub->topic.len; i++) {
          if (sub->topic.ptr[i] == '+') ((char *) sub->topic.ptr)[i] = '*';
        }
        resp[num_topics++] = qos;
      }


      mg_mqtt_send_header(c, 9, 0, num_topics + 2);
      uint16_t id = mg_ntohs(mm->id);
      mg_send(c, &id, 2);
      mg_send(c, resp, num_topics);
      break;
    }

    case 3: {

      do { if (mg_log_prefix((MG_LL_INFO), "temp_no_pp.c", 108, __func__)) mg_log ("PUBLISH from %p [%.*s] -> [%.*s]", c->fd, (int) mm->data.len, mm->data.ptr, (int) mm->topic.len, mm->topic.ptr); } while (0)

                                                   ;


      if (mm->qos == 1) {
        uint16_t id = mg_ntohs(mm->id);
        mg_mqtt_send_header(c, 4, 0, 2);
        mg_send(c, &id, 2);
        do { if (mg_log_prefix((MG_LL_INFO), "temp_no_pp.c", 117, __func__)) mg_log ("Sent PUBACK for packet ID %d", mm->id); } while (0);
      }


      for (struct sub *sub = s_subs; sub != 0; sub = sub->next) {
        if (mg_match(mm->topic, sub->topic, 0)) {
          do { if (mg_log_prefix((MG_LL_INFO), "temp_no_pp.c", 123, __func__)) mg_log ("Forwarding to %p", sub->c->fd); } while (0);
          struct mg_mqtt_opts pub_opts;
          memset(&pub_opts, 0, sizeof(pub_opts));
          pub_opts.topic = mm->topic;
          pub_opts.message = mm->data;
          pub_opts.qos = sub->qos;
          pub_opts.retain = 0;
          mg_mqtt_pub(sub->c, &pub_opts);
        }
      }
      break;
    }

    case 12: {
      do { if (mg_log_prefix((MG_LL_INFO), "temp_no_pp.c", 137, __func__)) mg_log ("PINGREQ from %p", c->fd); } while (0);
      mg_mqtt_send_header(c, 13, 0, 0);
      do { if (mg_log_prefix((MG_LL_INFO), "temp_no_pp.c", 139, __func__)) mg_log ("Sent PINGRESP to %p", c->fd); } while (0);
      break;
    }

    case 14: {
      do { if (mg_log_prefix((MG_LL_INFO), "temp_no_pp.c", 144, __func__)) mg_log ("DISCONNECT from %p", c->fd); } while (0);
      c->is_closing = 1;
      break;
    }

    default:
      do { if (mg_log_prefix((MG_LL_INFO), "temp_no_pp.c", 150, __func__)) mg_log ("Unknown MQTT command %d from %p", mm->cmd, c->fd); } while (0);
      break;
  }
}


static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_READ) {

    size_t len = c->recv.len;

    if (len > 0) {
      do { if (mg_log_prefix((MG_LL_INFO), "temp_no_pp.c", 162, __func__)) mg_log ("Received %d bytes from %p", (int)len, c->fd); } while (0);


      uint8_t *heap_buf = (uint8_t *)malloc(len);
      if (!heap_buf) {
        do { if (mg_log_prefix((MG_LL_ERROR), "temp_no_pp.c", 167, __func__)) mg_log ("Failed to allocate memory"); } while (0);
        return;
      }


      memcpy(heap_buf, c->recv.buf, len);


      if (len <= 20) {
        printf("Data: ");
        for (size_t i = 0; i < len; i++) {
          printf("%02x ", heap_buf[i]);
        }
        printf("\n");
      }


      struct mg_mqtt_message mm;


      uint8_t version = 4;
      if (c->data[0] == 5) {
        version = 5;
      } else if (len == 8 && heap_buf[0] == 0x30 && heap_buf[1] == 0x06) {
        version = 5;
      }

      do { if (mg_log_prefix((MG_LL_INFO), "temp_no_pp.c", 194, __func__)) mg_log ("Parsing with MQTT version %d", version); } while (0);
      int result = mg_mqtt_parse(heap_buf, len, version, &mm);

      if (result == MQTT_OK) {
        do { if (mg_log_prefix((MG_LL_INFO), "temp_no_pp.c", 198, __func__)) mg_log ("Successfully parsed MQTT message"); } while (0);
        process_mqtt_message(c, &mm);
        mg_iobuf_del(&c->recv, 0, mm.dgram.len);
      } else if (result == MQTT_INCOMPLETE) {
        do { if (mg_log_prefix((MG_LL_INFO), "temp_no_pp.c", 202, __func__)) mg_log ("Incomplete MQTT message, waiting for more data"); } while (0);
      } else {
        do { if (mg_log_prefix((MG_LL_INFO), "temp_no_pp.c", 204, __func__)) mg_log ("MQTT parse error %d", result); } while (0);

        mg_iobuf_del(&c->recv, 0, len);
      }


      free(heap_buf);
    }
  } else if (ev == MG_EV_ACCEPT) {
    do { if (mg_log_prefix((MG_LL_INFO), "temp_no_pp.c", 213, __func__)) mg_log ("New connection accepted from %p", c->fd); } while (0);

    c->data[0] = 4;
  } else if (ev == MG_EV_CLOSE) {
    do { if (mg_log_prefix((MG_LL_INFO), "temp_no_pp.c", 217, __func__)) mg_log ("Connection %p closed", c->fd); } while (0);

    for (struct sub *next, *sub = s_subs; sub != 0; sub = next) {
      next = sub->next;
      if (c != sub->c) continue;
      do { if (mg_log_prefix((MG_LL_INFO), "temp_no_pp.c", 222, __func__)) mg_log ("Removing subscription [%.*s] for %p", (int) sub->topic.len, sub->topic.ptr, c->fd); } while (0)
                                                            ;
      do { struct sub **h = &s_subs; while (*h != (sub)) h = &(*h)->next; *h = (sub)->next; } while (0);
      free((void *)sub->topic.ptr);
      free(sub);
    }
  }
  (void) ev_data;
  (void) fn_data;
}

int main(void) {
  struct mg_mgr mgr;
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  mg_mgr_init(&mgr);

  printf("================================================\n");
  printf("---------------- MQTT Server --------------------\n");
  printf("================================================\n");
  printf("This is a  MQTT server that demonstrates\n");
  printf("basic mqtt functionality with mongoose v7.10.3\n");
  printf("\n");
  printf("Features:\n");
  printf("- CONNECT/CONNACK handling\n");
  printf("- SUBSCRIBE/SUBACK with topic management\n");
  printf("- PUBLISH message forwarding\n");
  printf("- PINGREQ/PINGRESP keep-alive\n");
  printf("\n");
  printf("Listening on %s\n", s_listen_on);
  printf("================================================\n\n");

  mg_mqtt_listen(&mgr, s_listen_on, fn, 0);

  while (s_signo == 0) mg_mgr_poll(&mgr, 1000);


  for (struct sub *next, *sub = s_subs; sub != 0; sub = next) {
    next = sub->next;
    free((void *)sub->topic.ptr);
    free(sub);
  }

  mg_mgr_free(&mgr);
  return 0;
}
