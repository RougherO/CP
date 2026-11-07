-- xmake iobench [--toolchain=gcc] [--count=10000000] [--runs=5]
-- Whole-program I/O race (spec §8.4): cp, scanf/printf and unsynced cin/cout each read N numbers
-- and write N numbers, from/to a file and through pipes. Release build (-O2) in build/bench/<toolchain>; best of
-- --runs per cell. Every contender must produce the same sum and the same bytes, or the task fails.
task("iobench")
    set_category("plugin")
    set_menu {
        usage = "xmake iobench [options]",
        description = "Time N integers read and written by cp, scanf/printf and cin/cout.",
        options = {
            {nil, "toolchain", "kv", "gcc", "Toolchain to build with."},
            {nil, "count", "kv", "10000000", "Numbers to read and write."},
            {nil, "runs", "kv", "5", "Runs per measurement; the fastest is kept."}
        }
    }
    on_run(function ()
        import("core.base.option")
        import("isolated_build")
        import("report")
        report.start("iobench")
        local toolchain = option.get("toolchain")
        local count = option.get("count")
        local runs = tonumber(option.get("runs"))
        local contenders = {{"io_cp", "cp"}, {"io_iostream", "cin/cout"}, {"io_stdio", "scanf/printf"}}
        local opt, dir = isolated_build.bench(toolchain, "iobench")
        local names = {}
        for _, c in ipairs(contenders) do table.insert(names, c[1]) end
        local progs = isolated_build.targetfiles(opt, names)

        local work = path.join(dir, "iobench")
        os.mkdir(work)
        local input = path.join(work, "input.txt")
        local output = path.join(work, "out.txt")
        local sum = path.join(work, "sum.txt")
        os.execv(progs[1], {"write", count}, {stdout = input})

        -- Pipes need a shell: POSIX hosts only.
        local posix = not is_host("windows")
        local function sh(cmd) os.execv("sh", {"-c", cmd}) end
        local function quote(s) return "'" .. s:gsub("'", [['\'']]) .. "'" end
        local scenarios = {
            {"read file", function (p) os.execv(p, {"read"}, {stdin = input, stderr = sum}) end},
            {"read pipe", posix and function (p) sh("cat " .. quote(input) .. " | " .. quote(p) .. " read 2> " .. quote(os.nuldev())) end},
            {"write file", function (p) os.execv(p, {"write", count}, {stdout = output}) end},
            {"write pipe", posix and function (p) sh(quote(p) .. " write " .. count .. " | cat > " .. quote(os.nuldev())) end}
        }
        if not posix then report.warning("pipe scenarios need a POSIX shell; skipped on this host") end

        local function best(run, prog)
            local t = math.huge
            for _ = 1, runs do
                local start = os.mclock()
                run(prog)
                t = math.min(t, os.mclock() - start)
            end
            return t
        end

        local sums, hashes = {}, {}
        report.info("%d numbers, best of %d, %s -O2 (ms)", count, runs, toolchain)
        local header = string.format("%-14s", "")
        for _, s in ipairs(scenarios) do header = header .. string.format("%12s", s[1]) end
        print(header)
        for i, c in ipairs(contenders) do
            local row = string.format("%-14s", c[2])
            for _, s in ipairs(scenarios) do
                row = row .. (s[2] and string.format("%12d", best(s[2], progs[i])) or string.format("%12s", "-"))
            end
            print(row)
            sums[c[2]] = io.readfile(sum):trim()
            hashes[c[2]] = hash.md5(output)
        end
        for _, c in ipairs(contenders) do
            if sums[c[2]] ~= sums["cp"] or hashes[c[2]] ~= hashes["cp"] then
                raise("iobench: %s disagrees with cp: sum %s vs %s, output %s vs %s", c[2], sums[c[2]], sums["cp"],
                      hashes[c[2]], hashes["cp"])
            end
        end
        report.info("all contenders read the same sum and wrote the same bytes")
        os.rm(output)
        report.done()
    end)
task_end()
