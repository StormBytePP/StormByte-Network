#pragma once

#include <StormByte/buffer/pipeline.hxx>
#include <StormByte/logger/threaded_log.hxx>
#include <StormByte/network/object.hxx>
#include <StormByte/network/packet.hxx>

#include <memory>

/**
 * @namespace StormByte::Network
 * @brief Contains all the network-related classes and utilities.
 */
namespace StormByte::Network {
	// Class responsible for a correct high level network communication between a server and a client.
	// It encodes and decodes Packets into/from network Objects, using Buffer Pipelines for preprocessing
	// input data and postprocessing output data (for example, for compression or encryption).
	/**
	 * @class Codec
	 * @brief High-level encoder/decoder for network messages.
	 *
	 * `Codec` provides the application-level translation between raw byte
	 * streams and domain objects (`Network::Object`) via an intermediate
	 * `Packet` representation. It is intended to be subclassed to implement a
	 * concrete wire protocol (opcodes, framing, versioning, etc.).
	 *
	 * Responsibilities:
	 * - Decode incoming `Packet` instances into typed `Network::Object`
	 *   instances used by application code.
	 * - Encode application-level data (via a `Buffer::Consumer`) into a
	 *   `Packet` ready for transport.
	 * - Provide `Process()` that turns a `Packet` into raw bytes applying
	 *   optional input/output `Buffer::Pipeline` stages (for example
	 *   compression/encryption).
	 *
	 * Threading and ownership:
	 * - `Codec` instances may hold a `Logger::ThreadedLog` for diagnostics.
	 * - The public API uses `std::shared_ptr<Packet>` and
	 *   `std::shared_ptr<Object>` to preserve polymorphism for user-defined
	 *   `Packet`/`Object` hierarchies and to simplify lifetime management
	 *   across asynchronous boundaries.
	 *
	 * Usage notes:
	 * - Implement `Encode()`/`Decode()` in derived classes. `Encode()` should
	 *   read from the provided `Buffer::Consumer` and construct a `Packet`.
	 * - Call `SetPipeline()` to configure preprocessing/postprocessing
	 *   pipelines applied by `Process()`.
	 *
	 */
	class STORMBYTE_NETWORK_PUBLIC Codec {
		public:
			/**
			 * @brief Copy constructor.
			 *
			 * Defaulted copy performs member-wise copy of pipelines and the
			 * logger. It preserves the public API semantics and keeps
			 * polymorphic behaviour for returned `Packet`/`Object` instances.
			 * Derived classes that hold additional non-copyable state should
			 * explicitly define a copy constructor.
			 */
			Codec(const Codec& other)						= default;

			/**
			 * @brief Move constructor.
			 *
			 * Defaulted move will move pipeline and logger members. Marked
			 * `noexcept` to allow efficient container operations and to
			 * communicate that moving a `Codec` will not throw.
			 */
			Codec(Codec&& other) noexcept					= default;

			/**
			 * @brief Copy assignment.
			 *
			 * Defaulted copy-assignment performs member-wise assignment. As
			 * with the copy constructor, derived classes with special state
			 * should provide their own implementation when necessary.
			 */
			Codec& operator=(const Codec& other)			= default;

			/**
			 * @brief Move assignment.
			 *
			 * Defaulted move-assignment transfers ownership of movable
			 * members and is marked `noexcept` to match the move
			 * constructor's exception-safety guarantees.
			 */
			Codec& operator=(Codec&& other) noexcept		= default;

			/**
			 * @brief Virtual destructor.
			 *
			 * Defaulted virtual destructor ensures correct cleanup through
			 * polymorphic interfaces in derived `Codec` implementations.
			 */
			virtual ~Codec() noexcept						= default;

			/**
			 * @brief Encode data from a `Buffer::Consumer` into a `Packet`.
			 *
			 * Implementations should consume bytes from `consumer`, parse
			 * or assemble them according to the application protocol, and
			 * return a `Packet` ready for network transport. Return value is
			 * wrapped in `std::shared_ptr` for lifetime convenience.
			 *
			 * @param consumer Source of raw input bytes.
			 * @return Shared pointer to constructed `Packet` (or nullptr on failure).
			 */
			std::shared_ptr<Packet>							Encode(Buffer::Consumer consumer) const noexcept;

			/**
			 * @brief Decode a `Packet` into a domain `Object`.
			 *
			 * Translate the protocol-level `Packet` into a typed object the
			 * application can operate on. Implementations should return a
			 * `std::shared_ptr<Object>` pointing to a concrete subclass.
			 *
			 * @param packet The packet to decode.
			 * @return Shared pointer to decoded `Object` (or nullptr on failure).
			 */
			inline std::shared_ptr<Object>					Decode(const Packet& packet) const noexcept {
				return DoDecode(packet);
			}

			/**
			 * @brief Turn a `Packet` into raw bytes applying output pipeline.
			 *
			 * This helper applies any configured `m_out_pipeline` stages and
			 * returns the final byte vector that should be written to the
			 * transport. It is a convenience wrapper around `Packet` -> bytes
			 * conversion and pipeline steps.
			 *
			 * @param packet The packet to process.
			 * @return Buffer::FIFO of bytes ready for sending on the wire.
			 */
			Buffer::FIFO									Process(const Packet& packet) const noexcept;

			/**
			 * @brief Configure input and output buffer pipelines.
			 *
			 * Pipelines are applied by `Process()` and may perform
			 * transformations such as compression, encryption, checksumming,
			 * or framing adjustments. Both pipelines are copied.
			 *
			 * @param in_pipeline Pipeline applied to incoming data.
			 * @param out_pipeline Pipeline applied before sending data.
			 */
			inline void SetPipeline(const Buffer::Pipeline& in_pipeline, const Buffer::Pipeline& out_pipeline) noexcept {
				m_in_pipeline = in_pipeline;
				m_out_pipeline = out_pipeline;
			}

		protected:
			Buffer::Pipeline m_in_pipeline, m_out_pipeline;	///< Pre/post-processing pipelines
			mutable Logger::ThreadedLog m_log;				///< Logger for diagnostics

			/**
			 * @brief Protected ctor accepting a logger.
			 *
			 * Derived implementations may pass a `ThreadedLog` for
			 * runtime diagnostics; defaulted to empty logger if not used.
			 */
			inline Codec(const Logger::ThreadedLog& log) noexcept: m_log(log) {}

		private:
			/**
			 * @brief Construct a `Packet` from raw payload bytes.
			 *
			 * Implementations receive the `opcode` (already parsed) and a
			 * `Buffer::Consumer` containing only the packet payload bytes — the
			 * framing layer has already removed opcode bytes from the consumer.
			 * Implementors MUST NOT attempt to re-read the opcode from the
			 * `consumer`; instead, focus on parsing the custom payload fields for
			 * the given opcode. The function must parse
			 * the consumer according to the wire-format for the given opcode and
			 * return a concrete `Packet` instance wrapped in `std::shared_ptr`.
			 *
			 * The method is `noexcept` to keep the Codec API exception-free; when
			 * parsing fails implementations should return `nullptr` rather than
			 * throwing.
			 *
			 * @param opcode Already-parsed opcode identifying the packet type.
			 * @param consumer Provides the packet payload bytes to decode.
			 * @return `std::shared_ptr<Packet>` to a newly-constructed packet,
			 *         or `nullptr` on parse/error.
			 */
			virtual std::shared_ptr<Packet>					DoEncode(const unsigned short& opcode, Buffer::Consumer consumer) const noexcept = 0;

			/**
			 * @brief Construct a domain `Object` from a decoded `Packet`.
			 *
			 * Translate a protocol-level `Packet` into an application-level
			 * `Object`. Implementations should examine the packet contents and
			 * return a `std::shared_ptr<Object>` pointing to a concrete derived
			 * type. Return `nullptr` to indicate an error or an unknown/unsupported
			 * packet.
			 *
			 * This function is `noexcept` by API contract; do not throw from
			 * implementations.
			 *
			 * @param packet Decoded packet to convert.
			 * @return `std::shared_ptr<Object>` to the created object, or `nullptr`.
			 */
			virtual std::shared_ptr<Object>					DoDecode(const Packet& packet) const noexcept = 0;
	};
}