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

callDLL("init");

-- need to redefine some functions
require("ui.uieditor.actions")

-- when this mod is unloaded, the dll will also be unloaded
function Mods_LoadMod(controller, element)
	callDLL("close")
	local data = {
		ugcName = CoD.SafeGetModelValue(element:getModel(), "ugcName");
		ugcType = LuaEnums.MODS_BASE_PATH,
		ugcVersion = 1
	};
	Engine.LobbyVM_CallFunc("LoadMod", data);
end

function Mods_Unload(controller, element)
	callDLL("close")
	local data = {
		ugcName = "",
		ugcType = LuaEnums.MODS_BASE_PATH,
		ugcVersion = 1
	};
	Engine.LobbyVM_CallFunc("LoadMod", data);
end
