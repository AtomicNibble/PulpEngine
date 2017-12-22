Script.Load("util.lua");

function OnInit()

	Core.Log("Hello world!!");
	Core.Warning("fuck you");
	Core.Error("stu not fnd");
end

function TestFile()

	file = Io.OpenFile("test.dat","w")

	-- writes a string and a number
	file:write("potato", 5)
	-- just a string
	file:write("--test");
	file:write(0.45454);

	file:write("--test");

	-- random number :D
	file:write(math.random())

	Io.CloseFile(file)
end

function TestRead()

	file = Io.OpenFile("test.dat","r")

	local data = file:read("*a")

	file:close()

	Core.Log("File Contents: ^5\"" .. data .. "\"")

end


function TestVector()
	vecTemp1 = vec.new(1,1,1)
	vecTemp2 = vec.new(3,2,1,8) + vecTemp1

	Log( "%s", vecTemp2 );
end




--Game = Game or {}
--Game.__index = Game

OnInit();
TestFile();
TestRead();
---TestVector();