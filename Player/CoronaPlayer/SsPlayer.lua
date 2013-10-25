

local PART_NO = 1
local FLAGS = 2
local SHEET_FRAME_NO = 3
local DST_X = 4
local DST_Y = 5

local ORG_X = 6
local ORG_Y = 7
local ROT = 8
local X_SCALE = 9
local Y_SCALE = 10
local ALPHA = 11
-- additional userdata (This is a trial production. 
local UDAT = 12


local function toInt(n)
	if n >= 0 then
		return math.floor(n)
	else
		n = math.ceil(n)
		if (n == -0) then n = 0 end
		return n
	end
end


------------------------------------------------
-- SsInternalFunc
------------------------------------------------
local SsInternalFunc = {}

function SsInternalFunc.newImageSheets(animation, images, baseDir)
	local imageSheets = {}
	for i, options in ipairs(animation.imageOptions) do
		local m = {}
		if #options.frames >= 1 then
			local filename = baseDir .. images[i]
			local imageSheet = graphics.newImageSheet(filename, options)
			assert(imageSheet ~= nil, "Error: Image load failed '" .. baseDir .. images[i] .. "'")
	
			m.imageSheet = imageSheet
			m.sequenceData = {
				{ name="anim", start=1, count=#options.frames }
			}
		end
		table.insert(imageSheets, m)
	end	
	return imageSheets
end

function SsInternalFunc.setPartParams(part, animation, frame, index,obj)
	local params = animation.ssa[1 + frame][index]
	local sprite = part[1]

	sprite:setFrame( 1 + params[SHEET_FRAME_NO] )

	local flags = params[FLAGS]
	if flags == 1 or flags == 3 then
		sprite.xScale = -1
	else
		sprite.xScale = 1
	end
	if flags == 2 or flags == 3 then
		sprite.yScale = -1
	else
		sprite.yScale = 1
	end

	local orgX = 0
	if #params >= ORG_X then orgX = params[ORG_X] end
	local orgY = 0
	if #params >= ORG_Y then orgY = params[ORG_Y] end
	part.xReference = orgX
	part.yReference = orgY

	local rot = 0
	if #params >= ROT then rot = params[ROT] end
	part.rotation = rot

	local xScale = 1.0
	if #params >= X_SCALE then xScale = params[X_SCALE] end
	part.xScale = xScale

	local yScale = 1.0
	if #params >= Y_SCALE then yScale = params[Y_SCALE] end
	part.yScale = yScale

	local alpha = 1
	if #params >= ALPHA then alpha = params[ALPHA] end
	part.alpha = alpha

	part.x = params[DST_X]
	part.y = params[DST_Y]

	--

	local frame_str =""..frame;

	if ( animation.userdata == nil ) then return end
	local udat = animation.userdata[frame_str];

	if udat ~= nil then
		-- local udat_block = params.udat;
		print("user")
		obj.udatcall( udat )
	end

end


------------------------------------------------
-- SsObjectFrameUpdater
------------------------------------------------
local SsObjectFrameUpdater = {
	players = {}
}

function SsObjectFrameUpdater.addPlayer(player)
	table.insert(SsObjectFrameUpdater.players, player)
end

function SsObjectFrameUpdater.removePlayer(player)
	local index = table.indexOf(SsObjectFrameUpdater.players, player)
	table.remove(SsObjectFrameUpdater.players, index)
end

function SsObjectFrameUpdater.enterFrame(event)
	for i,player in ipairs(SsObjectFrameUpdater.players) do
		player:updateFrame()
	end
end

Runtime:addEventListener("enterFrame", SsObjectFrameUpdater.enterFrame)


------------------------------------------------
-- SsObject
------------------------------------------------
local SsObject = {}
SsObject.new = function(player, animation, initParams)

	local obj = display.newGroup()
	obj.m = {}
	obj.m.player = player
	obj.m.animation = nil
	obj.m.parts = nil

	------------------------------------------------
	-- udatcall()
	------------------------------------------------
	obj.udatcall = function (arg)
	end

	------------------------------------------------
	-- play()
	------------------------------------------------
	obj.play = function(self)
		self.isPlaying = true
	end

	------------------------------------------------
	-- pause()
	------------------------------------------------
	obj.pause = function(self)
		self.isPlaying = false
	end

	------------------------------------------------
	-- isPlaying
	------------------------------------------------
	obj.isPlaying = false

	------------------------------------------------
	-- setFrame(frame)
	------------------------------------------------
	obj.setFrame = function(self, frame)
		self.frame = frame
	end

	------------------------------------------------
	-- frame
	-- current frame no. (from 0)
	------------------------------------------------
	obj.frame = 0

	------------------------------------------------
	-- numFrames (read-only)
	------------------------------------------------
	obj.numFrames = 0

	------------------------------------------------
	-- loop
	------------------------------------------------
	obj.loop = 1
	
	------------------------------------------------
	-- loopCount
	------------------------------------------------
	obj.loopCount = 0

	------------------------------------------------
	-- timeScale
	------------------------------------------------
	obj.timeScale = 1.0

	------------------------------------------------
	-- setAnimation
	------------------------------------------------
	obj.setAnimation = function(self, animation, initParams)
		if animation ~= self.m.animation then
			self:clearAnimation()
		
			local imageSheets = self.m.player:getImageSheets(animation)
			assert(imageSheets ~= nil, "Error: Not registered animation")
		
			local parts = {}
			for i, part in ipairs(animation.parts) do
				local index = 1 + part.imageNo
				local pm = display.newGroup()
				if imageSheets[index].imageSheet ~= nil then
					local ps = display.newSprite( imageSheets[index].imageSheet, imageSheets[index].sequenceData )
					pm:insert(ps)
				end
				pm.isVisible = false;
				table.insert(parts, pm)
			end
		
			self.m.animation = animation
			self.m.parts = parts
		end

		self.frame = 0
		self.numFrames = #animation.ssa
		self.loopCount = 0
		
		if initParams ~= nil then
			if initParams.isPlaying then self.isPlaying = initParams.isPlaying end
			if initParams.frame then self.frame = initParams.frame end
			if initParams.loop then self.loop = initParams.loop end
			if initParams.timeScale then self.timeScale = initParams.timeScale end
		end

		self:setParts(self.frame)
	end

	------------------------------------------------
	-- clearAnimation
	------------------------------------------------
	obj.clearAnimation = function(self)
		if self:hasAnimation() == false then return end
	
		for i = self.numChildren, 1, -1 do
			self:remove(i);
		end
		
		for i = 1, #self.m.parts do
			table.remove(self.m.parts)
		end

		self.m.animation = nil
		self.m.parts = nil
	end

	------------------------------------------------
	-- hasAnimation
	------------------------------------------------
	obj.hasAnimation = function(self)
		return self.m.animation ~= nil
	end



	------------------------------------------------
	-- *** internal functions ***
	------------------------------------------------

	------------------------------------------------
	-- updateFrame()
	------------------------------------------------
	obj.updateFrame = function(self)
		if self:hasAnimation() then
			local event = self:updatePlayingFrame()
			self:setParts(self.frame)
			
			-- dispatch event
			if event ~= nil then
				self:dispatchEvent(event)
			end
		end
	end

	------------------------------------------------
	-- updatePlayingFrame()
	------------------------------------------------
	obj.updatePlayingFrame = function(self)
		if self.isPlaying == false then return nil end

		local event = nil
		
		local coronaFps = 30
		if self.loop == 0 or self.loopCount < self.loop then
			local s = self.m.animation.fps / coronaFps * self.timeScale
			self.frame = self.frame + s
			
			local iFrame = toInt(self.frame)
			local c = toInt(iFrame / self.numFrames)
			
			if self.timeScale >= 0 then
				-- forward play
				if iFrame >= self.numFrames then
					self.loopCount = self.loopCount + c
					if self.loop ~= 0 and self.loopCount >= self.loop then
						-- end looping
						self.frame = self.numFrames - 1
						self.isPlaying = false

						-- ended event
						event = { name = "ended", target = self }
					else
						self.frame = iFrame % self.numFrames
					end
				end
			else
				-- reverse play
				if iFrame < 0 then
					self.loopCount = self.loopCount + (1 + -c)
					if self.loop ~= 0 and self.loopCount >= self.loop then
						-- end looping
						self.frame = 0
						self.isPlaying = false

						-- ended event
						event = { name = "ended", target = self }
					else
						self.frame = iFrame % self.numFrames
						if self.frame < 0 then self.frame = self.frame + self.numFrames end
					end				
				end
			end
		end
		
		return event
	end

	------------------------------------------------
	-- setParts(frame)
	------------------------------------------------
	obj.setParts = function(self, frame)
		-- normalize frame value
		frame = toInt(frame)
		frame = frame % self.numFrames
		if frame < 0 then frame = frame + self.numFrames end

		-- all invisibled once
		for i, part in ipairs(self.m.parts) do
			part.isVisible = false;
		end

		-- set part parameters
		local frameData = self.m.animation.ssa[1 + frame]
		for index = 1, #frameData do
			local partNo = 1 + frameData[index][PART_NO]
			local pm = self.m.parts[partNo]
			
			SsInternalFunc.setPartParams(pm, self.m.animation, frame, index , self)
			pm.isVisible = true;
			self:insert(pm)
		end
	end

	------------------------------------------------
	-- removeSelf()
	------------------------------------------------
	-- safe original removeSelf()
	obj.m.orgRemoveSelf = obj.removeSelf
	-- override removeSelf()
	obj.removeSelf = function(self)
		-- remove frame callback
		SsObjectFrameUpdater.removePlayer(self)
		-- destroy animation
		self:clearAnimation()
		-- call original removeSelf()
		self.removeSelf = self.m.orgRemoveSelf
		self:removeSelf()
	end

	-- add frame callback
	SsObjectFrameUpdater.addPlayer(obj)

	if animation ~= nil then
		obj:setAnimation(animation, initParams)
	end

	return obj
end


------------------------------------------------
-- SsPlayer
------------------------------------------------
SsPlayer = {}
SsPlayer.new = function()

	local obj = {
		animationsImageSheets = {}
	}

	obj.addAnimation = function(self, animation, images, baseDir)
		local imageSheets = SsInternalFunc.newImageSheets(animation, images, baseDir)
		self.animationsImageSheets[animation] = imageSheets
	end
	
	obj.getImageSheets = function(self, animation)
		return self.animationsImageSheets[animation]
	end
	
	obj.newObject = function(self, animation, initParams)
		return SsObject.new(self, animation, initParams)
	end

	return obj
end


