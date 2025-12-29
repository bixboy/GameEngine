set_xmakever("3.0.4")
add_rules("mode.debug", "mode.release")

-- =============================================================================
-- CONFIGURATION
-- =============================================================================
local Config = {
    Name        = "BixEngine",
    Language    = "cxx20",
    BuildRoot   = "Build",
    Tools       = "Tools",
    ThirdParty  = "ThirdParty",
    Content     = "Content",
    Source      = "src"
}

set_languages(Config.Language)
set_warnings("allextra")
set_optimize("faster")

add_requires("box2d")

add_moduledirs(path.join(os.projectdir(), Config.Tools, "xmake"))

local function find_sdk(name, default_inc, default_lib)
    local inc = get_config(name .. "_dir") or default_inc
    local lib = get_config(name .. "_lib_dir") or default_lib
    return inc, lib
end

local SDL3_Include, SDL3_Lib = find_sdk("sdl3", 
    path.join(os.projectdir(), Config.ThirdParty, "SDL3-3.2.22/include"),
    path.join(os.projectdir(), Config.ThirdParty, "SDL3-3.2.22/lib/x64")
)

local SDLImage_Include, SDLImage_Lib = find_sdk("sdl3_image", 
    path.join(os.projectdir(), Config.ThirdParty, "SDL3_image/include/SDL3_image"),
    path.join(os.projectdir(), Config.ThirdParty, "SDL3_image/lib/x64")
)

local function get_build_path(...)
    local mode = get_config("mode") or "debug"
    return path.join(Config.BuildRoot, os.host(), os.arch(), mode, ...)
end

local Generated_Dir = get_build_path("Intermediate", "GeneratedHeaders")

-- =============================================================================
-- BUILD TOOLS
-- =============================================================================
target("BixHeaderTool")
    set_kind("binary")
    set_group("Tools")
    set_default(false)
    add_files(path.join(Config.Tools, "BixHeaderTool/**.cpp"))
    add_includedirs(path.join(Config.Tools, "BixHeaderTool"))
    before_build(function(t) t:set("targetdir", get_build_path()) end)
target_end()

target("GenerateHeaders")
    set_kind("phony")
    set_group("Tools")
    add_deps("BixHeaderTool")
    before_build(function()
        import("Tools.xmake.reflection")
        if not os.isdir(Generated_Dir) then os.mkdir(Generated_Dir) end
        reflection.generate_headers(false, Generated_Dir)
    end)
target_end()

-- =============================================================================
--  MODULE SYSTEM
-- =============================================================================
local Modules = {}
local Global_Public_Includes = {
    Config.Source,
    path.join(Config.ThirdParty),
    path.join(Config.ThirdParty, "ImGui"),
    path.join(Config.ThirdParty, "ImGui/backends"),
    path.join(Config.ThirdParty, "stb"),
    path.join(Config.ThirdParty, "miniaudio"),
    Generated_Dir,
    SDL3_Include,
    SDLImage_Include
}

for _, dir in ipairs(os.dirs(path.join(Config.Source, "*"))) do
    local mod_name = path.basename(dir)
    if mod_name ~= "Main" then
        local public_dir = path.join(dir, "public")
        if os.isdir(public_dir) then
            table.insert(Global_Public_Includes, public_dir)
        end
        table.insert(Modules, { name=mod_name, path=dir })
    end
end

local Module_Target_Names = {}

for _, mod in ipairs(Modules) do
    target(mod.name)
        set_kind("object")
        set_default(false)
        set_group("Engine/Modules")

        add_deps("GenerateHeaders")
        
        add_packages("box2d")
        add_includedirs(Global_Public_Includes, {public = true})
        add_includedirs(path.join(mod.path, "private"))
        
        if #os.files(path.join(mod.path, "**.cpp")) > 0 then
            add_files(path.join(mod.path, "**.cpp"), {rootdir = mod.path})
        end
        
        if #os.files(path.join(mod.path, "**.h")) > 0 then
            add_headerfiles(path.join(mod.path, "**.h"), {rootdir = mod.path})
        end
        
        table.insert(Module_Target_Names, mod.name)
    target_end()
end

-- =============================================================================
-- BixEngine
-- =============================================================================
target("BixEngine")
    set_kind("static")
    set_group("Engine")
    set_default(false)

    add_deps(table.unpack(Module_Target_Names))
    
    add_packages("box2d")
    add_linkdirs(SDL3_Lib, SDLImage_Lib)
    add_links("SDL3", "SDL3_image")
    
    add_includedirs(Global_Public_Includes, {public = true})
    
    add_files(path.join(Config.ThirdParty, "ImGui/*.cpp"))
    add_files(path.join(Config.ThirdParty, "ImGui/backends/imgui_impl_sdl3.cpp"))
    add_files(path.join(Config.ThirdParty, "ImGui/backends/imgui_impl_sdlrenderer3.cpp"))
    
target_end()

-- =============================================================================
-- BixMain
-- =============================================================================
target("BixMain")
    set_kind("object")
    set_group("Engine/Boot")
    set_default(false)
    
    add_files(path.join(Config.Source, "Main/private/**.cpp"))
    add_includedirs(Global_Public_Includes)
    
    add_packages("box2d")
    add_includedirs(SDL3_Include, SDLImage_Include)
target_end()

-- =============================================================================
--  BixRun 
-- =============================================================================
target("BixRun")
    set_kind("binary")
    set_group("Game")
    set_default(true)
    
    add_deps("BixEngine", "BixMain")
    
    add_includedirs(Global_Public_Includes)
    add_packages("box2d")
    add_linkdirs(SDL3_Lib, SDLImage_Lib)
    add_links("SDL3", "SDL3_image")

    local content_src = path.join(get_build_path(), Config.Content)
    if os.isdir(content_src) then
         if #os.files(path.join(content_src, "**.cpp")) > 0 then
             add_files(path.join(content_src, "**.cpp")) 
         end

         if #os.files(path.join(content_src, "**.h")) > 0 then
            add_headerfiles(path.join(content_src, "**.h"))
         end

         add_includedirs(content_src)
         print("Found Content Scripts: " .. content_src)
    end
    
    after_build(function(t)
        local out_dir = path.directory(t:targetfile())
        os.cp(path.join(SDL3_Lib, "SDL3.dll"), out_dir)
        os.cp(path.join(SDLImage_Lib, "SDL3_image.dll"), out_dir)
    end)
target_end()

-- =============================================================================
-- TASKS
-- =============================================================================
task("regen")
    on_run(function()
        import("Tools.xmake.reflection").generate_headers(true, Generated_Dir)
    end)
task_end()

task("cleanbuild")
    on_run(function()
        os.exec("xmake clean -a")
        os.tryrm(".xmake")
        os.tryrm("Build")
        task.run("regen")
        os.exec("xmake build BixRun")
        os.exec("xmake run BixRun")
    end)
task_end()
