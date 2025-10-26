local config = import("core.project.config")

local M = {}

local function generated_dir()
    return path.join(os.projectdir(), "engine", "include", "Bix", "Generated")
end

local function build_tool_path()
    config.load()
    local buildir = config.buildir and config.buildir() or config.get("buildir") or "build"
    local plat = config.get("plat") or os.host()
    local arch = config.get("arch") or os.arch()
    local mode = config.get("mode") or "debug"
    local suffix = is_plat("windows") and ".exe" or ""
    return path.join(os.projectdir(), buildir, plat, arch, mode, "bix_header_tool" .. suffix)
end

local function scan_generated_includes()
    local names = {}
    local patterns = {
        "engine/include/**.h",
        "Samples/**.h"
    }

    for _, pattern in ipairs(patterns) do
        for _, header in ipairs(os.files(pattern)) do
            local content = io.readfile(header)
            if content then
                local inc = content:match([[#include%s+"([%w_]+%.generated%.h)"]])
                if inc then
                    names[inc] = true
                end
            end
        end
    end

    return names
end

function M.generated_dir()
    return generated_dir()
end

function M.clean_generated()
    local dir = generated_dir()
    if not os.isdir(dir) then
        return
    end

    for _, file in ipairs(os.files(path.join(dir, "*.generated.h"))) do
        os.rm(file)
    end
end

function M.generate_headers(force)
    local dir = generated_dir()
    if force then
        M.clean_generated()
    end

    if not os.isdir(dir) then
        os.mkdir(dir)
    end

    local names = scan_generated_includes()
    local count = 0
    for _ in pairs(names) do
        count = count + 1
    end

    if count == 0 then
        cprint("${bright red}[Reflection] Aucun include .generated.h trouvé — rien à faire.")
        return
    end

    for name in pairs(names) do
        local stub = path.join(dir, name)
        if not os.isfile(stub) then
            io.writefile(stub, "")
        end
    end

    local tool = build_tool_path()
    if not os.isfile(tool) then
        raise("BixHeaderTool introuvable : " .. tool)
    end

    local args = {
        path.join(os.projectdir(), "engine", "include"),
        path.join(os.projectdir(), "Samples"),
        dir
    }

    local ok, output = os.iorunv(tool, args)
    if not ok then
        raise(output or "Échec de l'exécution de BixHeaderTool")
    end

    if output and #output > 0 then
        print(output)
    end

    for _, file in ipairs(os.files(path.join(dir, "*.generated.h"))) do
        if os.filesize(file) == 0 then
            os.rm(file)
        end
    end
end

function M.ensure_headers_generated()
    local dir = generated_dir()
    if not os.isdir(dir) then
        os.mkdir(dir)
    end

    local headers = os.files(path.join(dir, "*.generated.h"))
    if headers and #headers > 0 then
        return
    end

    os.exec("xmake build bix_header_tool")
    M.generate_headers(false)
end

return M
