/*  TA3D, a remake of Total Annihilation
    Copyright (C) 2005  Roland BROCHARD

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA*/

#include "logs.h"
#include <iostream>
#include <ctime>

namespace TA3D
{

	// The unique instance of the ta3d logger
	MyLogger logs;

	void MyLogger::Log(const String& msg)
	{
		std::lock_guard<std::mutex> lock(mutex);

		// example: 2017-01-02 20:31:00 (length 19 + null terminator)
		time_t raw_time = time(NULL);
		struct tm* timeinfo = localtime(&raw_time);
		char current_time[20];
		strftime(current_time, 20, "%F %T", timeinfo);

		std::cout << "[" << current_time << "]" << msg << std::endl;
		log_file << "[" << current_time << "]" << msg << std::endl;
	}

	bool MyLogger::logFileIsOpened() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return log_file.is_open();
	}

	const String& MyLogger::logfile() const
	{
		std::lock_guard<std::mutex> lock(mutex);
		return log_file_name;
	}

	void MyLogger::Reset(const String& filename)
	{
		std::lock_guard<std::mutex> lock(mutex);
		log_file_name = filename;
		log_file = std::ofstream(filename.c_str(), std::ios::trunc | std::ios::binary);
	}

	MyLogger::MyLogBuffer MyLogger::checkpoint()
	{
		auto buffer = MyLogger::MyLogBuffer(*this);
		buffer << "[checkpoint]";
		return buffer;
	}
	MyLogger::MyLogBuffer MyLogger::info()
	{
		auto buffer = MyLogger::MyLogBuffer(*this);
		buffer << "[info]";
		return buffer;
	}
	MyLogger::MyLogBuffer MyLogger::notice()
	{
		auto buffer = MyLogger::MyLogBuffer(*this);
		buffer << "[notice]";
		return buffer;
	}
	MyLogger::MyLogBuffer MyLogger::warning()
	{
		auto buffer = MyLogger::MyLogBuffer(*this);
		buffer << "[warning]";
		return buffer;
	}
	MyLogger::MyLogBuffer MyLogger::error()
	{
		auto buffer = MyLogger::MyLogBuffer(*this);
		buffer << "[error]";
		return buffer;
	}
	MyLogger::MyLogBuffer MyLogger::progress()
	{
		auto buffer = MyLogger::MyLogBuffer(*this);
		buffer << "[progress]";
		return buffer;
	}
	MyLogger::MyLogBuffer MyLogger::fatal()
	{
		auto buffer = MyLogger::MyLogBuffer(*this);
		buffer << "[fatal]";
		return buffer;
	}
	MyLogger::MyLogBuffer MyLogger::compatibility()
	{
		auto buffer = MyLogger::MyLogBuffer(*this);
		buffer << "[compatibility]";
		return buffer;
	}
	MyLogger::MyLogBuffer MyLogger::debug()
	{
		auto buffer = MyLogger::MyLogBuffer(*this);
		buffer << "[debug]";
		return buffer;
	}

} // namespace TA3D
