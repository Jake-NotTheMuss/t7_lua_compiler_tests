local function local_function_1(a, b, c)
	return a * b + c;
end

local function local_function_2(c, d, e)
	return c / d << e;
end

local t = {};
function t:local_function_4(c, d, e)
	return c / d << e;
end