#include "realfile.h"

namespace TA3D
{
	namespace UTILS
	{
		RealFile::RealFile() : buffer(NULL)
		{
		}

		RealFile::RealFile(const String& filename) : buffer(NULL)
		{
			open(filename);
		}

		RealFile::~RealFile()
		{
			sFile.close();
			if (buffer)
				delete[] buffer;
		}

		void RealFile::open(const String& filename)
		{
			sFile.close();
			if (buffer)
				delete[] buffer;
			buffer = NULL;
			sFile.open(filename.c_str(), std::ios::binary);
			realFilename = filename;
		}

		bool RealFile::isOpen()
		{
			return sFile.is_open();
		}

		bool RealFile::eof()
		{
			if (!sFile.is_open())
				return true;
			return sFile.eof();
		}

		int RealFile::size()
		{
			if (!sFile.is_open())
				return 0;
			ssize_t pos = sFile.tellg();
			sFile.seekg(0, sFile.end);
			ssize_t s = sFile.tellg();
			sFile.seekg(pos);
			return int(s);
		}

		int RealFile::tell()
		{
			if (!sFile.is_open())
				return 0;
			return int(sFile.tellg());
		}

		void RealFile::seek(int pos)
		{
			sFile.seekg(pos);
		}

		int RealFile::read(void* p, int s)
		{
			if (!sFile.is_open())
				return 0;
			sFile.read((char*)p, s);
			return sFile.gcount();
		}

		bool RealFile::readLine(String& line)
		{
			if (sFile.eof())
				return false;

			line.clear();
			for (int c = getc(); c != 0 && c != 13 && c != 10 && c != -1; c = getc())
				line << char(c);

			return true;
		}

		const char* RealFile::data()
		{
			if (buffer)
				return buffer;

			buffer = new char[size() + 1];
			int pos = tell();
			seek(0);
			read(buffer, size());
			buffer[size()] = 0;
			seek(pos);

			return buffer;
		}

		void RealFile::close()
		{
			sFile.close();
			if (buffer)
				delete[] buffer;
			buffer = NULL;
		}

		bool RealFile::isReal() const
		{
			return true;
		}

		const String& RealFile::getRealFilename() const
		{
			return realFilename;
		}
	}
}
