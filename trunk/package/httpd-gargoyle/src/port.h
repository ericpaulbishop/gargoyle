/* port.h - portability defines */

# define OS_Linux
# define ARCH "Linux"


# define HAVE_DAEMON
# define HAVE_SETSID
# define HAVE_WAITPID
# define HAVE_TM_GMTOFF
# define HAVE_SCANDIR
# define HAVE_INT64T

//sendfile functionality is broken in latest
// Kamikaze Openwrt (version 7.09)
//comment this out to compile a version that
//doesn't require it

//# define HAVE_SENDFILE
//# define HAVE_LINUX_SENDFILE

