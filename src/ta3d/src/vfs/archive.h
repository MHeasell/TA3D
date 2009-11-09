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

/*
**  File: archive.h
** Notes:
**   Zuzuf:  This module implements an abstract class used to manipulate archive in VFS module.
*/

#ifndef __TA3D_UTILS_VFS_ARCHIVE_H__
# define __TA3D_UTILS_VFS_ARCHIVE_H__

# include <misc/string.h>

namespace TA3D
{
    namespace UTILS
    {
        /*! \class Archive
        **
        ** \brief abstract class defining the interface required to manipulate archives
        */
        class Archive
        {
        public:
            class File
            {
            protected:
                String name;
                int priority;
                Archive *parent;
            public:
                virtual ~File() {}
                inline String getName()  const  {   return name;    }
                inline int getPriority() const  {   return priority;    }
                inline void setPriority(int p)  {  priority = p;   }
                inline Archive *getParent() const {   return parent;  }
                byte* read(uint32* file_length = NULL);
                byte* readRange(const uint32 start, const uint32 length, uint32* file_length = NULL);
                bool needsCaching() const;
            };
        protected:
            String  name;
        public:
            virtual ~Archive() {}

            //! \brief returns the name of the opened archive
            inline String getName() {   return name;    }

            /*!
            ** \brief Loads an archive
            */
            virtual void open(const String& filename) = 0;

            /*!
            ** \brief Just close the opened archive
            */
            virtual void close() = 0;

            /*!
            ** \brief Return the list of all files in the archive
            */
            virtual void getFileList(std::list<File*> &lFiles) = 0;

            /*!
            ** \brief
            */
            virtual byte* readFile(const String& filename, uint32* file_length = NULL) = 0;
            virtual byte* readFile(const File *file, uint32* file_length = NULL) = 0;

            /*!
            ** \brief
            ** \param filename
            ** \param start
            ** \param length
            ** \return
            */
            virtual byte* readFileRange(const String& filename, const uint32 start, const uint32 length, uint32 *file_length = NULL) = 0;
            virtual byte* readFileRange(const File *file, const uint32 start, const uint32 length, uint32 *file_length = NULL) = 0;

            /*!
            ** \brief returns true if using the cache is a good idea (real FS will return false)
            ** \return
            */
            virtual bool needsCaching();

        public:
            typedef void (*ArchiveFinder)(String::List &fileList, const String &path);
            typedef Archive* (*ArchiveLoader)(const String &filename);
        private:
            static std::list<ArchiveFinder> *listArchiveFinder;
            static std::list<ArchiveLoader> *listArchiveLoader;
        public:
            static void registerArchiveFinder(ArchiveFinder finder);
            static void registerArchiveLoader(ArchiveLoader loader);
            static Archive *load(const String &filename);
            static void getArchiveList(String::List &fileList, const String &path);
        }; // class Archive

        //! A simple macro to auto register Archive classes functionnalities :) (you don't even need to include the headers \o/)
#define REGISTER_ARCHIVE_TYPE(x) \
        class __class_register_archive__##x \
        {\
         public:\
             inline __class_register_archive__##x() \
            {\
             TA3D::UTILS::Archive::registerArchiveFinder( x::finder );\
             TA3D::UTILS::Archive::registerArchiveLoader( x::loader );\
            }\
        };\
        __class_register_archive__##x __my__##x##__registerer;      // Instantiate an object to have it fully functionnal :)
    } // namespace utils
} // namespace TA3D


#endif // __TA3D_UTILS_VFS_ARCHIVE_H__
