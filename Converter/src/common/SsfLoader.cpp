
#include "SsfLoader.h"
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

using namespace ss;
using boost::property_tree::ptree;
using boost::optional;
using boost::shared_ptr;


/**
 * ssfファイルをロード 
 */
ss::SsImageList::Ptr SsfLoader::load(const std::string& filename)
{
	do
	{
		ptree pt;
		read_ini(filename, pt);

		std::vector<SsImage::Ptr> images;
		BOOST_FOREACH( const ptree::value_type& v, pt.get_child("Source") )
		{
			if (v.first == "list_max") continue;

			int id = 0;
			try
			{
				id = boost::lexical_cast<int>(v.first);
			}
			catch (const boost::bad_lexical_cast&)
			{
				continue;
			}

			optional<std::string> path = v.second.get_optional<std::string>("");
			if (path)
			{
				SsImage::Ptr image = SsImage::Ptr(new SsImage(id, path.get(), SsSize(), 0));
				images.push_back(image);
			}
		}

		return SsImageList::Ptr(new SsImageList(images));

	} while (false);

	return SsImageList::Ptr();
}
