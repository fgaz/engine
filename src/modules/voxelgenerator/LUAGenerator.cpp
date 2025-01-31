/**
 * @file
 */

#include "LUAGenerator.h"
#include "commonlua/LUAFunctions.h"
#include "core/StringUtil.h"
#include "lauxlib.h"
#include "lua.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "commonlua/LUA.h"
#include "voxel/Voxel.h"
#include "core/Color.h"
#include "io/Filesystem.h"
#include "noise/Simplex.h"
#include "app/App.h"

#define GENERATOR_LUA_SANTITY 1

namespace voxelgenerator {

static const char *luaVoxel_metavolumewrapper() {
	return "__meta_volumewrapper";
}

static const char *luaVoxel_metapalette() {
	return "__meta_palette";
}

static const char *luaVoxel_metanoise() {
	return "__meta_noise";
}

static voxel::RawVolumeWrapper* luaVoxel_tovolumewrapper(lua_State* s, int n) {
	return *(voxel::RawVolumeWrapper**)clua_getudata<voxel::RawVolume*>(s, n, luaVoxel_metavolumewrapper());
}

static int luaVoxel_pushvolumewrapper(lua_State* s, voxel::RawVolumeWrapper* volume) {
	if (volume == nullptr) {
		return clua_error(s, "No volume given - can't push");
	}
	return clua_pushudata(s, volume, luaVoxel_metavolumewrapper());
}

static int luaVoxel_volumewrapper_voxel(lua_State* s) {
	const voxel::RawVolumeWrapper* volume = luaVoxel_tovolumewrapper(s, 1);
	const int x = luaL_checkinteger(s, 2);
	const int y = luaL_checkinteger(s, 3);
	const int z = luaL_checkinteger(s, 4);
	const voxel::Voxel& voxel = volume->voxel(x, y, z);
	if (voxel::isAir(voxel.getMaterial())) {
		lua_pushinteger(s, -1);
	} else {
		lua_pushinteger(s, voxel.getColor());
	}
	return 1;
}

static int luaVoxel_volumewrapper_region(lua_State* s) {
	const voxel::RawVolumeWrapper* volume = luaVoxel_tovolumewrapper(s, 1);
	return LUAGenerator::luaVoxel_pushregion(s, &volume->region());
}

static int luaVoxel_volumewrapper_setvoxel(lua_State* s) {
	voxel::RawVolumeWrapper* volume = luaVoxel_tovolumewrapper(s, 1);
	const int x = luaL_checkinteger(s, 2);
	const int y = luaL_checkinteger(s, 3);
	const int z = luaL_checkinteger(s, 4);
	const int color = luaL_checkinteger(s, 5);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, color);
	const bool insideRegion = volume->setVoxel(x, y, z, voxel);
	lua_pushboolean(s, insideRegion ? 1 : 0);
	return 1;
}

static int luaVoxel_palette_colors(lua_State* s) {
	const voxel::MaterialColorArray& colors = voxel::getMaterialColors();
	lua_createtable(s, colors.size(), 0);
	for (size_t i = 0; i < colors.size(); ++i) {
		const glm::vec4& c = colors[i];
		lua_pushinteger(s, i + 1);
		clua_push(s, c);
		lua_settable(s, -3);
	}
	return 1;
}

static int luaVoxel_palette_color(lua_State* s) {
	const uint8_t color = luaL_checkinteger(s, 1);
	const glm::vec4& rgba = voxel::getMaterialColor(voxel::createVoxel(voxel::VoxelType::Generic, color));
	return clua_push(s, rgba);
}

static int luaVoxel_palette_closestmatch(lua_State* s) {
	const voxel::MaterialColorArray& materialColors = voxel::getMaterialColors();
	const float r = luaL_checkinteger(s, 1) / 255.0f;
	const float g = luaL_checkinteger(s, 2) / 255.0f;
	const float b = luaL_checkinteger(s, 3) / 255.0f;
	const int match = core::Color::getClosestMatch(glm::vec4(r, b, g, 1.0f), materialColors);
	if (match < 0 || match > (int)materialColors.size()) {
		return clua_error(s, "Given color index is not valid or palette is not loaded");
	}
	lua_pushinteger(s, match);
	return 1;
}

static int luaVoxel_palette_similar(lua_State* s) {
	const int paletteIndex = lua_tointeger(s, 1);
	const int colorCount = lua_tointeger(s, 2);
	voxel::MaterialColorArray colors = voxel::getMaterialColors();
	if (paletteIndex < 0 || paletteIndex >= (int)colors.size()) {
		return clua_error(s, "Palette index out of bounds");
	}
	const glm::vec4 color = colors[paletteIndex];
	voxel::MaterialColorIndices newColorIndices;
	newColorIndices.resize(colorCount);
	int maxColorIndices = 0;
	colors.erase(paletteIndex);
	for (; maxColorIndices < colorCount; ++maxColorIndices) {
		const int index = core::Color::getClosestMatch(color, colors);
		if (index <= 0) {
			break;
		}
		const glm::vec4& c = colors[index];
		const int materialIndex = core::Color::getClosestMatch(c, voxel::getMaterialColors());
		colors.erase(index);
		newColorIndices[maxColorIndices] = materialIndex;
	}
	if (maxColorIndices <= 0) {
		lua_pushnil(s);
		return 1;
	}

	lua_createtable(s, newColorIndices.size(), 0);
	for (size_t i = 0; i < newColorIndices.size(); ++i) {
		lua_pushinteger(s, i + 1);
		lua_pushinteger(s, newColorIndices[i]);
		lua_settable(s, -3);
	}

	return 1;
}

static int luaVoxel_region_width(lua_State* s) {
	const voxel::Region* region = LUAGenerator::luaVoxel_toRegion(s, 1);
	lua_pushinteger(s, region->getWidthInVoxels());
	return 1;
}

static int luaVoxel_region_height(lua_State* s) {
	const voxel::Region* region = LUAGenerator::luaVoxel_toRegion(s, 1);
	lua_pushinteger(s, region->getHeightInVoxels());
	return 1;
}

static int luaVoxel_region_depth(lua_State* s) {
	const voxel::Region* region = LUAGenerator::luaVoxel_toRegion(s, 1);
	lua_pushinteger(s, region->getDepthInVoxels());
	return 1;
}

static int luaVoxel_region_x(lua_State* s) {
	const voxel::Region* region = LUAGenerator::luaVoxel_toRegion(s, 1);
	lua_pushinteger(s, region->getLowerX());
	return 1;
}

static int luaVoxel_region_y(lua_State* s) {
	const voxel::Region* region = LUAGenerator::luaVoxel_toRegion(s, 1);
	lua_pushinteger(s, region->getLowerY());
	return 1;
}

static int luaVoxel_region_z(lua_State* s) {
	const voxel::Region* region = LUAGenerator::luaVoxel_toRegion(s, 1);
	lua_pushinteger(s, region->getLowerZ());
	return 1;
}

static int luaVoxel_region_mins(lua_State* s) {
	const voxel::Region* region = LUAGenerator::luaVoxel_toRegion(s, 1);
	clua_push(s, region->getLowerCorner());
	return 1;
}

static int luaVoxel_region_maxs(lua_State* s) {
	const voxel::Region* region = LUAGenerator::luaVoxel_toRegion(s, 1);
	clua_push(s, region->getUpperCorner());
	return 1;
}

static int luaVoxel_region_setmins(lua_State* s) {
	voxel::Region* region = LUAGenerator::luaVoxel_toRegion(s, 1);
	const glm::ivec3& mins = clua_tovec<glm::ivec3>(s, 2);
	region->setLowerCorner(mins);
	return 0;
}

static int luaVoxel_region_setmaxs(lua_State* s) {
	voxel::Region* region = LUAGenerator::luaVoxel_toRegion(s, 1);
	const glm::ivec3& maxs = clua_tovec<glm::ivec3>(s, 2);
	region->setUpperCorner(maxs);
	return 0;
}

static int luaVoxel_region_tostring(lua_State *s) {
	const voxel::Region* region = LUAGenerator::luaVoxel_toRegion(s, 1);
	const glm::ivec3& mins = region->getLowerCorner();
	const glm::ivec3& maxs = region->getUpperCorner();
	lua_pushfstring(s, "region: [%i:%i:%i]/[%i:%i:%i]", mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
	return 1;
}

static int luaVoxel_noise_simplex2(lua_State* s) {
	lua_pushnumber(s, noise::noise(clua_tovec<glm::vec2>(s, 1)));
	return 1;
}

static int luaVoxel_noise_simplex3(lua_State* s) {
	lua_pushnumber(s, noise::noise(clua_tovec<glm::vec3>(s, 1)));
	return 1;
}

static int luaVoxel_noise_simplex4(lua_State* s) {
	lua_pushnumber(s, noise::noise(clua_tovec<glm::vec4>(s, 1)));
	return 1;
}

static int luaVoxel_noise_fBm2(lua_State* s) {
	lua_pushnumber(s, noise::fBm(clua_tovec<glm::vec2>(s, 1)));
	return 1;
}

static int luaVoxel_noise_fBm3(lua_State* s) {
	lua_pushnumber(s, noise::fBm(clua_tovec<glm::vec3>(s, 1)));
	return 1;
}

static int luaVoxel_noise_fBm4(lua_State* s) {
	lua_pushnumber(s, noise::fBm(clua_tovec<glm::vec4>(s, 1)));
	return 1;
}

static int luaVoxel_noise_ridgedMF2(lua_State* s) {
	const glm::vec2 v = clua_tovec<glm::vec2>(s, 1);
	const float ridgeOffset = luaL_optnumber(s, 2, 1.0f);
	const uint8_t octaves = luaL_optinteger(s, 3, 4);
	const float lacunarity = luaL_optnumber(s, 4, 2.0f);
	const float gain = luaL_optnumber(s, 5, 0.5f);
	lua_pushnumber(s, noise::ridgedMF(v, ridgeOffset, octaves, lacunarity, gain));
	return 1;
}

static int luaVoxel_noise_ridgedMF3(lua_State* s) {
	const glm::vec3 v = clua_tovec<glm::vec3>(s, 1);
	const float ridgeOffset = luaL_optnumber(s, 2, 1.0f);
	const uint8_t octaves = luaL_optinteger(s, 3, 4);
	const float lacunarity = luaL_optnumber(s, 4, 2.0f);
	const float gain = luaL_optnumber(s, 5, 0.5f);
	lua_pushnumber(s, noise::ridgedMF(v, ridgeOffset, octaves, lacunarity, gain));
	return 1;
}

static int luaVoxel_noise_ridgedMF4(lua_State* s) {
	const glm::vec4 v = clua_tovec<glm::vec4>(s, 1);
	const float ridgeOffset = luaL_optnumber(s, 2, 1.0f);
	const uint8_t octaves = luaL_optinteger(s, 3, 4);
	const float lacunarity = luaL_optnumber(s, 4, 2.0f);
	const float gain = luaL_optnumber(s, 5, 0.5f);
	lua_pushnumber(s, noise::ridgedMF(v, ridgeOffset, octaves, lacunarity, gain));
	return 1;
}

static int luaVoxel_noise_worley2(lua_State* s) {
	lua_pushnumber(s, noise::worleyNoise(clua_tovec<glm::vec2>(s, 1)));
	return 1;
}

static int luaVoxel_noise_worley3(lua_State* s) {
	lua_pushnumber(s, noise::worleyNoise(clua_tovec<glm::vec3>(s, 1)));
	return 1;
}

static void prepareState(lua_State* s) {
	static const luaL_Reg volumeFuncs[] = {
		{"voxel", luaVoxel_volumewrapper_voxel},
		{"region", luaVoxel_volumewrapper_region},
		{"setVoxel", luaVoxel_volumewrapper_setvoxel},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, volumeFuncs, luaVoxel_metavolumewrapper());

	static const luaL_Reg regionFuncs[] = {
		{"width", luaVoxel_region_width},
		{"height", luaVoxel_region_height},
		{"depth", luaVoxel_region_depth},
		// TODO: change region
		{"x", luaVoxel_region_x},
		{"y", luaVoxel_region_y},
		{"z", luaVoxel_region_z},
		{"mins", luaVoxel_region_mins},
		{"maxs", luaVoxel_region_maxs},
		{"setMins", luaVoxel_region_setmins},
		{"setMaxs", luaVoxel_region_setmaxs},
		{"__tostring", luaVoxel_region_tostring},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, regionFuncs, LUAGenerator::luaVoxel_metaregion());

	static const luaL_Reg paletteFuncs[] = {
		{"colors", luaVoxel_palette_colors},
		{"color", luaVoxel_palette_color},
		{"match", luaVoxel_palette_closestmatch},
		{"similar", luaVoxel_palette_similar},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, paletteFuncs, luaVoxel_metapalette(), "palette");

	static const luaL_Reg noiseFuncs[] = {
		{"noise2", luaVoxel_noise_simplex2},
		{"noise3", luaVoxel_noise_simplex3},
		{"noise4", luaVoxel_noise_simplex4},
		{"fBm2", luaVoxel_noise_fBm2},
		{"fBm3", luaVoxel_noise_fBm3},
		{"fBm4", luaVoxel_noise_fBm4},
		{"ridgedMF2", luaVoxel_noise_ridgedMF2},
		{"ridgedMF3", luaVoxel_noise_ridgedMF3},
		{"ridgedMF4", luaVoxel_noise_ridgedMF4},
		{"worley2", luaVoxel_noise_worley2},
		{"worley3", luaVoxel_noise_worley3},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, noiseFuncs, luaVoxel_metanoise(), "noise");

	clua_mathregister(s);
}

bool LUAGenerator::init() {
	return true;
}

void LUAGenerator::shutdown() {
}

bool LUAGenerator::argumentInfo(const core::String& luaScript, core::DynamicArray<LUAParameterDescription>& params) {
	lua::LUA lua;

	// load and run once to initialize the global variables
	if (luaL_dostring(lua, luaScript.c_str())) {
		Log::error("%s", lua_tostring(lua, -1));
		return false;
	}

	const int preTop = lua_gettop(lua);

	// get help method
	lua_getglobal(lua, "arguments");
	if (!lua_isfunction(lua, -1)) {
		// this is no error - just no parameters are needed...
		return true;
	}

	const int error = lua_pcall(lua, 0, LUA_MULTRET, 0);
	if (error != LUA_OK) {
		Log::error("LUA generate arguments script: %s", lua_isstring(lua, -1) ? lua_tostring(lua, -1) : "Unknown Error");
		return false;
	}

	const int top = lua_gettop(lua);
	if (top <= preTop) {
		return true;
	}

	if (!lua_istable(lua, -1)) {
		Log::error("Expected to get a table return value");
		return false;
	}

	const int args = lua_rawlen(lua, -1);

	for (int i = 0; i < args; ++i) {
		lua_pushinteger(lua, i + 1); // lua starts at 1
		lua_gettable(lua, -2);
		if (!lua_istable(lua, -1)) {
			Log::error("Expected to return tables of { name = 'name', desc = 'description', type = 'int' } at %i", i);
			return false;
		}

		core::String name = "";
		core::String description = "";
		core::String defaultValue = "";
		core::String enumValues = "";
		double minValue = 0.0;
		double maxValue = 100.0;
		LUAParameterType type = LUAParameterType::Max;
		lua_pushnil(lua);					// push nil, so lua_next removes it from stack and puts (k, v) on stack
		while (lua_next(lua, -2) != 0) {	// -2, because we have table at -1
			if (!lua_isstring(lua, -1) || !lua_isstring(lua, -2)) {
				Log::error("Expected to find string as parameter key and value");
				// only store stuff with string key and value
				return false;
			}
			const char *key = lua_tostring(lua, -2);
			const char *value = lua_tostring(lua, -1);
			if (!SDL_strcmp(key, "name")) {
				name = value;
			} else if (!SDL_strncmp(key, "desc", 4)) {
				description = value;
			} else if (!SDL_strncmp(key, "enum", 4)) {
				enumValues = value;
			} else if (!SDL_strcmp(key, "default")) {
				defaultValue = value;
			} else if (!SDL_strcmp(key, "min")) {
				minValue = SDL_atof(value);
			} else if (!SDL_strcmp(key, "max")) {
				maxValue = SDL_atof(value);
			} else if (!SDL_strcmp(key, "type")) {
				if (!SDL_strcmp(value, "int")) {
					type = LUAParameterType::Integer;
				} else if (!SDL_strcmp(value, "float")) {
					type = LUAParameterType::Float;
				} else if (!SDL_strcmp(value, "colorindex")) {
					type = LUAParameterType::ColorIndex;
				} else if (!SDL_strncmp(value, "str", 3)) {
					type = LUAParameterType::String;
				} else if (!SDL_strncmp(value, "enum", 4)) {
					type = LUAParameterType::Enum;
				} else if (!SDL_strncmp(value, "bool", 4)) {
					type = LUAParameterType::Boolean;
				} else {
					Log::error("Invalid type found: %s", value);
					return false;
				}
			} else {
				Log::warn("Invalid key found: %s", key);
			}
			lua_pop(lua, 1); // remove value, keep key for lua_next
		}

		if (name.empty()) {
			Log::error("No name = 'myname' key given");
			return false;
		}

		if (type == LUAParameterType::Max) {
			Log::error("No type = 'int', 'float', 'str', 'bool', 'enum' or 'colorindex' key given for '%s'", name.c_str());
			return false;
		}

		if (type == LUAParameterType::Enum && enumValues.empty()) {
			Log::error("No enum property given for argument '%s', but type is 'enum'", name.c_str());
			return false;
		}

		params.emplace_back(name, description, defaultValue, enumValues, minValue, maxValue, type);
		lua_pop(lua, 1); // remove table
	}
	return true;
}

static bool luaVoxel_pushargs(lua_State* s, const core::DynamicArray<core::String>& args, const core::DynamicArray<LUAParameterDescription>& argsInfo) {
	for (size_t i = 0u; i < argsInfo.size(); ++i) {
		const LUAParameterDescription &d = argsInfo[i];
		const core::String &arg = args.size() > i ? args[i] : d.defaultValue;
		switch (d.type) {
		case LUAParameterType::Enum:
		case LUAParameterType::String:
			lua_pushstring(s, arg.c_str());
			break;
		case LUAParameterType::Boolean: {
			const bool val = arg == "1" || arg == "true";
			lua_pushboolean(s, val ? 1 : 0);
			break;
		}
		case LUAParameterType::ColorIndex:
		case LUAParameterType::Integer:
			lua_pushinteger(s, glm::clamp(core::string::toInt(arg), (int)d.minValue, (int)d.maxValue));
			break;
		case LUAParameterType::Float:
			lua_pushnumber(s, glm::clamp(core::string::toFloat(arg), (float)d.minValue, (float)d.maxValue));
			break;
		case LUAParameterType::Max:
			Log::error("Invalid argument type");
			return false;
		}
	}
	return true;
}

voxel::Region* LUAGenerator::luaVoxel_toRegion(lua_State* s, int n) {
	return *(voxel::Region**)clua_getudata<voxel::Region*>(s, n, LUAGenerator::luaVoxel_metaregion());
}

int LUAGenerator::luaVoxel_pushregion(lua_State* s, const voxel::Region* region) {
	if (region == nullptr) {
		return clua_error(s, "No region given - can't push");
	}
	return clua_pushudata(s, region, LUAGenerator::luaVoxel_metaregion());
}

core::String LUAGenerator::load(const core::String& scriptName) const {
	core::String filename = scriptName;
	io::normalizePath(filename);
	if (!core::string::endsWith(filename, ".lua")) {
		filename.append(".lua");
	}
	if (!filename.contains("/")) {
		filename = "scripts/" + filename;
	}
	return io::filesystem()->load(filename);
}

core::DynamicArray<LUAScript> LUAGenerator::listScripts() const {
	lua::LUA lua;
	core::DynamicArray<LUAScript> scripts;
	core::DynamicArray<io::Filesystem::DirEntry> entities;
	io::filesystem()->list("scripts", entities, "*.lua");
	scripts.reserve(entities.size());
	for (const auto& e : entities) {
		const core::String path = "scripts/" + e.name;
		lua.load(io::filesystem()->load(path));
		lua_getglobal(lua, "main");
		if (!lua_isfunction(lua, -1)) {
			Log::debug("No main() function found in %s", path.c_str());
			scripts.push_back({e.name, false});
			continue;
		}
		scripts.push_back({e.name, true});
	}
	return scripts;
}

bool LUAGenerator::exec(const core::String& luaScript, voxel::RawVolumeWrapper* volume, const voxel::Region& region, const voxel::Voxel& voxel, const core::DynamicArray<core::String>& args) {
	core::DynamicArray<LUAParameterDescription> argsInfo;
	if (!argumentInfo(luaScript, argsInfo)) {
		Log::error("Failed to get argument details");
		return false;
	}

	if (!args.empty() && args[0] == "help") {
		Log::info("Parameter description");
		for (const auto& e : argsInfo) {
			Log::info(" %s: %s (default: '%s')", e.name.c_str(), e.description.c_str(), e.defaultValue.c_str());
		}
		return true;
	}

	lua::LUA lua;
	prepareState(lua);
	initializeCustomState(lua);

	// load and run once to initialize the global variables
	if (luaL_dostring(lua, luaScript.c_str())) {
		Log::error("%s", lua_tostring(lua, -1));
		return false;
	}

	// get main(volume, region) method
	lua_getglobal(lua, "main");
	if (!lua_isfunction(lua, -1)) {
		Log::error("LUA generator: no main(volume, region, color) function found in '%s'", luaScript.c_str());
		return false;
	}

	// first parameter is volume
	if (luaVoxel_pushvolumewrapper(lua, volume) == 0) {
		Log::error("Failed to push volume");
		return false;
	}

	// second parameter is the region to operate on
	if (luaVoxel_pushregion(lua, &region) == 0) {
		Log::error("Failed to push region");
		return false;
	}

	// third parameter is the current color
	lua_pushinteger(lua, voxel.getColor());

#if GENERATOR_LUA_SANTITY > 0
	if (!lua_isfunction(lua, -4)) {
		Log::error("LUA generate: expected to find the main function");
		return false;
	}
	if (!lua_isuserdata(lua, -3)) {
		Log::error("LUA generate: expected to find volume");
		return false;
	}
	if (!lua_isuserdata(lua, -2)) {
		Log::error("LUA generate: expected to find region");
		return false;
	}
	if (!lua_isnumber(lua, -1)) {
		Log::error("LUA generate: expected to find color");
		return false;
	}
#endif

	if (!luaVoxel_pushargs(lua, args, argsInfo)) {
		Log::error("Failed to execute main() function with the given number of arguments. Try calling with 'help' as parameter");
		return false;
	}

	const int error = lua_pcall(lua, 3 + argsInfo.size(), 0, 0);
	if (error != LUA_OK) {
		Log::error("LUA generate script: %s", lua_isstring(lua, -1) ? lua_tostring(lua, -1) : "Unknown Error");
		return false;
	}

	return true;
}

}

#undef GENERATOR_LUA_SANTITY
