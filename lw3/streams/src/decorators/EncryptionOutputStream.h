#pragma once
#include "OutputStreamDecorator.h"
#include "EncryptionTable.h"
#include <array>

class EncryptionOutputStream : public OutputStreamDecorator
{
public:
	EncryptionOutputStream(std::unique_ptr<IOutputDataStream>&& stream, uint32_t key)
		: OutputStreamDecorator(std::move(stream))
		, m_encryptionTable(EncryptionUtils::GenerateEncryptionTable(key))
	{
	}

	void WriteByte(uint8_t data) override
	{
		uint8_t encryptedByte = m_encryptionTable[data];
		m_stream->WriteByte(encryptedByte);
	}

	void WriteBlock(const void* srcData, std::streamsize size) override
	{
		const auto* bytes = static_cast<const uint8_t*>(srcData);
		for (std::streamsize i = 0; i < size; ++i)
		{
			WriteByte(bytes[i]);
		}
	}

private:
	std::array<uint8_t, 256> m_encryptionTable;
};