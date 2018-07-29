
function LogError(fmt, ...)
	core.error(string.format(fmt, ...));
end


function LogWarning(fmt, ...)
	core.warning(string.format(fmt, ...));
end


function Log(fmt, ...)
	core.log(string.format(fmt, ...));
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
