json = require('json')



local animefile = arg[1];
local animedata = require(animefile)

local anime = {}
--anime.anime = animedata._animation;
--anime.image = animedata._images;
--anime.udat = animedata.userdata;

for key, value in pairs(animedata) do

	local idx = string.find(key, "_images")
	if idx ~= nil then
		anime[key] = value;

		local animeSymbol1 = string.sub(key, 1, idx - 1)
		local animeSymbol2 = animeSymbol1 .. "_animation"

		if animedata[animeSymbol1] ~= nil then
			anime[animeSymbol1] = animedata[animeSymbol1];
		elseif animedata[animeSymbol2] ~= nil then
			anime[animeSymbol2] = animedata[animeSymbol2];
		end
	end
end


io.open(arg[2], "w"):write(json.encode(anime) )



