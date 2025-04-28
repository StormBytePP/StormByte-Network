#pragma once

#include <StormByte/network/typedefs.hxx>
#include <StormByte/logger.hxx>

#include <atomic>

/**
 * @namespace StormByte::Network
 * @brief Contains all the network-related classes and utilities.
 */
namespace StormByte::Network {
    /**
     * @class EndPoint
     * @brief Represents a network endpoint, serving as a base class for both clients and servers.
     */
    class STORMBYTE_NETWORK_PUBLIC EndPoint {
        public:
            /**
             * @brief Constructs an EndPoint with the specified protocol, handler, and logger.
             * @param protocol The protocol to use (e.g., IPv4, IPv6).
             * @param handler A shared pointer to the connection handler.
             * @param logger A shared pointer to the logger instance.
             */
            EndPoint(const Connection::Protocol& protocol, std::shared_ptr<Connection::Handler> handler, std::shared_ptr<Logger> logger) noexcept;

            /**
             * @brief Deleted copy constructor to prevent copying.
             */
            EndPoint(const EndPoint& other) 									= delete;

            /**
             * @brief Defaulted move constructor.
             */
            EndPoint(EndPoint&& other) noexcept 								= default;

            /**
             * @brief Virtual destructor for proper cleanup in derived classes.
             */
            virtual ~EndPoint() noexcept;

            /**
             * @brief Deleted copy assignment operator to prevent copying.
             */
            EndPoint& operator=(const EndPoint& other) 							= delete;

            /**
             * @brief Defaulted move assignment operator.
             */
            EndPoint& operator=(EndPoint&& other) noexcept 						= default;

            /**
             * @brief Retrieves the protocol used by the endpoint.
             * @return A constant reference to the protocol.
             */
            const Connection::Protocol& 										Protocol() const noexcept;

        protected:
            Connection::Protocol m_protocol;									///< The protocol used by the endpoint (e.g., IPv4, IPv6).
            std::shared_ptr<Connection::Handler> m_handler;						///< Shared pointer to the connection handler.
            std::shared_ptr<Logger> m_logger;									///< Shared pointer to the logger instance.
            std::atomic<Connection::Status> m_status;							///< The current connection status of the endpoint.
			Socket::Socket* m_socket;											///< Unique pointer to the socket instance.
    };
}