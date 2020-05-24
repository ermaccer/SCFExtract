// scfextract.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include "filef.h"



struct rcf_header {
	char  header[32] = {};
	int   version;
	int   data_offset;
	int   data_size;
	int   name_offset;
	int   name_size;
	int   pad;
	int   files;
};

struct rcf_entry {
	unsigned int crc;
	int offset;
	int size;
};

enum eModes {
	MODE_EXTRACT = 1,
	MODE_CREATE,
	PARAM_SKIPCRC
};

bool rcf_compare(rcf_entry first, rcf_entry in)
{
	return first.offset < in.offset;
}


int main(int argc, char* argv[])
{
	if (argc == 1)
	{
		std::cout << "Scarface RCF Extractor by ermaccer\n"
			"Usage: scfextract <input>\n"
			"Params:\n"
			"   -e         Extracts input archive\n"
			"   -o <name>  Specifies output filename/folder\n";
		return 0;
	}


	int mode = 0;
	int param = 0;
	std::string o_param;
	std::string l_param;





	// params
	for (int i = 1; i < argc - 1; i++)
	{
		if (argv[i][0] != '-' || strlen(argv[i]) != 2) {
			return 1;
		}
		switch (argv[i][1])
		{
		case 'e': mode = MODE_EXTRACT;
			break;
			//TODO
			// finish creation
		case 'c': mode = MODE_EXTRACT; 
			break;
		case 's': param = PARAM_SKIPCRC;
			break;
		case 'o':
			i++;
			o_param = argv[i];
			break;
		default:
			std::cout << "ERROR: Param does not exist: " << argv[i] << std::endl;
			return 1;
			break;
		}
	}

	if (mode == MODE_EXTRACT)
	{

		std::ifstream pFile(argv[argc - 1], std::ifstream::binary);

		if (!pFile)
		{
			std::cout << "ERROR: Failed to open " << argv[argc - 1] << "!" << std::endl;
			return 1;
		}

		if (pFile)
		{
			rcf_header rcf;
			pFile.read((char*)&rcf, sizeof(rcf_header));

			if (!(strcmp(rcf.header, "ATG CORE CEMENT LIBRARY") == 0))
			{
				std::cout << "ERROR: " << argv[argc - 1] << "is not a valid RCF archive!" << std::endl;
				return 1;
			}

			pFile.seekg(rcf.data_offset, pFile.beg);

			std::vector<rcf_entry> entries;
			std::vector<rcf_entry> sortedEntries;
			std::vector<std::string> names;
			for (int i = 0; i < rcf.files; i++)
			{
				rcf_entry ent;
				pFile.read((char*)&ent, sizeof(rcf_entry));
				entries.push_back(ent);
			}

			pFile.seekg(rcf.name_offset + (sizeof(int) * 2), pFile.beg);

			for (int i = 0; i < rcf.files; i++)
			{
				pFile.seekg(sizeof(int) * 3, pFile.cur);
				int len;
				pFile.read((char*)&len, sizeof(int));
				std::unique_ptr<char[]> name = std::make_unique<char[]>(len);
				pFile.read(name.get(), len - 1);
				std::string tmp(name.get(), len - 1);
				names.push_back(tmp);
				pFile.seekg(sizeof(int), pFile.cur);
			}




			if (!o_param.empty())
			{
				if (!std::experimental::filesystem::exists(o_param))
					std::experimental::filesystem::create_directory(o_param);

				std::experimental::filesystem::current_path(o_param);
			}




			std::sort(entries.begin(), entries.end(), rcf_compare);

			for (int i = 0; i < rcf.files; i++)
			{
				int dataSize = entries[i].size;

				pFile.seekg(entries[i].offset, pFile.beg);
				std::unique_ptr<char[]> dataBuff = std::make_unique<char[]>(dataSize);
				pFile.read(dataBuff.get(), dataSize);
				if (checkSlash(names[i]))
					std::experimental::filesystem::create_directories(splitString(names[i], false));


				std::ofstream oFile(names[i], std::ofstream::binary);
				oFile.write(dataBuff.get(), dataSize);
				std::cout << "Processing: " << splitString(names[i], true) << std::endl;
			}
			std::cout << "Finished." << std::endl;
	}


	}
	if (mode == MODE_CREATE)
	{
		std::experimental::filesystem::path folder(argv[argc - 1]);
		if (!std::experimental::filesystem::exists(folder))
		{
			std::cout << "ERROR: Could not open directory: " << argv[argc - 1] << "!" << std::endl;
			return 1;
		}
		
		if (std::experimental::filesystem::exists(folder))
		{

			std::vector<std::string> names;
			std::vector<std::string> paths;
			std::vector<int> sizes;
			std::vector<unsigned int> crc;

			for (const auto & file : std::experimental::filesystem::recursive_directory_iterator(folder))
			{
				if (!std::experimental::filesystem::is_directory(file.path().string()))
				{
					std::string tmp(file.path().relative_path().string().c_str() + folder.string().size() + 1, file.path().relative_path().string().size() - folder.string().size());
					names.push_back(tmp);
					paths.push_back(file.path().string());
					std::ifstream tFile(file.path().string(), std::ifstream::binary);
					if (tFile)
					{
						unsigned int size = getSizeToEnd(tFile);
						sizes.push_back(size);
						std::unique_ptr<char[]> buff = std::make_unique<char[]>(size);
			
						// TODO
						// do crc stuff here
						unsigned long  calccrc = 0;
						crc.push_back(calccrc);
					}
					//std::cout << tmp << std::endl;
				}
			}

			std::string output;

			if (o_param.empty())
				output = "new.rcf";
			else
				output = o_param;

			std::ofstream oFile(output, std::ofstream::binary);
			rcf_header rcf;
			sprintf(rcf.header, "ATG CORE CEMENT LIBRARY");
			rcf.files = names.size();
			rcf.version = 16777474;
			rcf.data_offset = sizeof(rcf_header);
			rcf.data_size = sizeof(rcf_entry) * names.size();
			rcf.pad = 0;

			// estimate possible names size
			int result = (sizeof(int) * 5) * names.size();
			for (int i = 0; i < names.size(); i++)
				result += names[i].length() - 1;

			// add 2 unk bytes
			result += sizeof(int) * 2;

			rcf.name_offset = calcOffsetFromPad(sizeof(rcf_header) + rcf.data_size, 2048);
			rcf.name_size = result;

			oFile.write((char*)&rcf, sizeof(rcf_header));



			int baseOffset = calcOffsetFromPad(sizeof(rcf_header) + rcf.data_size, 2048) + calcOffsetFromPad(rcf.name_size,2048);
			for (int i = 0; i < names.size(); i++)
			{
				rcf_entry ent;
				ent.offset = baseOffset;
				ent.size = calcOffsetFromPad(sizes[i],2048);
				ent.crc = crc[i];
				oFile.write((char*)&ent, sizeof(rcf_entry));
				baseOffset += calcOffsetFromPad(sizes[i], 2048);
			}

			std::unique_ptr<char[]> pad = std::make_unique<char[]>(calcOffsetFromPad(sizeof(rcf_header) + rcf.data_size, 2048) - (sizeof(rcf_header) + rcf.data_size));
			oFile.write(pad.get(), calcOffsetFromPad(sizeof(rcf_header) + rcf.data_size, 2048) - (sizeof(rcf_header) + rcf.data_size));

			int unkValues[2] = { 2048, 0 };
			oFile.write((char*)&unkValues, sizeof(int) * 2);

			for (int i = 0; i < names.size(); i++)
			{ 
				int stuff[3] = {0,2048, 0 };
				oFile.write((char*)&stuff, sizeof(int) * 3);
				int len = names[i].length();
				oFile.write((char*)&len, sizeof(int));
				oFile.write(names[i].c_str(), len - 1);
				len = 0;
				oFile.write((char*)&len, sizeof(int));
			}
			int calc = calcOffsetFromPad(sizeof(rcf_header) + rcf.data_size + rcf.name_size, 2048) - (rcf.name_size);
			std::unique_ptr<char[]>newpad = std::make_unique<char[]>(calc);
			oFile.write(newpad.get(),calc);


			for (int i = 0; i < names.size(); i++)
			{
				std::ifstream pFile(paths[i], std::ifstream::binary);
				if (!pFile)
				{
					std::cout << "ERROR: Failed to open " << names[i] << std::endl;
					return 1;
				}

				int dataSize = calcOffsetFromPad(sizes[i], 2048);
				std::unique_ptr<char[]> dataBuff = std::make_unique<char[]>(dataSize);

				std::cout << "Processing: " << names[i] << std::endl;
				pFile.read(dataBuff.get(), dataSize);
				oFile.write(dataBuff.get(), dataSize);
			}
			std::cout << "Finished." << std::endl;
		}
	}

}

