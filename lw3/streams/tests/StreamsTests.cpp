#include "../src/parse/CommandLineParser.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>

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