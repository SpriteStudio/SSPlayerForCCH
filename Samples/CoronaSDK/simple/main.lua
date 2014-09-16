

require('SsPlayer')

require('data.Samples.robo.robo_04')
require('data.Samples.comipo.run')

-- SsPlayerのオブジェクトを作成します
-- Create SsPlayer object.
local ssplayer = SsPlayer.new()

-- 使用するアニメーションを登録します
-- ここでイメージが読み込まれ、内部に保管されます
-- Register animation data.
-- Image is read here, it is stored inside.
ssplayer:addAnimation( robo_04_animation, robo_04_images, "data/Samples/robo/" )
ssplayer:addAnimation( run_animation, run_images, "data/Samples/comipo/" )



-- *** Example 1 ***

-- アニメーションオブジェクトを作成します
-- 返されるオブジェクトは DisplayObject として扱えます
-- Create animation handle object.
-- Returned object can serve as DisplayObject.
local robo = ssplayer:newObject()

robo.x = 150
robo.y = 150

-- 再生するアニメーションを設定します
-- Set animation of played.
robo:setAnimation( robo_04_animation )

-- アニメーションを10回ループ再生（0で無限ループ）
-- Set 10 times loop playback. (Infinite at 0)
robo.loop = 10

-- アニメーションの再生スピード
-- Set speed of playing.
robo.timeScale = 1.0

-- 必要であれば再生終了時のイベントを設定します
-- If necessary, set the events of the play at the end.
local function roboEnd()
	-- オブジェクトの削除は removeSelf() を呼び出します（DisplayObjectと同じ）
	-- Delete the object will call removeSelf(). (Same DisplayObject)
	robo:removeSelf()
	robo = nil
end
robo:addEventListener( "ended", roboEnd )

-- アニメーション再生開始
-- Start played.
robo:play()



-- *** Example 2 ***

-- アニメーションオブジェクト作成時にパラメータの指定も可能です
-- When create an object, specifying the parameter is also possible.
local girl = ssplayer:newObject( run_animation, { loop = 0, timeScale = 1.0 } )
--[[
{}に指定可能なものは次のものです
Can be specified are:
　・isPlaying
　・frame
　・loop
　・timeScale
--]]

girl.y = 450
girl.x = -100
girl.xScale = -1

-- アニメーションオブジェクトの frame プロパティに表示したいフレーム番号を直接指定可能です。
-- The frame property of the object, can be specified directly the frame number you want to view.
-- transitionとの組み合わせも可能です
-- Can also combination of a transition.
transition.to( girl, { time = 2000, x = 800, frame = girl.numFrames - 1 })



