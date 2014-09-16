-----------------------------------------------------------------------------------------
--
-- main.lua
--
-----------------------------------------------------------------------------------------
require('SsPlayer')

local ssplayer = SsPlayer.new()


local animefile = "datas.userdata_userdata"

local animedata = require(animefile)
ssplayer:addAnimation( animedata.sample1 , animedata.sample1_images , "datas/" )
ssplayer:addAnimation( animedata.sample2 , animedata.sample2_images , "datas/" )

local anime = ssplayer:newObject()
anime:setAnimation( animedata.sample1 )
anime.x = 270
anime.y = 640

anime.loop = 0			-- アニメーションを無限ループ  Set infinite loop.
anime.timeScale = 1.0	-- アニメーションの再生スピード  Set speed of playing.

anime:play()

 

