#pragma once

#include <StormByte/buffer/external.hxx>
#include <StormByte/network/visibility.h>

#include <functional>

/**
 * @namespace Socket
 * @brief Low-level socket wrappers.
 */
namespace StormByte::Network::Socket {
	class Client;

	/**
	 * @class Writer
	 * @brief Buffer::ExternalWriter adapter over Client.
	 */
	class STORMBYTE_NETWORK_PRIVATE Writer final: public Buffer::ExternalWriter {
		public:
			/**
			 * @param client Non-owning client reference.
			 */
			inline Writer(Client& client) noexcept
				: m_client(client) {}

			Writer(const Writer& other) noexcept = default;
			Writer(Writer&& other) noexcept = default;
			~Writer() noexcept override = default;

			Writer& operator=(const Writer& other) noexcept = default;
			Writer& operator=(Writer&& other) noexcept = default;

			/**
			 * @return Cloned writer.
			 */
			inline PointerType Clone() const noexcept override {
				return MakePointer<Writer>(*this);
			}

			/**
			 * @return Moved writer instance.
			 */
			inline PointerType Move() noexcept override {
				return MakePointer<Writer>(std::move(*this));
			}

			/**
			 * @return false after Close or SetError.
			 */
			bool IsWritable() const noexcept override;

			/**
			 * Sends all of @p data.
			 * @param data Bytes.
			 * @return true on success.
			 */
			bool Write(const Buffer::DataType& data) noexcept override;

			/**
			 * Sends all of @p data (move).
			 * @param data Bytes.
			 * @return true on success.
			 */
			bool Write(Buffer::DataType&& data) noexcept override;

			/**
			 * Sends up to @p count bytes from @p data.
			 * @param count Max bytes.
			 * @param data Source.
			 * @return true on success.
			 */
			bool Write(std::size_t count, const Buffer::DataType& data) noexcept override;

			/**
			 * Sends up to @p count bytes from @p data (rvalue).
			 * @param count Max bytes.
			 * @param data Source.
			 * @return true on success.
			 */
			bool Write(std::size_t count, Buffer::DataType&& data) noexcept override;

			/**
			 * Disallows further writes (local flag).
			 */
			void Close() noexcept override;

			/**
			 * Marks permanent error (local flag).
			 */
			void SetError() noexcept override;

			using Buffer::ExternalWriter::Write;

		private:
			std::reference_wrapper<Client> m_client;	///< Client socket
			bool m_writable = true;						///< Writable flag
			bool m_error = false;						///< Error flag
	};
}
