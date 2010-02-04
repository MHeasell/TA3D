#include "realfile.h"

namespace TA3D
{
	namespace UTILS
	{
		RealFile::RealFile() : buffer(NULL)
		{
		}

		RealFile::RealFile(const String &filename) : buffer(NULL)
		{
			open(filename);
		}

		RealFile::~RealFile()
		{
			sFile.close();
			if (buffer)
				delete[] buffer;
		}

		void RealFile::open(const String &filename)
		{
			sFile.close();
			if (buffer)
				delete[] buffer;
			buffer = NULL;
			sFile.open(filename, Yuni::Core::IO::File::OpenMode::read);
		}

		bool RealFile::isOpen()
		{
			return sFile.opened();
		}

		bool RealFile::eof()
		{
			return sFile.eof();
		}

		int RealFile::size()
		{
			if (!sFile.opened())
				return 0;
			ssize_t pos = sFile.tell();
			sFile.seekFromEndOfFile(0);
			ssize_t s = sFile.tell();
			sFile.seekFromBeginning(pos);
			return int(s);
		}

		int RealFile::tell()
		{
			return int(sFile.tell());
		}

		void RealFile::seek(int pos)
		{
			sFile.seekFromBeginning(pos);
		}

		int RealFile::read(void *p, int s)
		{
			return int(sFile.read((char*)p, s));
		}

		bool RealFile::readLine(String &line)
		{
			if (sFile.eof())
				return false;

			line.clear();
			for(int c = getc() ; c != 0 && c != 13 && c != 10 && c != -1 ; c = getc())
				line << char(c);

			return true;
		}

		const char *RealFile::data()
		{
			if (buffer)
				return buffer;

			buffer = new char[size()];
			int pos = tell();
			seek(0);
			read(buffer, size());
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
	}
}
