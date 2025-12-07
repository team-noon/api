role = ""

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
			API.Robot.Kick(mateDirection, 100)
		else
			if direction < 0 then
				direction = - (math.pi / 2)
			else
				direction = (math.pi / 2)
			end
			API.Robot.Move(direction, 120)
		end
	elseif (distance < 30.1) then
		if math.random(0, 4) < 4 then
			print(team, "\t", player, "shooting for goal")
			target = API.Locate.OppGoal()
		else
			
			choice = math.random(1, 3)
			if choice == player then
				choice = choice + 1
			end
			if choice > 3 then
				choice = 1
			end
			print(team, "\t", player, "passing to teammate ", choice)
			target = API.Locate.Teammate(choice)
		end
		targetDirection, targetDistance = GetDistanceAndAngle(target, ball)
		API.Robot.Kick(targetDirection, 100)
	else
		API.Robot.Move(direction, 120)
	end
end