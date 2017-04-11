
#include "cache.h"
#include "misc/string.h"
#include "misc/paths.h"
#include "TA3D_NameSpace.h"
#include <fstream>

namespace TA3D
{
	namespace Cache
	{

		void Clear(const bool force)
		{
			bool rebuild_cache = false;
			// Check cache date
			const String cache_info_data = (lp_CONFIG
												   ? String("build info : ") << __DATE__ << " , " << __TIME__ << "\ncurrent mod : " << lp_CONFIG->last_MOD << '\n'
												   : String("build info : ") << __DATE__ << " , " << __TIME__ << "\ncurrent mod : \n")
				<< "Texture Quality : " << lp_CONFIG->unitTextureQuality;

			String cache_info_filename = String(Paths::Caches) << "cache_info.txt";

			if (Paths::Exists(cache_info_filename) && !force)
			{
				std::ifstream cache_info(cache_info_filename.c_str(), std::ios::binary);
				if (cache_info.is_open())
				{
					char* buf = new char[cache_info_data.size() + 1];
					if (buf)
					{
						::memset(buf, 0, cache_info_data.size() + 1);
						cache_info.read(buf, cache_info_data.size());
						if (buf == cache_info_data)
							rebuild_cache = false;
						else
							rebuild_cache = true;
						DELETE_ARRAY(buf);
					}
					cache_info.close();
				}
			}
			else
				rebuild_cache = true;

			if (lp_CONFIG->developerMode) // Developer mode forces cache refresh
				rebuild_cache = true;

			if (rebuild_cache)
			{
				String::Vector file_list;
				Paths::GlobFiles(file_list, String(Paths::Caches) << "*");
				for (String::Vector::iterator i = file_list.begin(); i != file_list.end(); ++i)
					remove(i->c_str());
				// Update cache date
				std::ofstream cache_info(cache_info_filename.c_str(), std::ios::binary);
				if (cache_info.is_open())
				{
					cache_info.write(cache_info_data.c_str(), cache_info_data.size());
					cache_info.put(0);
					cache_info.close();
				}
			}
		}

	} // namespace Cache
} // namespace TA3D
