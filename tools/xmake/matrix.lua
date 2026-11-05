-- xmake matrix [--toolchains=gcc,clang] [--stds=17,20,23,26]
-- Builds and runs the tests (debug mode) for every compiler and language level.
-- Each cell has its own build and config directory, so your own `xmake f` settings
-- are untouched and re-runs are incremental.
task("matrix")
    set_category("plugin")
    set_menu {
        usage = "xmake matrix [options]",
        description = "Run the tests on every compiler x language level.",
        options = {
            {nil, "toolchains", "kv", "gcc,clang", "Comma-separated toolchains."},
            {nil, "stds", "kv", "17,20,23,26", "Comma-separated language levels."}
        }
    }
    on_run(function ()
        import("core.base.option")
        local results = {}
        local failed = 0
        for _, toolchain in ipairs(option.get("toolchains"):split(",")) do
            for _, std in ipairs(option.get("stds"):split(",")) do
                local name = toolchain .. "-c++" .. std
                local dir = path.join(os.projectdir(), "build", "matrix", name)
                local opt = {envs = {XMAKE_CONFIGDIR = path.join(dir, "config"), CP_MATRIX = "1"}}
                local ok = try {
                    function ()
                        os.execv(os.programfile(), {"f", "-y", "-q", "-m", "debug", "--toolchain=" .. toolchain,
                                                    "--cxxstd=" .. std, "--tests=y", "--builddir=" .. dir}, opt)
                        os.execv(os.programfile(), {"build", "-q", "-g", "tests"}, opt)
                        os.execv(os.programfile(), {"test"}, opt)
                        return true
                    end,
                    catch { function () end }
                }
                table.insert(results, (ok and "PASS " or "FAIL ") .. name)
                if not ok then failed = failed + 1 end
            end
        end
        print("")
        for _, line in ipairs(results) do print(line) end
        if failed > 0 then raise("%d of %d cells failed", failed, #results) end
    end)
task_end()
