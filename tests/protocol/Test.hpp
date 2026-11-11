#ifndef IRC_TESTS_PROTOCOL_TEST_HPP
#define IRC_TESTS_PROTOCOL_TEST_HPP

#include <iostream>
#include <sstream>
#include <stdexcept>

inline void check(bool condition, const char *expression, const char *file,
				  int line)
{
	if (!condition)
	{
		std::ostringstream message;
		message << file << ':' << line << ": check failed: " << expression;
		throw std::runtime_error(message.str());
	}
}

#define CHECK(expression) check((expression), #expression, __FILE__, __LINE__)

template <typename Test>
int runTests(const char *name, Test test)
{
	try
	{
		test();
		std::cout << "PASS " << name << '\n';
		return 0;
	}
	catch (const std::exception &error)
	{
		std::cerr << "FAIL " << name << ": " << error.what() << '\n';
	}
	catch (...)
	{
		std::cerr << "FAIL " << name << ": unexpected exception\n";
	}
	return 1;
}

#endif
