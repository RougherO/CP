-- xmake matrix [--toolchains=gcc,clang,mingw] [--stds=17,20,23,26] [--bundles=y]: debug tests per cell (mingw runs
-- natively on Windows, else with wine), and every example bundled for that compiler and level (the 64 KiB gate).
task("matrix")
    set_category("plugin")
    set_menu {
        usage = "xmake matrix [options]",
        description = "Run the tests on every compiler x language level.",
        options = {
            {nil, "toolchains", "kv", "gcc,clang,mingw", "Comma-separated toolchains (mingw: Windows, built here)."},
            {nil, "stds", "kv", "17,20,23,26", "Comma-separated language levels."},
            {nil, "bundles", "kv", "y", "y or n: bundle every example per cell (fails over the size limit)."}
        }
    }
    on_run(function ()
        import("core.base.option")
        import("lib.detect.find_program")
        import("isolated_build")
        import("async.runjobs")
        local examples = os.files(path.join(os.projectdir(), "examples", "*.cpp"))
        -- Bundles every example for `toolchain` at `std` in parallel; the first failure's message, or nil.
        local function bundle_examples(toolchain, std, dir)
            local failure
            runjobs("bundles", function (i)
                local name = path.basename(examples[i])
                try {
                    function ()
                        os.iorunv(os.programfile(), {"bundle", "--profile=" .. toolchain, "--std=c++" .. std,
                                                     "--input=" .. examples[i],
                                                     "--output=" .. path.join(dir, "bundles", name .. ".cpp")},
                                  {curdir = os.projectdir()})
                    end,
                    catch {function (e)
                        local why = type(e) == "table" and ((e.stdout or "") .. (e.stderr or "")) or ""
                        if why:trim() == "" then why = tostring(e) end
                        why = why:gsub("\27%[[%d;]*m", "")
                        failure = failure or ("bundle " .. name .. ": " .. (why:match("bundle: (.+)") or why):trim())
                    end}
                }
            end, {total = #examples, comax = os.default_njob()})
            return failure
        end
        local wine = find_program("wine") ~= nil
        local results = {}
        local failed = 0
        for _, toolchain in ipairs(option.get("toolchains"):split(",")) do
            for _, std in ipairs(option.get("stds"):split(",")) do
                local name = toolchain .. "-c++" .. std
                local windows = toolchain == "mingw"
                local runs    = not windows or is_host("windows") or wine
                local err
                try {
                    function ()
                        local config = {"-m", "debug", "--cxxstd=" .. std, "--tests=y"}
                        table.join2(config, windows and {"-p", "mingw"} or {"--toolchain=" .. toolchain})
                        local opt = isolated_build.build(path.join(os.projectdir(), "build", "matrix", name), config, "tests")
                        if runs then os.execv(os.programfile(), {"test"}, opt) end
                        if option.get("bundles") == "y" then
                            local failure = bundle_examples(toolchain, std, path.join(os.projectdir(), "build", "matrix", name))
                            if failure then raise(failure) end
                        end
                    end,
                    catch { function (e) err = table.concat(table.slice(tostring(e):split("\n"), 1, 20), "\n    ") end }
                }
                if err then
                    table.insert(results, "FAIL " .. name .. ": " .. err)
                    failed = failed + 1
                else
                    table.insert(results, "PASS " .. name .. (runs and "" or " (built; no wine)"))
                end
            end
        end
        print("")
        for _, line in ipairs(results) do print(line) end
        if failed > 0 then raise("%d of %d cells failed", failed, #results) end
    end)
task_end()
