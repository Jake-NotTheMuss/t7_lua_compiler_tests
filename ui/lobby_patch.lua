require("ui.t6.lobby.lobbybase");
require("ui.t6.lobby.lobbymenus");

CoD.LobbyButtons.RUN_COMPILER_TESTS = 
{
    stringRef = "RUN LUA COMPILER TESTS",
    action = function(self, element, controller, param, menu)
        RunLuaCompilerTests();
    end,
    customId = "btnRunLuaCompilerTests",
    starterPack = CoD.LobbyButtons.STARTERPACK_UPGRADE
};

CoD.LobbyButtons.RUN_CHUNK_TESTS =
{
	stringRef = "RUN BYTECODE LOADER TESTS",
	action = function(self, element, controller, param, menu)
		RunLuaChunkTests();
	end,
	customId = "btnRunChunkTests",
	starterPack = CoD.LobbyButtons.STARTERPACK_UPGRADE
};

local function IsGamescomDemo()
	return Dvar.ui_execdemo_gamescom:get()
end

local function IsBetaDemo()
	return Dvar.ui_execdemo_beta:get()
end

local function SetButtonState(button, state)
	if state == nil then
		return 
	elseif state == CoD.LobbyButtons.DISABLED then
		button.disabled = true
	elseif state == CoD.LobbyButtons.HIDDEN then
		button.hidden = true
	end
end


local function AddButton(controller, options, button, isLargeButton)
	button.disabled = false
	button.hidden = false
	button.selected = false
	button.warning = false
	if button.defaultState ~= nil then
		if button.defaultState == CoD.LobbyButtons.DISABLED then
			button.disabled = true
		elseif button.defaultState == CoD.LobbyButtons.HIDDEN then
			button.hidden = true
		end
	end
	if button.disabledFunc ~= nil then
		button.disabled = button.disabledFunc(controller)
	end
	if button.visibleFunc ~= nil then
		button.hidden = not button.visibleFunc(controller)
	end
	if IsBetaDemo() then
		SetButtonState(button, button.demo_beta)
	elseif IsGamescomDemo() then
		SetButtonState(button, button.demo_gamescom)
	end
	if button.hidden then
		return 
	end
	local lobbyNav = LobbyData.GetLobbyNav()
	if button.selectedFunc ~= nil then
		button.selected = button.selectedFunc(button.selectedParam)
	elseif CoD.LobbyMenus.History[lobbyNav] ~= nil then
		button.selected = CoD.LobbyMenus.History[lobbyNav] == button.customId
	end
	if button.newBreadcrumbFunc then
		local newBreadcrumbFuncOrString = button.newBreadcrumbFunc
		if type(newBreadcrumbFuncOrString) == "string" then
			newBreadcrumbFuncOrString = LUI.getTableFromPath(newBreadcrumbFuncOrString)
		end
		if newBreadcrumbFuncOrString then
			button.isBreadcrumbNew = newBreadcrumbFuncOrString(controller)
		end
	end
	if button.warningFunc ~= nil then
		button.warning = button.warningFunc(controller)
	end
	if button.starterPack == CoD.LobbyButtons.STARTERPACK_UPGRADE then
		button.starterPackUpgrade = true
		if IsStarterPack() then
			button.disabled = false
		end
	end
	table.insert(options, {
		optionDisplay = button.stringRef,
		action = button.action,
		param = button.param,
		customId = button.customId,
		isLargeButton = isLargeButton,
		isLastButtonInGroup = false,
		disabled = button.disabled,
		selected = button.selected,
		isBreadcrumbNew = button.isBreadcrumbNew,
		warning = button.warning,
		requiredChunk = button.selectedParam,
		starterPackUpgrade = button.starterPackUpgrade,
		unloadMod = button.unloadMod
	})
end

local function AddLargeButton(controller, options, button)
	AddButton(controller, options, button, true)
end

local function AddSmallButton(controller, options, button)
	AddButton(controller, options, button, false)
end

local function AddSpacer(options)
	if #options > 0 then
		options[#options].isLastButtonInGroup = true
	end
end

function CoD.LobbyMenus.ModeSelect(controller, options, isLeader)
	AddLargeButton(controller, options, CoD.LobbyButtons.RUN_COMPILER_TESTS);
	AddLargeButton(controller, options, CoD.LobbyButtons.RUN_CHUNK_TESTS);
	if Mods_Enabled() and isLeader == 1 then
		AddLargeButton(controller, options, CoD.LobbyButtons.MODS_LOAD);
	end
	AddSmallButton(controller, options, CoD.LobbyButtons.QUIT);
end

local LobbyUITargets = {
	[LobbyData.UITargets.UI_MAIN.id] = CoD.LobbyMenus.ModeSelect,
	[LobbyData.UITargets.UI_MODESELECT.id] = CoD.LobbyMenus.ModeSelect,
	[LobbyData.UITargets.UI_CPLOBBYLANGAME.id] = CoD.LobbyMenus.CPButtonsLAN,
	[LobbyData.UITargets.UI_CPLOBBYLANCUSTOMGAME.id] = CoD.LobbyMenus.CPButtonsLANCUSTOM,
	[LobbyData.UITargets.UI_CPLOBBYONLINE.id] = CoD.LobbyMenus.CPButtonsOnline,
	[LobbyData.UITargets.UI_CPLOBBYONLINEPUBLICGAME.id] = CoD.LobbyMenus.CPButtonsPublicGame,
	[LobbyData.UITargets.UI_CPLOBBYONLINECUSTOMGAME.id] = CoD.LobbyMenus.CPButtonsCustomGame,
	[LobbyData.UITargets.UI_CP2LOBBYLANGAME.id] = CoD.LobbyMenus.CPZMButtonsLAN,
	[LobbyData.UITargets.UI_CP2LOBBYLANCUSTOMGAME.id] = CoD.LobbyMenus.CPButtonsLANCUSTOM,
	[LobbyData.UITargets.UI_CP2LOBBYONLINE.id] = CoD.LobbyMenus.CPZMButtonsOnline,
	[LobbyData.UITargets.UI_CP2LOBBYONLINEPUBLICGAME.id] = CoD.LobbyMenus.CPZMButtonsPublicGame,
	[LobbyData.UITargets.UI_CP2LOBBYONLINECUSTOMGAME.id] = CoD.LobbyMenus.CPButtonsCustomGame,
	[LobbyData.UITargets.UI_DOALOBBYLANGAME.id] = CoD.LobbyMenus.DOAButtonsLAN,
	[LobbyData.UITargets.UI_DOALOBBYONLINE.id] = CoD.LobbyMenus.DOAButtonsOnline,
	[LobbyData.UITargets.UI_DOALOBBYONLINEPUBLICGAME.id] = CoD.LobbyMenus.DOAButtonsPublicGame,
	[LobbyData.UITargets.UI_MPLOBBYLANGAME.id] = CoD.LobbyMenus.MPButtonsLAN,
	[LobbyData.UITargets.UI_MPLOBBYMAIN.id] = CoD.LobbyMenus.MPButtonsMain,
	[LobbyData.UITargets.UI_MPLOBBYONLINE.id] = CoD.LobbyMenus.MPButtonsOnline,
	[LobbyData.UITargets.UI_MPLOBBYONLINEPUBLICGAME.id] = CoD.LobbyMenus.MPButtonsOnlinePublic,
	[LobbyData.UITargets.UI_MPLOBBYONLINEMODGAME.id] = CoD.LobbyMenus.MPButtonsModGame,
	[LobbyData.UITargets.UI_MPLOBBYONLINECUSTOMGAME.id] = CoD.LobbyMenus.MPButtonsCustomGame,
	[LobbyData.UITargets.UI_MPLOBBYONLINEARENA.id] = CoD.LobbyMenus.MPButtonsArena,
	[LobbyData.UITargets.UI_MPLOBBYONLINEARENAGAME.id] = CoD.LobbyMenus.MPButtonsArenaGame,
	[LobbyData.UITargets.UI_FRLOBBYONLINEGAME.id] = CoD.LobbyMenus.FRButtonsOnlineGame,
	[LobbyData.UITargets.UI_FRLOBBYLANGAME.id] = CoD.LobbyMenus.FRButtonsLANGame,
	[LobbyData.UITargets.UI_ZMLOBBYLANGAME.id] = CoD.LobbyMenus.ZMButtonsLAN,
	[LobbyData.UITargets.UI_ZMLOBBYONLINE.id] = CoD.LobbyMenus.ZMButtonsOnline,
	[LobbyData.UITargets.UI_ZMLOBBYONLINEPUBLICGAME.id] = CoD.LobbyMenus.ZMButtonsPublicGame,
	[LobbyData.UITargets.UI_ZMLOBBYONLINECUSTOMGAME.id] = CoD.LobbyMenus.ZMButtonsCustomGame,
	[LobbyData.UITargets.UI_MPLOBBYONLINETHEATER.id] = CoD.LobbyMenus.ButtonsTheaterGame,
	[LobbyData.UITargets.UI_ZMLOBBYONLINETHEATER.id] = CoD.LobbyMenus.ButtonsTheaterGame
}

function CoD.LobbyMenus.AddButtonsForTarget(controller, id)
	local buttonFunc = LobbyUITargets[id]
	local model = nil
	if Engine.IsLobbyActive(Enum.LobbyType.LOBBY_TYPE_GAME) then
		model = Engine.GetModel(DataSources.LobbyRoot.getModel(controller), "gameClient.isLeader")
	else
		model = Engine.GetModel(DataSources.LobbyRoot.getModel(controller), "privateClient.isLeader")
	end
	local isLeader = nil
	if model ~= nil then
		isLeader = Engine.GetModelValue(model)
	else
		isLeader = 1
	end
	local result = {}
	buttonFunc(controller, result, isLeader)
	return result
end
