local sdl3_inc = get_values("bix.sdl3_inc") or path.join(os.projectdir(), "third_party", "sdl3", "include")
local sdl3_lib = get_values("bix.sdl3_lib") or path.join(os.projectdir(), "third_party", "sdl3", "lib", "x64")

local reflection = import("tools.xmake.reflection")

target("bix_run")
    set_kind("binary")
    set_default(true)

    add_files("apps/bix_run/main.cpp")
    add_deps("bix_engine")

    add_includedirs("engine/include/Bix/Generated")
    add_includedirs(sdl3_inc)

    add_linkdirs(sdl3_lib)
    add_links("SDL3")

    before_build(function (target)
        reflection.ensure_headers_generated()
    end)

    after_build(function (target)
        local dll_path = path.join(sdl3_lib, "SDL3.dll")
        if os.isfile(dll_path) then
            os.cp(dll_path, path.directory(target:targetfile()))
        end
    end)
