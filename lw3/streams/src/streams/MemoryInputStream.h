#pragma once
#include "IInputDataStream.h"
#include <vector>

class MemoryInputStream : public IInputDataStream
{
public:
	explicit MemoryInputStream(const std::vector<uint8_t>& data);

	bool IsEOF() const override;
	uint8_t ReadByte() override;
	std::streamsize ReadBlock(void* dstBuffer, std::streamsize size) override;

private:
	void EnsureIsNotEOF() const;

	std::vector<uint8_t> m_data;
	size_t m_position = 0;
};