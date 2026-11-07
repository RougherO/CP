-- xmake bench [--toolchain=gcc] [--filter=regex]
-- Builds the benchmarks in release mode (-O2, like the judges) in build/bench/<toolchain> and runs each one.
-- Your own `xmake f` settings are untouched.
task("bench")
    set_category("plugin")
    set_menu {
        usage = "xmake bench [options]",
        description = "Build (release) and run the Google Benchmark targets.",
        options = {
            {nil, "toolchain", "kv", "gcc", "Toolchain to build with."},
            {nil, "filter", "kv", nil, "Only run benchmarks matching this regex."}
        }
    }
    on_run(function ()
        import("core.base.option")
        import("isolated_build")
        import("report")
        report.start("bench")
        local opt = isolated_build.bench(option.get("toolchain"), "bench")
        for _, file in ipairs(os.files(path.join(os.projectdir(), "bench", "bench_*.cpp"))) do
            local args = {"--benchmark_min_warmup_time=0.1"}
            if option.get("filter") then table.insert(args, "--benchmark_filter=" .. option.get("filter")) end
            os.execv(os.programfile(), table.join({"run", path.basename(file)}, args), opt)
        end
        report.done()
    end)
task_end()
