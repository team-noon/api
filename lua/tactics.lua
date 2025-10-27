function OnUpdate(dt)
	ball = API.Locate.Ball()
	print("The ball has been spotted by player (", team, player, ") at coordinates (", ball.x, ball.y, ").")
end