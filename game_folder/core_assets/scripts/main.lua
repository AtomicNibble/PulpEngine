script.load("util.lua");

function OnInit()

	core.log("Hello world!!");
	core.warning("test test");
	core.error("lua erorr msg");
end

function TestFile()

	file = io.openFile("test.dat","w")

	-- writes a string and a number
	file:write("potato", 5)
	-- just a string
	file:write("--test");
	file:write(0.45454);

	file:write("--test");

	-- random number :D
	file:write(math.random())

	io.closeFile(file)
end

function TestRead()

	file = io.openFile("test.dat","r")

	local data = file:read("*a")

	file:close()

	core.log("File Contents: ^5\"" .. data .. "\"")

end


function TestVector()
	vecTemp1 = vec.new(1,1,1)
	vecTemp2 = vec.new(3,2,1,8) + vecTemp1

	Log( "%s", vecTemp2 );
end




--Game = Game or {}
--Game.__index = Game

--OnInit();
--TestFile();
--TestRead();
--TestVector();