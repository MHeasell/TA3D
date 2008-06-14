/*  TA3D, a remake of Total Annihilation
    Copyright (C) 2006  Roland BROCHARD

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

#pragma once

namespace TA3D
{
	namespace INTERFACES
	{
		class cLogger : protected cCriticalSection, protected IInterface
		{
		private:
			FILE				*m_File;
			bool				m_bNoInterface;

		public:
			cLogger( const String &FileName, bool noInterface = false );
			virtual ~cLogger();

		void LogData( const char *txt );
		void LogData( const std::string &txt );
		FILE *get_log_file();

		private:
			uint32 InterfaceMsg( const lpcImsg msg );



		};
	}
} 
