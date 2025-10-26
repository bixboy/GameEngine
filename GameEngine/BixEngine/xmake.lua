set_xmakever("3.0.4")
set_project("BixEngine")

add_rules("mode.debug", "mode.release")
add_moduledirs(path.join(os.projectdir(), "tools/xmake"))

set_languages("cxx20")
set_warnings("allextra")
set_optimize("faster")

option("sdl3_dir")
    set_showmenu(true)
    set_description("Chemin vers le répertoire include de SDL3")
    set_default(os.getenv("SDL3_DIR") or "")
option_end()

option("sdl3_lib_dir")
    set_showmenu(true)
    set_description("Chemin vers le répertoire lib de SDL3")
    set_default(os.getenv("SDL3_LIB_DIR") or "")
option_end()

local sdl3_inc = get_config("sdl3_dir") or ""
local sdl3_lib = get_config("sdl3_lib_dir") or ""

if (sdl3_inc == nil or sdl3_inc == "") and (sdl3_lib == nil or sdl3_lib == "") then
    sdl3_inc = path.join(os.projectdir(), "third_party/sdl3/include")
    sdl3_lib = path.join(os.projectdir(), "third_party/sdl3/lib/x64")
elseif (sdl3_inc == nil or sdl3_inc == "") or (sdl3_lib == nil or sdl3_lib == "") then
    raise("Veuillez définir à la fois sdl3_dir et sdl3_lib_dir pour utiliser une installation externe de SDL3.")
end

set_values("bix.sdl3_inc", sdl3_inc)
set_values("bix.sdl3_lib", sdl3_lib)

includes("engine/xmake.lua")
includes("tools/**/xmake.lua")
includes("apps/**/xmake.lua")
