role = "" -- The player's role
defaultSpeed = 60	-- sets a default speed
fastSpeed = 75 -- sets a fast speed
defaultKick = 100	-- sets a default kick speed
targetpos = Vec2:New(0, 0)	-- the current target's position


function GetDistanceAndAngle(a, b)	-- a function with two distance arguments
	local lengthdiff = math.abs(a.y-b.y)	-- sets the length difference
	local widthdiff = math.abs(a.x-b.x)		-- sets the width difference
	for i=0,3 do	-- searches for all the opponents
		pos = API.Locate.Opp(i)
		print("The opponents position: ", pos.x, pos.y)
		opp_pos = {pos.x, pos.y}
		
		if i ~= player then		-- if the player is not us
			pos = API.Locate.Teammate(i) -- locate all the teammates
			print("The teammates position: ", pos.x, pos.y)
			tm8_pos = {pos.x, pos.y}
		end
	end

	-- dist = Vec2:New(myself).dist(Vec2:New(ball))
	print(dist(myself, ball))	--print the distance between ourselves and the ball


	local hypotenuse = math.sqrt(widthdiff*widthdiff + lengthdiff*lengthdiff)	-- uses the Pythegorian theorem
	local direction = math.atan(lengthdiff, widthdiff)	-- calculates the direction with an asin function
--[[ 	if (a.x < b.x) then		-- if the ball is on my left
		direction = math.pi - direction		-- corrects the direction
	end
	if (a.y < b.y) then		if the ball is behind me
		direction = direction + 2*(math.pi - direction)		-- corrects the direction
	end ]]
	return hypotenuse, direction
end

function dist(a, b)		-- a distance calculating function
	local lengthdiff = math.abs(a.y-b.y)	-- the difference in length is the subtraction's absolute value
	local widthdiff = math.abs(a.x-b.x)		-- the difference in width is the subtraction's absolute value
	return math.sqrt(widthdiff*widthdiff + lengthdiff*lengthdiff)	-- uses the direction formula
end

function OnInit()
	if player == 0 then		-- if our id is 0
		role = "keeper"		-- we are the goalkeepers
	else
		role = "striker"		-- we are the strikers
	end
	print(role)		-- print the player's role
end

bm_dis_ext = bm_distance + 10
om_dis_ext = om_distance + 10
tm_dis_ext = tm_distance + 10
ob_dis_ext = ob_distance + 10
tb_dis_ext = tb_distance + 10
to_dis_ext = to_distance + 10
circle_radius = 200

function OnUpdate(dt)
	ball = API.Locate.Ball()		-- initializing the balL's position
	myself = API.Locate.Teammate(player)	-- defines the position of ourselves

	bm_distance, bm_direction = GetDistanceAndAngle(ball, myself)
	om_distance, om_direction = GetDistanceAndAngle(opp_pos, myself)
	tm_distance, tm_direction = GetDistanceAndAngle(tm8_pos, myself)
	ob_distance, ob_direction = GetDistanceAndAngle(opp_pos, ball)
	tb_distance, tb_direction = GetDistanceAndAngle(tm8_pos, ball)
	to_distance, to_direction = GetDistanceAndAngle(tm8_pos, opp_pos)
	def_mes = true
	strikerpos = {0, 0}
	defenderpos = {0, 0}
	keeperpos = {0, 0}
	defendid = ""

	if role == "striker" && bm_distance < 50 && ob_distance > bm_dis_ext then
		API.Robot.Move(bm_direction, fastSpeed)
		API.Robot.Kick(tm_direction, 80)
	end

	elseif role == "striker" && ob_distance <= bm_dis_ext then 
		if def_mes then
			if role == "defender" && 250 <= strikerpos.y <= 350 then
				if defendid = "left" then
					API.Robot.Move(strikerpos.x - 75, defaultSpeed)
					API.Robot.Move(strikerpos.y - 75, defaultSpeed)
				end
				if defendid = "left" then
					API.Robot.Move(strikerpos.x - 75, defaultSpeed)
					API.Robot.Move(strikerpos.y + 75, defaultSpeed)
				end
			elseif role == "defender" && 0 < strikerpos < 250 then
				API.Robot.Move(strikerpos.x - 75, defaultSpeed)
				API.Robot.Move(strikerpos.y + 75, defaultSpeed)
			end
			elseif role == "defender" && 350 < strikerpos < 600 then
				API.Robot.Move(strikerpos.x - 75, defaultSpeed)
				API.Robot.Move(strikerpos.y - 75, defaultSpeed)
			end
			end
		end
	else:
		API.Robot.Move(bm_direction, defaultSpeed)
	end

	elseif role == "keeper" && bm_distance <= 400 then 
		API.Robot.Move(bm_direction, defaultSpeed)
		API.Robot.Kick(tm_direction, 120)
	end
	
end