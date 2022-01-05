#include <iostream>
#include <fstream>
#include <iterator>
#include <thread>
#include <filesystem>
#include <boost/uuid/detail/md5.hpp>
#include <boost/algorithm/hex.hpp>

#include <Windows.h>

using boost::uuids::detail::md5;

std::string ToString(const md5::digest_type &digest)
{
	const auto intDigest = reinterpret_cast<const int*>(&digest);
	std::string result;
	boost::algorithm::hex(intDigest, intDigest + (sizeof(md5::digest_type) / sizeof(int)), std::back_inserter(result));
	return result;
}

long long GenerateSignatureFile(const std::vector<std::string> & stringsToProcess)
{
	std::ios_base::sync_with_stdio(false);
	auto startTime = std::chrono::high_resolution_clock::now();

	std::string result;
	for (const auto & block : stringsToProcess)
	{
		md5 hash;
		md5::digest_type digest;

		hash.process_bytes(block.data(), block.size());
		hash.get_digest(digest);

		const auto foo = ToString(digest);
		result.append(foo);
	}

	auto endTime = std::chrono::high_resolution_clock::now();

	std::ofstream outfile("nanocube.txt", std::ios::out | std::ios::binary);
	outfile.write(result.data(), result.size());

	return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
}

std::vector<std::string> ChopFile(const char * filePath)
{
	std::ios_base::sync_with_stdio(false);
	auto startTime = std::chrono::high_resolution_clock::now();

	const auto file = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	const auto sz = GetFileSize(file, nullptr);
	const auto buffSize = 1024 * 1024;

	SYSTEM_INFO SysInfo;
	GetSystemInfo(&SysInfo);
	const auto systemGranularity = SysInfo.dwAllocationGranularity;
	std::vector<std::string> result; // placeholder size

	for (DWORD i = 0; i < sz; i += buffSize)
	{
		const auto fileStart = i;
		const auto fileMapSize = fileStart + buffSize;
		const auto mapping = CreateFileMapping(file, nullptr, PAGE_READONLY, 0, fileMapSize, nullptr);
		if (!mapping)
			break;

		const auto fileMapStart = (fileStart / systemGranularity) * systemGranularity;
		const auto mapViewSize = (fileStart % systemGranularity) + buffSize;
		const auto viewOfFile = MapViewOfFile(mapping, FILE_MAP_READ, 0, fileMapStart, mapViewSize);

		const std::string data(static_cast<const char *>(viewOfFile), mapViewSize);
		result.emplace_back(data);

 		UnmapViewOfFile(viewOfFile);
		CloseHandle(mapping);
	}

	auto endTime = std::chrono::high_resolution_clock::now();
	std::cout << "chop_time: " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() << "\n";

	return result;
}

int main(int argc, char * argv[])
{
	//const auto path = "C:\\Program Files\\WindowsApps\\MSIXVC\\3D263E92-93CD-4F9B-90C7-5438150CECBF";
	const auto path = "C:\\dev\\testdata\\sample-2mb-text-file.txt";

	const auto chunks = ChopFile(path);
	std::cout << "GenerateSignatureFile duration: " << GenerateSignatureFile(chunks) << "\n";

	return 0;
}
