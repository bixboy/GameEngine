-- XMake build script for BixEngine

set_xmakever("3.0.4")
add_rules("mode.debug", "mode.release")

add_moduledirs(path.join(os.projectdir(), "Tools/xmake"))

set_languages("cxx20")
set_warnings("allextra")
set_optimize("faster")

-- SDL3 configuration
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

    sdl3_inc = path.join(os.projectdir(), "ThirdParty/SDL3-3.2.22/include")
    sdl3_lib = path.join(os.projectdir(), "ThirdParty/SDL3-3.2.22/lib")

elseif (sdl3_inc == nil or sdl3_inc == "") or (sdl3_lib == nil or sdl3_lib == "") then

    raise("Veuillez définir *à la fois* sdl3_dir et sdl3_lib_dir pour utiliser SDL3 externe.")
end

-- Dossier de génération dynamique
local plat = get_config("plat") or os.host()
local arch = get_config("arch") or os.arch()
local mode = get_config("mode") or "debug"

local generated_dir = path.join(
    "Build",
    plat,
    arch,
    mode,
    "Intermediate",
    "GeneratedHeaders"
)

print("[IncludeDir] Added generated header path:", path.join(os.projectdir(), generated_dir))

-- ✅ Target: BixHeaderTool
target("BixHeaderTool")
    set_kind("binary")
    set_default(false)
    set_plat(os.host())
    set_arch(os.arch())
    set_policy("build.fence", true)

    add_includedirs("Tools/BixHeaderTool", { public = true })
    add_files("Tools/BixHeaderTool/**.cpp")

-- ✅ Target: BixEngine
target("BixEngine")
    set_kind("static")
    add_deps("BixHeaderTool")

    -- Include paths
    add_includedirs("Runtime/Include", { public = true })
    add_includedirs("ThirdParty/ImGui", { public = true })
    add_includedirs("ThirdParty/ImGui/backends", { public = true })
    add_includedirs(sdl3_inc)
    add_includedirs(path.join(os.projectdir(), generated_dir), { public = true })
    add_includedirs(generated_dir, { public = true })


    -- Source files
    add_files("Runtime/Source/**.cpp")

    -- Libs
    add_linkdirs(sdl3_lib)
    add_links("SDL3")

    -- Auto-regeneration
    before_build(function(target)
        local reflection = import("Tools.xmake.reflection")

        if not os.isdir(generated_dir) or #os.files(path.join(generated_dir, "*.generated.h")) == 0 then
            print("[BixEngine] Génération initiale des headers manquants...")
            reflection.generate_headers(false, generated_dir)
        end
    end)

-- ✅ Task: regen
task("regen")
    set_category("action")

    on_run(function()
        import("core.base.option")
        local reflection = import("Tools.xmake.reflection")
        
        os.exec("xmake build BixHeaderTool")
        reflection.generate_headers(option.get("force") or false, generated_dir)
    end)

    set_menu {
        usage = "xmake regen [options]",
        description = "Regénérer les fichiers d'en-tête générés (reflection)",
        options = {
            { nil, "force", "k", nil, "Supprimer tous les headers générés avant la régénération" }
        }
    }

-- ✅ Task: fullbuild
task("fullbuild")
    set_category("action")

    on_run(function()
        import("core.base.option")
        import("core.project.task")
        task.run("regen", { force = option.get("force") or false })
        os.exec("xmake build")
    end)

    set_menu {
        usage = "xmake fullbuild [options]",
        description = "Régénérer les headers puis construire entièrement BixEngine",
        options = {
            { nil, "force", "k", nil, "Forcer la régénération complète avant la compilation" }
        }
    }
