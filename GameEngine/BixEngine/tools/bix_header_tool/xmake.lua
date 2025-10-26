target("bix_header_tool")
    set_kind("binary")
    set_default(false)
    set_policy("build.fence", true)

    add_includedirs("tools/bix_header_tool/include", { public = true })
    add_files("tools/bix_header_tool/source/**.cpp")
