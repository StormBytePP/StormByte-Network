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
	class STORMBYTE_NETWORK_PUBLIC Codec {
		public:
			Codec(const Codec& other) 								= default;
			Codec(Codec&& other) noexcept 							= default;
			Codec& operator=(const Codec& other) 					= default;
			Codec& operator=(Codec&& other) noexcept 				= default;
			virtual ~Codec() noexcept 								= default;
			// Encodes raw data from a consumer into a Packet
			virtual std::shared_ptr<Packet> 						Encode(const Buffer::Consumer& consumer) noexcept = 0;
			// Decodes a Packet into a network Object
			virtual std::shared_ptr<Object> 						Decode(const Packet& packet) noexcept = 0;
			// Processes a Packet into raw data for a consumer
			std::vector<std::byte> 									Process(const Packet& packet) noexcept;

			inline void SetPipeline(const Buffer::Pipeline& in_pipeline, const Buffer::Pipeline& out_pipeline) noexcept {
				m_in_pipeline = in_pipeline;
				m_out_pipeline = out_pipeline;
			}

		protected:
			Buffer::Pipeline m_in_pipeline, m_out_pipeline;
			Logger::ThreadedLog m_log;
			
			inline Codec(const Logger::ThreadedLog& log) noexcept: m_log(log) {}
	};
}