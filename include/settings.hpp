#ifndef SETTINGS_HPP
# define SETTINGS_HPP

# ifndef MAX_CLIENT
#  define MAX_CLIENT 10
# endif	//	MAX_CLIENT

# ifndef MIN_PORT
#  define MIN_PORT 1
# endif	//	MIN_PORT

# ifndef MAX_PORT
#  define MAX_PORT 65535
# endif	//	MAX_PORT

# ifndef LISTEN_BACKLOG
#  define LISTEN_BACKLOG 128
# endif	//	LISTEN_BCKLOG

# ifndef POLL_TIMEOUT_MS
#  define POLL_TIMEOUT_MS -1
# endif	//	POLL_TIMEOUT_MS	

#ifndef CLIENT_READ_BUF_SIZE
# define CLIENT_READ_BUF_SIZE 512
#endif //	CLIENT_READ_BUF_SIZE

#ifndef DEFAULT_LINE_BUDGET
# define DEFAULT_LINE_BUDGET 4
#endif //	DEFAULT_LINE_BUDGET

#endif //	SETTINGS_HPP
