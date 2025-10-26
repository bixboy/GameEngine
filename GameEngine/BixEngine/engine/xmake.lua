local sdl3_inc = get_values("bix.sdl3_inc") or path.join(os.projectdir(), "third_party", "sdl3", "include")
local sdl3_lib = get_values("bix.sdl3_lib") or path.join(os.projectdir(), "third_party", "sdl3", "lib", "x64")

local reflection = import("tools.xmake.reflection")

target("bix_engine")
    set_kind("static")
    set_default(false)
    set_policy("build.fence", true)

    add_deps("bix_header_tool")

    add_files("engine/source/**.cpp")

    add_files("third_party/imgui/*.cpp")
    add_files("third_party/imgui/backends/imgui_impl_sdl3.cpp")
    add_files("third_party/imgui/backends/imgui_impl_sdlrenderer3.cpp")

    add_includedirs("engine/include", { public = true })
    add_includedirs("engine/include/Bix/Generated", { public = true })
    add_includedirs("third_party/imgui", { public = true })
    add_includedirs("third_party/imgui/backends", { public = true })
    add_includedirs(sdl3_inc, { public = true })

    add_linkdirs(sdl3_lib)
    add_links("SDL3")

    on_load(function (target)
        reflection.ensure_headers_generated()
    end)

    after_clean(function ()
        reflection.clean_generated()
    end)

task("regen")
    set_category("action")
    set_menu {
        usage = "xmake regen [options]",
        description = "Regénère les fichiers d'en-tête de réflexion",
        options = {
            { nil, "force", "k", nil, "Supprimer les fichiers générés avant de les regénérer" }
        }
    }

    on_run(function ()
        local option = import("core.base.option")
        local force = option.get("force") or false
        if force then
            cprint("${bright yellow}[BixEngine] Nettoyage complet des entêtes générés...")
        end
        os.exec("xmake build bix_header_tool")
        reflection.generate_headers(force)
    end)
