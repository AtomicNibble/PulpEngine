
function LogError(fmt, ...)
	Core.Error(string.format(fmt, ...));
end


function LogWarning(fmt, ...)
	Core.Warning(string.format(fmt, ...));
end


function Log(fmt, ...)
	Core.Log(string.format(fmt, ...));
end


function count(_tbl)
	local count = 0;
	if (_tbl) then
		for i,v in pairs(_tbl) do
			count = count+1;
		end
	end	
	return count;
end
