#include <StormByte/network/handler.hxx>
#include <StormByte/util/string.hxx>

#ifdef LINUX
#include <cerrno> // For errno
#include <cstring> // For strerror
#else
#include <windows.h>
#endif

using namespace StormByte::Network;

Handler::Handler() {
	#ifdef WINDOWS
	WSAStartup(MAKEWORD(2, 2), &m_wsaData);
	#endif
}

Handler::~Handler() {
	#ifdef WINDOWS
	WSACleanup();
	#endif
}

std::string Handler::LastError() const {
	std::string error_string;
	#ifdef WINDOWS
	wchar_t* errorMsg = nullptr;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				reinterpret_cast<LPWSTR>(&errorMsg), 0, nullptr);
	error_string = StormByte::Util::String::UTF8Encode(errorMsg);
	LocalFree(errorMsg);
	#else
	if (errno == 0)
		error_string = strerror(errno);
	#endif
	return error_string;
}
