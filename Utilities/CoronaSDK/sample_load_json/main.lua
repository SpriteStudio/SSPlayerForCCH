-----------------------------------------------------------------------------------------
--
-- main.lua
--
-----------------------------------------------------------------------------------------
require('SsPlayer')
json = require('json')

--Jsonファイルを読み込む
local path = system.pathForFile( "datas\\out.json" )
fh, msg = io.open( path, "r")

if fh then
  data = fh:read("*a")  -- ファイル全体を読み込む.
else
  print(msg)
end


local ssplayer = SsPlayer.new()


-- Jsonからデコード
local animedata = json.decode( data )

-- 使用するアニメーションを登録します
ssplayer:addAnimation( animedata.sample1 , animedata.sample1_images , "datas/" )
ssplayer:addAnimation( animedata.sample2 , animedata.sample2_images , "datas/" )

local anime = ssplayer:newObject()

--ユーザーデータコールバックの追加
anime.udatcall = function( arg )
	if arg.ni ~= nil then
		print ( arg.ni )
	end
	if arg.p ~= nil then
		print ( "point = ("..arg.p[1]..","..arg.p[2]..")" )
	end
	if arg.r ~= nil then
		print ( "rect = ("..arg.r[1]..","..arg.r[2]..","..arg.r[3]..","..arg.r[4]..")" )
	end
	if arg.s ~= nil then
		print ( "string = " .. arg.s );
	end
end

anime:setAnimation( animedata.sample1 )
anime.x = 270
anime.y = 640

anime.loop = 0			-- アニメーションを無限ループ  Set infinite loop.
anime.timeScale = 1.0	-- アニメーションの再生スピード  Set speed of playing.

anime:play()

 

