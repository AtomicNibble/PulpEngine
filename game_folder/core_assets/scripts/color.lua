
local COLOR = {}
COLOR.__index = COLOR

function Color( r, g, b, a )

    a = a or 255
    return setmetatable( { r = math.min( tonumber(r), 255 ), g =  math.min( tonumber(g), 255 ), b =  math.min( tonumber(b), 255 ), a =  math.min( tonumber(a), 255 ) }, COLOR )

end

function ColorAlpha( c, a )
    return Color( c.r, c.g, c.b, a )
end

function IsColor( obj )
    return getmetatable(obj) == COLOR
end


function COLOR:__tostring()
    return string.format( "%d %d %d %d", self.r, self.g, self.b, self.a )
end

function COLOR:__eq( c )
    return self.r == c.r and self.g == c.g and self.b == c.b and self.a == c.a
end
