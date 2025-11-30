#include <StormByte/network/connection/handler.hxx>
#include <StormByte/string.hxx>

#ifdef LINUX
#include <cerrno> // For errno
#include <cstring> // For strerror_r
#else
#include <windows.h>
#endif

#include <cstring>

using namespace StormByte::Network::Connection;

Handler::Handler(const Network::PacketInstanceFunction& packet_instance_function) noexcept:
m_packet_instance_function(packet_instance_function) {
	#ifdef WINDOWS
	m_initialized = WSAStartup(MAKEWORD(2, 2), &m_wsaData) != 0;
	#else
	m_initialized = true;
	#endif
}

Handler::~Handler() noexcept {
	#ifdef WINDOWS
	WSACleanup();
	#endif
}

std::string Handler::LastError() const noexcept {
	std::string error_string;
	#ifdef WINDOWS
	wchar_t* errorMsg = nullptr;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				reinterpret_cast<LPWSTR>(&errorMsg), 0, nullptr);
	error_string = StormByte::String::UTF8Encode(errorMsg);
	LocalFree(errorMsg);
	#else
	if (errno != 0)
		error_string = ErrnoToString(errno);
	#endif
	return error_string;
}

std::string Handler::ErrnoToString(int errnum) const noexcept {
	#ifdef WINDOWS
    char buf[256] = {0};
    if (strerror_s(buf, sizeof(buf), errnum) == 0) return std::string(buf);
    return std::to_string(errnum);
	#else
    char buf[256] = {0};
    // Handle both GNU (returns char*) and POSIX (returns int) strerror_r variants
	#if defined(__GLIBC__) && !defined(__APPLE__)
    char *msg = strerror_r(errnum, buf, sizeof(buf));
    return std::string(msg ? msg : "Unknown error");
	#else
    if (strerror_r(errnum, buf, sizeof(buf)) == 0) return std::string(buf);
    return std::to_string(errnum);
	#endif
	#endif
}

int Handler::LastErrorCode() const noexcept {
	#ifdef WINDOWS
	return WSAGetLastError();
	#else
	return errno;
	#endif
}

const StormByte::Network::PacketInstanceFunction& Handler::PacketInstanceFunction() const noexcept {
	return m_packet_instance_function;
}