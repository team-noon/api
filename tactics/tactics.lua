role = ""
defaultSpeed = 60
defaultKick = 100

function GetDistanceAndAngle(first, second)
	local heightdiff = math.abs(first.y-second.y)
	local widthdiff = math.abs(first.x-second.x)
	local hypotenuse = math.sqrt(widthdiff*widthdiff + heightdiff*heightdiff)
	local direction = math.asin(heightdiff / hypotenuse)
	if (first.x < second.x) then
		direction = math.pi - direction
	end
	if (first.y < second.y) then
		direction = direction + 2*(math.pi - direction)
	end
	return hypotenuse, direction
end

function OnInit()
	if player == 0 then
		role = "keeper"
	else
		role = "striker"
	end
	print(role)
end

function OnUpdate(dt)
	ball = API.Locate.Ball()
	myself = API.Locate.Teammate(player)
	distance, direction = GetDistanceAndAngle(ball, myself)
	if (role == "keeper") then
		if (distance < 30.1) then
			choice = math.random(1, 3)
			mate = API.Locate.Teammate(choice)
			mateDistance, mateDirection = GetDistanceAndAngle(mate, ball)
			API.Robot.Kick(mateDirection, defaultKick)
		else
			if direction < math.pi then
				direction = (math.pi / 2)
			else
				direction = 3*(math.pi / 2)
			end
			API.Robot.Move(direction, defaultSpeed)
		end
	elseif (distance < 30.1) then
		if math.random(0, 4) < 4 then
			target = API.Locate.OppGoal()
			
		else
			choice = math.random(1, 3)
			if choice == player then
				choice = choice + 1
			end
			if choice > 3 then
				choice = 1
			end
			target = API.Locate.Teammate(choice)
		end
		targetDistance, targetDirection = GetDistanceAndAngle(target, ball)
		API.Robot.Kick(targetDirection, defaultKick)
	else
		API.Robot.Move(direction, defaultSpeed)
	end
end