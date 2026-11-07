-- cp: header-only competitive-programming library (spec §13).
--
--   xmake                                  build main.cpp (debug by default)
--   xmake f -m release|releasedbg|debug    switch mode
--   xmake f --cxxstd=17|20|23|26           switch language level (default 23)
--   xmake f --toolchain=gcc|clang          switch compiler
--   xmake f --tests=y && xmake test        build and run the Catch2 tests
--   xmake matrix                           tests on gcc/clang/mingw x c++17/20/23/26 (mingw: built; run with wine)
--   xmake bench [--filter=regex]           Google Benchmark targets, release build
--   xmake iobench [--count=N]              whole-program I/O race: cp vs scanf/printf and cin/cout
--   xmake docs [--check]                   the doc site (Doxygen) in build/docs/html; --check: examples in sync
--   xmake bundle [--profile=gcc|clang|mingw] [--std=c++20] [--checks=0|1] [--sample=in.txt]
--                                          build/submit.cpp for one judge: inlined, #if-resolved, tree-shaken,
--                                          verified, at most 64 KB (mingw: Windows judges such as Codeforces)
set_project("cp")
set_xmakever("3.0.0")
set_allowedmodes("debug", "release", "releasedbg")
set_defaultmode("debug")
add_rules("mode.debug", "mode.release", "mode.releasedbg")

-- The matrix and bench tasks build in their own directories; only a normal build refreshes the
-- compile_commands.json that clangd reads.
if not os.getenv("CP_ISOLATED_BUILD") then
    add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})
end

-- Before `xmake f` runs, get_config sees no default, so the language level reads it from here too.
local default_cxxstd = "23"
option("cxxstd")
    set_default(default_cxxstd)
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

set_languages("c++" .. (get_config("cxxstd") or default_cxxstd))

-- debug: what local runs use: cp checks on, sanitizers stop at the first error (MinGW has none).
if is_mode("debug") then
    add_defines("CP_CHECKS=1", "_GLIBCXX_ASSERTIONS")
    if not is_plat("mingw") then
        set_policy("build.sanitizer.address", true)
        set_policy("build.sanitizer.undefined", true)
        add_cxflags("-fno-sanitize-recover=all")
        add_ldflags("-fno-sanitize-recover=all")
    end
end

-- Windows programs built here link statically, as judges do, so wine runs them without MinGW's DLLs.
if is_plat("mingw") then
    add_ldflags("-static")
end

add_moduledirs("tools/xmake/modules")
includes("tools/xmake/matrix.lua", "tools/xmake/bench.lua", "tools/xmake/iobench.lua", "tools/xmake/bundle.lua",
         "tools/xmake/docs.lua")

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
    add_requires("catch2 3.x")

    -- Library code is held to the strict set; Catch2's own headers are system headers.
    local function strict(target_name, body)
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
            body()
        target_end()
    end

    for _, file in ipairs(os.files("tests/test_*.cpp")) do
        strict(path.basename(file), function ()
            set_kind("binary")
            add_files(file)
            add_includedirs("tests")
            add_packages("catch2")
            add_tests("default")
        end)
    end

    -- The documentation's examples (examples/*.cpp): compiled with the tests, never run.
    for _, file in ipairs(os.files("examples/*.cpp")) do
        strict("example_" .. path.basename(file), function ()
            set_kind("binary")
            add_files(file)
        end)
    end

    -- Every header must compile on its own (spec §4): one generated .cpp per header.
    strict("headers", function ()
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
    end)
end

-- `xmake bench` runs bench/bench_*.cpp (Google Benchmark); `xmake iobench` races bench/io/io_*.cpp. Release builds.
if has_config("bench") then
    add_requires("benchmark")
    for _, set in ipairs({{"bench/bench_*.cpp", "bench", "benchmark"}, {"bench/io/io_*.cpp", "iobench"}}) do
        for _, file in ipairs(os.files(set[1])) do
            target(path.basename(file))
                set_kind("binary")
                set_group(set[2])
                set_default(false)
                add_deps("cp")
                add_files(file)
                add_includedirs(path.directory(file))
                if set[3] then add_packages(set[3]) end
                set_warnings("allextra", "error")
            target_end()
        end
    end
end
