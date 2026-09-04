/*
 * Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
 *
 * This file is part of StormByte-Network.
 *
 * StormByte-Network is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3
 * or later, as published by the Free Software Foundation.
 *
 * StormByte-Network is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with StormByte-Network. If not, see
 * <https://www.gnu.org/licenses/lgpl-3.0.html>.
 */

#include <StormByte/network/connection/handler.hxx>
#ifdef UNIX
#include <cerrno>		// For errno
#include <cstring>		// For strerror_r
#else
#include <winsock2.h>
#endif
#include <StormByte/string.hxx>
using namespace StormByte::Network::Connection;
Handler::Handler() noexcept {
	#ifdef WINDOWS
	// Initialize Winsock; set initialized=true only on success
	m_initialized = (WSAStartup(MAKEWORD(2, 2), &m_wsaData) == 0);
	#else
	m_initialized = true;
	#endif
}
Handler::~Handler() noexcept {
	#ifdef WINDOWS
	WSACleanup();
	#endif
}
Handler& Handler::Instance() noexcept {
	static Handler instance;
	return instance;
}
std::string Handler::LastError() const noexcept {
	std::string error_string;
	#ifdef WINDOWS
	wchar_t* errorMsg = nullptr;
	DWORD res = FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				reinterpret_cast<LPWSTR>(&errorMsg), 0, nullptr);
	if (res != 0 && errorMsg != nullptr) {
		error_string = StormByte::String::UTF8Encode(std::wstring(errorMsg));
		LocalFree(errorMsg);
	} else {
		// No message available; leave empty so callers can decide how to present it
		if (errorMsg) LocalFree(errorMsg);
	}
	#else
	if (errno != 0)
		error_string = ErrnoToString(errno);
	#endif
	return error_string;
}
int Handler::LastErrorCode() const noexcept {
	#ifdef WINDOWS
	return WSAGetLastError();
	#else
	return errno;
	#endif
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
