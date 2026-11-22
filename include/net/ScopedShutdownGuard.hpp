#ifndef SCOPEDSHUTDOWNGUARD_HPP
# define SCOPEDSHUTDOWNGUARD_HPP

class ScopedShutdownGuard {
	private:
	
		bool	&_flag;
	
		ScopedShutdownGuard(const ScopedShutdownGuard &src);
		ScopedShutdownGuard	&operator=(const ScopedShutdownGuard &src);
	
	public:
		explicit	ScopedShutdownGuard(bool &flag);
		~ScopedShutdownGuard();
};

#endif
