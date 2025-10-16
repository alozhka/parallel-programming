#pragma once
#include "InputStreamDecorator.h"
#include "EncryptionTable.h"
#include <array>

class DecryptionInputStream : public InputStreamDecorator
{
public:
	DecryptionInputStream(std::unique_ptr<IInputDataStream>&& stream, uint32_t key)
		: InputStreamDecorator(std::move(stream))
		, m_decryptionTable(EncryptionUtils::GenerateDecryptionTable(key))
	{
	}

	uint8_t ReadByte() override
	{
		uint8_t encryptedByte = m_stream->ReadByte();
		return m_decryptionTable[encryptedByte];
	}

	std::streamsize ReadBlock(void* dstBuffer, std::streamsize size) override
	{
		auto* bytes = static_cast<uint8_t*>(dstBuffer);
		std::streamsize bytesRead = 0;

		for (std::streamsize i = 0; i < size && !m_stream->IsEOF(); ++i)
		{
			bytes[i] = ReadByte();
			++bytesRead;
		}

		return bytesRead;
	}

private:
	std::array<uint8_t, 256> m_decryptionTable;
};