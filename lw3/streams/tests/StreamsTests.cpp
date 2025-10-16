#include "../src/parse/CommandLineParser.h"
#include "../src/streams/MemoryOutputStream.h"
#include "../src/streams/MemoryInputStream.h"
#include "../src/decorators/EncryptionOutputStream.h"
#include "../src/decorators/DecryptionInputStream.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>
#include <algorithm>
#include <memory>

class StreamsTests : public ::testing::Test
{
};

TEST_F(StreamsTests, ParseSimpleEncryptCommand)
{
	std::vector<const char*> argv = {"transform.exe", "--encrypt", "3", "input.dat", "output.dat"};
	int argc = static_cast<int>(argv.size());

	auto options = CommandLineParser::Parse(argc, const_cast<char**>(argv.data()));

	ASSERT_EQ(options.inputFile, "input.dat");
	ASSERT_EQ(options.outputFile, "output.dat");
	ASSERT_EQ(options.inputOperations.size(), 0);
	ASSERT_EQ(options.outputOperations.size(), 1);
	ASSERT_EQ(options.outputOperations[0].type, OperationType::ENCRYPT);
	ASSERT_EQ(options.outputOperations[0].key, 3);
}

TEST_F(StreamsTests, ParseMultipleEncryptCommands)
{
	std::vector<const char*> argv = {"transform.exe", "--encrypt", "3", "--encrypt", "100500", "input.dat", "output.dat"};
	int argc = static_cast<int>(argv.size());

	auto options = CommandLineParser::Parse(argc, const_cast<char**>(argv.data()));

	ASSERT_EQ(options.inputFile, "input.dat");
	ASSERT_EQ(options.outputFile, "output.dat");
	ASSERT_EQ(options.outputOperations.size(), 2);
	ASSERT_EQ(options.outputOperations[0].type, OperationType::ENCRYPT);
	ASSERT_EQ(options.outputOperations[0].key, 3);
	ASSERT_EQ(options.outputOperations[1].type, OperationType::ENCRYPT);
	ASSERT_EQ(options.outputOperations[1].key, 100500);
}

TEST_F(StreamsTests, ParseEncryptAndCompressCommands)
{
	std::vector<const char*> argv = {"transform.exe", "--encrypt", "3", "--encrypt", "100500", "--compress", "input.dat", "output.dat"};
	int argc = static_cast<int>(argv.size());

	auto options = CommandLineParser::Parse(argc, const_cast<char**>(argv.data()));

	ASSERT_EQ(options.inputFile, "input.dat");
	ASSERT_EQ(options.outputFile, "output.dat");
	ASSERT_EQ(options.outputOperations.size(), 3);
	ASSERT_EQ(options.outputOperations[0].type, OperationType::ENCRYPT);
	ASSERT_EQ(options.outputOperations[0].key, 3);
	ASSERT_EQ(options.outputOperations[1].type, OperationType::ENCRYPT);
	ASSERT_EQ(options.outputOperations[1].key, 100500);
	ASSERT_EQ(options.outputOperations[2].type, OperationType::COMPRESS);
	ASSERT_EQ(options.outputOperations[2].key, 0);
}

TEST_F(StreamsTests, ParseDecryptCommands)
{
	std::vector<const char*> argv = {"transform.exe", "--decompress", "--decrypt", "100500", "--decrypt", "3", "output.dat", "input.dat.restored"};
	int argc = static_cast<int>(argv.size());

	auto options = CommandLineParser::Parse(argc, const_cast<char**>(argv.data()));

	ASSERT_EQ(options.inputFile, "output.dat");
	ASSERT_EQ(options.outputFile, "input.dat.restored");
	ASSERT_EQ(options.inputOperations.size(), 3);
	ASSERT_EQ(options.inputOperations[0].type, OperationType::DECOMPRESS);
	ASSERT_EQ(options.inputOperations[1].type, OperationType::DECRYPT);
	ASSERT_EQ(options.inputOperations[1].key, 100500);
	ASSERT_EQ(options.inputOperations[2].type, OperationType::DECRYPT);
	ASSERT_EQ(options.inputOperations[2].key, 3);
}

TEST_F(StreamsTests, ParseCommandsMissingArguments)
{
	std::vector<const char*> argv = {"transform.exe", "--encrypt"};
	int argc = static_cast<int>(argv.size());

	ASSERT_THROW(CommandLineParser::Parse(argc, const_cast<char**>(argv.data())), std::invalid_argument);
}

TEST_F(StreamsTests, ParseCommandsMissingFiles)
{
	std::vector<const char*> argv = {"transform.exe", "--encrypt", "3"};
	size_t argc = argv.size();

	ASSERT_THROW(CommandLineParser::Parse(argc, const_cast<char**>(argv.data())), std::invalid_argument);
}

TEST_F(StreamsTests, ParseCommandsInvalidKey)
{
	std::vector<const char*> argv = {"transform.exe", "--encrypt", "abc", "input.dat", "output.dat"};
	int argc = static_cast<int>(argv.size());

	ASSERT_THROW(CommandLineParser::Parse(argc, const_cast<char**>(argv.data())), std::invalid_argument);
}

// ========== MemoryOutputStream Tests ==========

TEST_F(StreamsTests, OutputStreamWritesBytes)
{
	MemoryOutputStream stream;
	stream.WriteByte(1);
	stream.WriteByte(2);
	stream.WriteByte(3);

	const auto& data = stream.GetData();
	ASSERT_EQ(data.size(), 3);
	ASSERT_EQ(data[0], 1);
	ASSERT_EQ(data[1], 2);
	ASSERT_EQ(data[2], 3);
}

TEST_F(StreamsTests, OutputStreamWritesBlock)
{
	MemoryOutputStream stream;
	uint8_t buffer[] = {10, 20, 30, 40, 50};
	stream.WriteBlock(buffer, 5);

	const auto& data = stream.GetData();
	ASSERT_EQ(data.size(), 5);
	ASSERT_TRUE(std::ranges::equal(data, buffer));
}

TEST_F(StreamsTests, OutputStreamCanCloseStream)
{
	MemoryOutputStream stream;
	stream.WriteByte(42);
	stream.Close();

	const auto& data = stream.GetData();
	ASSERT_EQ(data.size(), 1);
	ASSERT_EQ(data[0], 42);
}

TEST_F(StreamsTests, CannotWriteToClosedOutputStream)
{
	MemoryOutputStream stream;
	uint8_t buffer[] = {1, 2, 3};

	stream.WriteByte(42);
	stream.Close();

	ASSERT_THROW(stream.WriteByte(100), std::logic_error);
	ASSERT_THROW(stream.WriteBlock(buffer, 3), std::logic_error);
}

// ========== MemoryInputStream Tests ==========

TEST_F(StreamsTests, InputStreamReadsData)
{
	std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6};
	MemoryInputStream stream(data);

	ASSERT_EQ(stream.ReadByte(), 1);

	uint8_t buffer[3] = {};
	std::streamsize bytesRead = stream.ReadBlock(buffer, 3);
	ASSERT_EQ(bytesRead, 3);
	ASSERT_EQ(buffer[0], 2);
	ASSERT_EQ(buffer[1], 3);
	ASSERT_EQ(buffer[2], 4);

	ASSERT_FALSE(stream.IsEOF());
	ASSERT_EQ(stream.ReadByte(), 5);
	ASSERT_FALSE(stream.IsEOF());
	ASSERT_EQ(stream.ReadByte(), 6);
	ASSERT_TRUE(stream.IsEOF());
}

TEST_F(StreamsTests, InputStreamReadsBeyondEnd)
{
	std::vector<uint8_t> data = {1, 2, 3};
	MemoryInputStream stream(data);

	uint8_t buffer[10] = {};
	std::streamsize bytesRead = stream.ReadBlock(buffer, 10);

	ASSERT_EQ(bytesRead, 3);
	ASSERT_EQ(buffer[0], 1);
	ASSERT_EQ(buffer[1], 2);
	ASSERT_EQ(buffer[2], 3);
	ASSERT_TRUE(stream.IsEOF());
}

TEST_F(StreamsTests, InputStreamReadFromEmptyStream)
{
	std::vector<uint8_t> data = {};
	MemoryInputStream stream(data);

	ASSERT_TRUE(stream.IsEOF());
	ASSERT_THROW(stream.ReadByte(), std::ios_base::failure);
}

// ========== Encryption/Decryption Tests ==========

TEST_F(StreamsTests, CanEncryptData)
{
	auto memoryStream = std::make_unique<MemoryOutputStream>();
	MemoryOutputStream* rawPtr = memoryStream.get();

	EncryptionOutputStream encryptedStream(std::move(memoryStream), 12345);

	std::vector<uint8_t> originalData = {1, 2, 3, 4, 5};
	encryptedStream.WriteBlock(originalData.data(), originalData.size());
	encryptedStream.Close();

	const auto& encryptedData = rawPtr->GetData();
	ASSERT_EQ(encryptedData.size(), originalData.size());

	ASSERT_FALSE(std::ranges::equal(encryptedData, originalData));
}

TEST_F(StreamsTests, CanDecryptData)
{
	// Шифруем данные
	auto memoryStreamOut = std::make_unique<MemoryOutputStream>();
	MemoryOutputStream* rawPtrOut = memoryStreamOut.get();

	EncryptionOutputStream encryptedStream(std::move(memoryStreamOut), 12345);

	std::vector<uint8_t> originalData = {10, 20, 30, 40, 50};
	encryptedStream.WriteBlock(originalData.data(), originalData.size());
	encryptedStream.Close();

	// Дешифруем данные
	const auto& encryptedData = rawPtrOut->GetData();
	auto memoryStreamIn = std::make_unique<MemoryInputStream>(encryptedData);

	DecryptionInputStream decryptedStream(std::move(memoryStreamIn), 12345);

	std::vector<uint8_t> decryptedData(originalData.size());
	std::streamsize bytesRead = decryptedStream.ReadBlock(decryptedData.data(), decryptedData.size());

	ASSERT_EQ(bytesRead, originalData.size());
	ASSERT_TRUE(std::ranges::equal(decryptedData, originalData));
}

TEST_F(StreamsTests, EncryptionAndDecryptionTakesInitalData)
{
	// Записываем зашифрованные данные
	auto memoryStreamOut = std::make_unique<MemoryOutputStream>();
	MemoryOutputStream* rawPtrOut = memoryStreamOut.get();
	EncryptionOutputStream encryptedStream(std::move(memoryStreamOut), 99999);
	std::string initialString = "Hello, World!";
	
	encryptedStream.WriteBlock(initialString.data(), initialString.size());
	encryptedStream.Close();

	const auto& encryptedData = rawPtrOut->GetData();
	auto memoryStreamIn = std::make_unique<MemoryInputStream>(encryptedData);
	DecryptionInputStream decryptedStream(std::move(memoryStreamIn), 99999);
	std::vector<char> decryptedBuffer(initialString.size());

	std::streamsize bytesRead = decryptedStream.ReadBlock(decryptedBuffer.data(), decryptedBuffer.size());

	ASSERT_EQ(bytesRead, initialString.size());
	std::string decryptedString(decryptedBuffer.begin(), decryptedBuffer.end());
	ASSERT_EQ(decryptedString, initialString);
}

TEST_F(StreamsTests, DifferentKeysProduceDifferentEncryption)
{
	std::vector<uint8_t> originalData = {1, 2, 3, 4, 5};

	// Шифруем с ключом 1
	auto memoryStream1 = std::make_unique<MemoryOutputStream>();
	MemoryOutputStream* rawPtr1 = memoryStream1.get();
	EncryptionOutputStream encryptedStream1(std::move(memoryStream1), 1);
	encryptedStream1.WriteBlock(originalData.data(), originalData.size());
	encryptedStream1.Close();

	// Шифруем с ключом 2
	auto memoryStream2 = std::make_unique<MemoryOutputStream>();
	MemoryOutputStream* rawPtr2 = memoryStream2.get();
	EncryptionOutputStream encryptedStream2(std::move(memoryStream2), 2);
	encryptedStream2.WriteBlock(originalData.data(), originalData.size());
	encryptedStream2.Close();

	const auto& encrypted1 = rawPtr1->GetData();
	const auto& encrypted2 = rawPtr2->GetData();

	// Разные ключи должны давать разные зашифрованные данные
	ASSERT_FALSE(std::ranges::equal(encrypted1, encrypted2));
}