local modName = Engine.UsingModsInternalName();
local dllName = "./mods/"..modName.."/zone/T7LuaCompiler.dll";

-- Call a C function from the DLL
local function callDLL(fnName)
	local function load()
		local func = require("package").loadlib(dllName, fnName);
		if not func then
			Engine.ComError(Enum.errorCode.ERROR_UI,
			"Unable to call function '"..fnName.."' from DLL '"..dllName.."'");
			return;
		end
		func();
	end
	local s, r = pcall(load);
	if not s and r then
		Engine.ComError(Enum.errorCode.ERROR_UI, tostring(r));
	end
end

-- Execute all compiler tests
function RunLuaCompilerTests()
	callDLL("run_tests");
end

function RunLuaChunkTests()
	callDLL("load_chunks");
end
