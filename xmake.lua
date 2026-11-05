-- cp: header-only competitive-programming library (spec §13).
--
--   xmake                                  build main.cpp (debug by default)
--   xmake f -m release|releasedbg|debug    switch mode
--   xmake f --cxxstd=17|20|23|26           switch language level (default 23)
--   xmake f --toolchain=gcc|clang          switch compiler
--   xmake f --tests=y && xmake test        build and run the Catch2 tests
--   xmake matrix                           tests on gcc/clang x c++17/20/23/26
--   xmake bench [--filter=regex]           Google Benchmark targets, release build
set_project("cp")
set_xmakever("3.0.0")
set_allowedmodes("debug", "release", "releasedbg")
set_defaultmode("debug")
add_rules("mode.debug", "mode.release", "mode.releasedbg")

-- The matrix task builds in its own directories; only a normal build refreshes the
-- compile_commands.json that clangd reads.
if not os.getenv("CP_MATRIX") then
    add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})
end

option("cxxstd")
    set_default("23")
    set_showmenu(true)
    set_values("17", "20", "23", "26")
    set_description("C++ language level")
option_end()

option("bench")
    set_default(false)
    set_showmenu(true)
    set_description("Build the Google Benchmark targets (use with -m release)")
option_end()

option("tests")
    set_default(false)
    set_showmenu(true)
    set_description("Build the Catch2 tests and the header self-containment check")
option_end()

set_languages("c++" .. (get_config("cxxstd") or "23"))

-- debug: what LOCAL runs use; sanitizers stop at the first error.
if is_mode("debug") then
    add_defines("LOCAL", "_GLIBCXX_ASSERTIONS")
    set_policy("build.sanitizer.address", true)
    set_policy("build.sanitizer.undefined", true)
    add_cxflags("-fno-sanitize-recover=all")
    add_ldflags("-fno-sanitize-recover=all")
end

includes("tools/xmake/matrix.lua", "tools/xmake/bench.lua")

-- The library: headers only.
target("cp")
    set_kind("headeronly")
    add_headerfiles("include/(cp/**.hpp)")
    add_includedirs("include", {public = true})
target_end()

-- The solution.
target("main")
    set_kind("binary")
    add_files("main.cpp")
    add_deps("cp")
    set_warnings("allextra")
target_end()

if has_config("tests") then
    add_requires("catch2 3.x", {system = true})

    -- Library code is held to the strict set; Catch2's own headers are system headers.
    local function strict(target_name)
        target(target_name)
            set_group("tests")
            set_default(false)
            add_deps("cp")
            set_warnings("allextra", "pedantic", "error")
            add_cxflags("-Wshadow", "-Wundef")
            -- libstdc++ 16's <inplace_vector> uses the deprecated is_trivial_v, and clang reports
            -- it at our instantiation, not inside the system header.
            if get_config("cxxstd") == "26" then
                add_cxflags("-Wno-deprecated-declarations", {tools = {"clang", "clangxx"}})
            end
    end

    for _, file in ipairs(os.files("tests/test_*.cpp")) do
        strict(path.basename(file))
            set_kind("binary")
            add_files(file)
            add_includedirs("tests")
            add_packages("catch2")
            add_tests("default")
        target_end()
    end

    -- Every header must compile on its own (spec §4): one generated .cpp per header.
    strict("headers")
        set_kind("object")
        on_load(function (target)
            local root = path.join(os.projectdir(), "include")
            for _, header in ipairs(os.files(path.join(root, "cp", "**.hpp"))) do
                local rel = path.unix(path.relative(header, root))
                local source = path.join(target:autogendir(), "headers", (rel:gsub("/", "_"):gsub("%.hpp$", ".cpp")))
                local content = ('#include "%s"\n'):format(rel)
                if not os.isfile(source) or io.readfile(source) ~= content then
                    io.writefile(source, content)
                end
                target:add("files", source)
            end
        end)
    target_end()
end

-- One benchmark binary per bench/bench_*.cpp; `xmake bench` builds them in release mode and runs them.
if has_config("bench") then
    add_requires("benchmark", {system = true})
    for _, file in ipairs(os.files("bench/bench_*.cpp")) do
        target(path.basename(file))
            set_kind("binary")
            set_group("bench")
            set_default(false)
            add_deps("cp")
            add_files(file)
            add_packages("benchmark")
            set_warnings("allextra", "error")
        target_end()
    end
end
