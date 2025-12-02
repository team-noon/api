function GetDistanceAndAngle(first, second)
	local heightdiff = math.abs(first.y-second.y)
	local widthdiff = math.abs(first.x-second.x)
	local hypotenuse = math.sqrt(widthdiff*widthdiff + heightdiff*heightdiff)
	local direction = math.asin(heightdiff / hypotenuse)
	if (first.x < second.x) then
		direction = math.pi - direction
	end
	if (first.y < second.y) then
		direction = direction * -1
	end
	return hypotenuse, direction
end

function OnUpdate(dt)
	ball = API.Locate.Ball()
	myself = API.Locate.Teammate(player)
	goal = API.Locate.OppGoal()
	distance, direction = GetDistanceAndAngle(ball, myself)
	if (distance < 10) then
		goalDistance, goalDirection = GetDistanceAndAngle(goal, ball)
		API.Robot.Kick(goalDirection, 100)
	else
		API.Robot.Move(direction, 50)
	end
end