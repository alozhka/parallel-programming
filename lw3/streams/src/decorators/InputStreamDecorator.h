#pragma once
#include "../streams/IInputDataStream.h"
#include <memory>

// Абстрактный базовый класс для декораторов входного потока
class InputStreamDecorator : public IInputDataStream
{
public:
	explicit InputStreamDecorator(std::unique_ptr<IInputDataStream>&& stream)
		: m_stream(std::move(stream))
	{
	}

	bool IsEOF() const override
	{
		return m_stream->IsEOF();
	}

protected:
	std::unique_ptr<IInputDataStream> m_stream;
};