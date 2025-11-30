#pragma once

#include <StormByte/expected.hxx>
#include <StormByte/logger/threaded_log.hxx>
#include <StormByte/network/connection/handler.hxx>
#include <StormByte/network/connection/info.hxx>
#include <StormByte/network/connection/protocol.hxx>
#include <StormByte/network/connection/rw.hxx>
#include <StormByte/network/connection/status.hxx>
#include <StormByte/network/exception.hxx>
#include <StormByte/network/typedefs.hxx>

/**
 * @namespace Socket
 * @brief The namespace containing all the socket related classes.
 */
namespace StormByte::Network::Socket {
	class Server;
	class STORMBYTE_NETWORK_PUBLIC Socket {
		friend class Server;
	public:
		Socket(const Socket& other) = delete;
		Socket(Socket&& other) noexcept;
		virtual ~Socket() noexcept;
		Socket& operator=(const Socket& other) = delete;
		Socket& operator=(Socket&& other) noexcept;

		virtual void Disconnect() noexcept;

		constexpr const Connection::Status& Status() const noexcept { return m_status; }
		constexpr const unsigned long& MTU() const noexcept { return m_mtu; }
		inline const Connection::Handler::Type& Handle() const noexcept { return *m_handle; }
		ExpectedReadResult WaitForData(const long long& usecs = 0) noexcept;

	protected:
		Connection::Protocol m_protocol;
		Connection::Status m_status;
		std::unique_ptr<const Connection::Handler::Type> m_handle;
		std::shared_ptr<const Connection::Handler> m_conn_handler;
		std::unique_ptr<Connection::Info> m_conn_info;
		unsigned long m_mtu;
		Logger::ThreadedLog m_logger;

		Socket(const Connection::Protocol& protocol, std::shared_ptr<const Connection::Handler> handler, Logger::ThreadedLog logger) noexcept;
		Expected<Connection::Handler::Type, ConnectionError> CreateSocket() noexcept;
		void InitializeAfterConnect() noexcept;

	private:
		constexpr static const unsigned short DEFAULT_MTU = 1500;
		int GetMTU() const noexcept;
		void SetNonBlocking() noexcept;
	};
}