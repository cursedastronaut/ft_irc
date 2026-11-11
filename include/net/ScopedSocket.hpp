#ifndef SCOPEDSOCKET_HPP
# define SCOPEDSOCKET_HPP

# include <exception>
# include <sys/types.h>
# include <sys/socket.h>
# include <unistd.h>

class ScopedSocket {
	private:
		int	_fd;

		ScopedSocket(const ScopedSocket &src);
		ScopedSocket	&operator=(const ScopedSocket &src);

	public:
		//	Constructors
		ScopedSocket();
		ScopedSocket(int fd);

		//	Destructor
		~ScopedSocket();

		//	Exceptions
		class SocketCreationException: public std::exception{
			public:
				virtual const char *what() const throw();
		};
		
		class InvalidFamilyException: public std::exception{
			public:
				virtual const char *what() const throw();
		};

		//	Geter
		int		get() const;
		bool	isValid() const;

		//	Seter
		void	openTcp(int domain = AF_INET);
		int		release();
		void	reset(int new_fd = -1);
		void	swap(ScopedSocket &other);
};

#endif //	SCOPEDSOCKET_HPP
