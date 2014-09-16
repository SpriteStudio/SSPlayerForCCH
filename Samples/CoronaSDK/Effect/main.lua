-----------------------------------------------------------------------------------------
--
-- main.lua
--
-----------------------------------------------------------------------------------------
require('SsPlayer')
require('datas.ssdatas')
local ui = require("ui")

local screen_w = display.contentWidth
local screen_h = display.contentHeight

local filename_txt = display.newText( "", screen_w / 2 , 64, native.systemFont, 30) 


-- Your code here
-- SsPlayerのオブジェクトを作成します
-- Create SsPlayer object.
local ssplayer = SsPlayer.new()

-- 使用するアニメーションを登録します
-- ここでイメージが読み込まれ内部に保管されます
-- Register animation data.
-- Image is read here, it is stored inside.
-- ssplayer:addAnimation( run_run_animation, run_run_images, "datas/" )

--- 
for i = 1 , anim_group_table.animenum do
	print ( anim_group_table.animes[i].name )
	ssplayer:addAnimation( anim_group_table.animes[i].anime , anim_group_table.animes[i].image, "datas/" )
end

-- アニメーションオブジェクトを作成します
-- 返されるオブジェクトは DisplayObject として扱えます
-- Create animation handle object.
-- Returned object can serve as DisplayObject.
local anime = ssplayer:newObject()

play_index = 1

-- 再生するアニメーションを設定します
-- Set animation of played.
anime:setAnimation( anim_group_table.animes[play_index].anime )
anime.x = 270
anime.y = 640

anime.loop = 0			-- アニメーションを無限ループ  Set infinite loop.
anime.timeScale = 1.0	-- アニメーションの再生スピード  Set speed of playing.

filename_txt.text = anim_group_table.animes[play_index].name
-- アニメーション再生開始
-- Start played.
anime:play()

 
local buttonPressNext = function( event )
	play_index = play_index + 1
	if ( play_index > anim_group_table.animenum ) then
		play_index = 1
	end

	anime:setAnimation( anim_group_table.animes[play_index].anime )
	filename_txt.text = anim_group_table.animes[play_index].name
	anime.x = 270
	anime.y = 640
end

local buttonPressPrev = function( event )
	play_index = play_index - 1

	if ( play_index < 1 ) then
		play_index = anim_group_table.animenum
	end

	anime:setAnimation( anim_group_table.animes[play_index].anime )
	filename_txt.text = anim_group_table.animes[play_index].name
	anime.x = 270
	anime.y = 640

end 

local button1Release = function( event )
        --t:setText( "Button 1 released" )
end
 
 
local buttonHandler = function( event )
        --t:setText( "id = " .. event.id .. ", phase = " .. event.phase )
end
 
local button1 = ui.newButton{
        default = "buttonRed.png",
        over = "buttonRedOver.png",
        onPress = buttonPressNext,
        onRelease = button1Release,
        text = "Next",
        emboss = true
}

local button2 = ui.newButton{
        default = "buttonRed.png",
        over = "buttonRedOver.png",
        onPress = buttonPressPrev,
        onRelease = button1Release,
        text = "Prev",
        emboss = true
}

button1.x = screen_w - (298/2); button1.y = screen_h - (56 / 2)
button2.x = 0 + (298/2); button2.y = screen_h - (56 / 2)



