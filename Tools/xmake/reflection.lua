import("core.project.config")

local M = {}

local function canonical_path(dir)
    if not dir or dir == "" then
        return nil
    end
    local abs = path.absolute(dir)
    if not abs then
        return dir
    end
    return abs:gsub("\\", "/"):lower()
end

local function push_unique(list, seen, dir)
    if not dir or dir == "" then
        return
    end
    local key = canonical_path(dir) or dir
    if not seen[key] then
        seen[key] = true
        table.insert(list, dir)
    end
end

local function collect_header_roots()
    local roots = {}
    local seen = {}

    for _, dir in ipairs(os.dirs(path.join(os.projectdir(), "src/*"))) do
        if os.isdir(dir) then
            push_unique(roots, seen, dir)
        end
    end

    local plat = get_config("plat") or os.host()
    local arch = get_config("arch") or os.arch()
    local mode = get_config("mode") or "debug"
    local content_dir = path.join(os.projectdir(), "Build", plat, arch, mode, "Content")
    if os.isdir(content_dir) then
        push_unique(roots, seen, content_dir)
    end

    return roots
end

local function needs_generation(roots)
    for _, root in ipairs(roots) do
        for _, header in ipairs(os.files(path.join(root, "**.h"))) do
            local content = io.readfile(header)
            if content and content:find('#include%s+"[%w_]+%.generated%.h"') then
                return true
            end
        end
    end
    return false
end

local function resolve_tool_binary(name)
    local plat = get_config("plat") or os.host()
    local arch = get_config("arch") or os.arch()
    local mode = get_config("mode") or "debug"
    local suffix = ""
    if (plat and plat:lower() == "windows") or os.host() == "windows" then
        suffix = ".exe"
    end

    local candidates = {
        path.join(os.projectdir(), "build", mode, name .. suffix),
        path.join(os.projectdir(), "Build", os.host(), os.arch(), mode, name .. suffix),
        path.join(os.projectdir(), "Build", plat, arch, mode, name .. suffix)
    }

    for _, candidate in ipairs(candidates) do
        if os.isfile(candidate) then
            return candidate
        end
    end

    return nil
end

local function remove_empty_generated(output_dir)
    for _, f in ipairs(os.files(path.join(output_dir, "*.generated.h"))) do
        local size = os.filesize(f)
        if size == 0 then
            cprint(string.format("${dim red}   ✖ Suppression (vide) : %s", f))
            os.rm(f)
        else
            cprint(string.format("${dim green}   ✓ Valide : %s (%d bytes)", f, size))
        end
    end
end

local function run_generate_headers(force, generated_dir)
    local output_dir = generated_dir
    if not path.is_absolute(output_dir) then
        output_dir = path.join(os.projectdir(), generated_dir)
    end

    cprint("${bright cyan}──────────────────────────────────────────────")
    cprint("${bright cyan}[Reflection] Démarrage de la génération des headers...")
    cprint("${bright cyan}──────────────────────────────────────────────")
    cprint(string.format("${dim blue}→ Dossier de sortie : %s", output_dir))

    if force and os.isdir(output_dir) then
        cprint("${bright yellow}[*] Suppression complète des fichiers générés (--force)")
        for _, f in ipairs(os.files(path.join(output_dir, "*.generated.h"))) do
            cprint(string.format("${dim red}   ✖ Suppression : %s", f))
            os.rm(f)
        end
    end

    if not os.isdir(output_dir) then
        cprint(string.format("${bright yellow}[*] Création du dossier manquant : %s", output_dir))
        os.mkdir(output_dir)
    else
        cprint("${dim green}[✓] Dossier de génération déjà présent.")
    end

    local header_roots = collect_header_roots()
    if not needs_generation(header_roots) then
        cprint("${bright red}[!] Aucun include .generated.h trouvé — rien à générer.")
        return
    end

    cprint("${bright yellow}[*] Exécution de BixHeaderTool pour générer les headers réels...")

    local tool_exe = resolve_tool_binary("BixHeaderTool")
    if not tool_exe then
        cprint("${bright red}[!] Erreur : BixHeaderTool introuvable, impossible de générer les headers.")
        return
    end

    cprint(string.format("${dim blue}→ Résolution du binaire : %s", tool_exe))

    local args = {}
    for _, dir in ipairs(header_roots) do
        table.insert(args, dir)
    end
    table.insert(args, output_dir)

    local code, stdout, stderr = os.execv(tool_exe, args)
    if code ~= 0 then
        cprint("${bright red}[!] Échec de l'exécution de BixHeaderTool :")
        if stdout and stdout ~= "" then
            print(stdout)
        end
        if stderr and stderr ~= "" then
            print(stderr)
        end
        return
    end

    cprint("${bright green}[✓] BixHeaderTool exécuté avec succès.")
    cprint("${bright yellow}[*] Nettoyage des fichiers vides...")
    remove_empty_generated(output_dir)
    cprint("${bright green}[+] Fichiers .generated.h mis à jour avec succès.")
    cprint("${bright cyan}──────────────────────────────────────────────\n")
end

M.generate_headers = run_generate_headers
M.run = run_generate_headers

return setmetatable(M, {
    __call = function(_, ...)
        return run_generate_headers(...)
    end
})
