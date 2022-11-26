local a = "a";

t = {};

InheritFrom2 = function(a, b, c) return a *b+c; end

function InheritFrom1(a, b, c) return a*b+c; end

-- yindex
t[a] = function(a, b, c)
	return a * 2 + b << c;
end

local localfunc1=function (a, b, c) return a*b*c; end

-- member method
function t:memberOfT(a, b)
	if self.activeCount then
		return self.activeCount * a;
	else
		return b * 2 * a;
	end
end

-- constant string index
function t.Kstring1(a, b, c)
	return a * 2 << (c + 1) / b;
end

t.Kstring2 = function(a, b, c)
	return a * 2 << (c + 1) / b;
end

local function localfunc2(a, b, c) return a*b*c; end


