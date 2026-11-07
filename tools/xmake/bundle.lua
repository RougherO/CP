-- xmake bundle [--profile=gcc|clang|mingw] [--std=c++20] [--checks=0|1] [--sample=in.txt]: one-file submission (spec §14.2).
task("bundle")
    set_category("plugin")
    set_menu {
        usage = "xmake bundle [options]",
        description = "Write a single-file submission with every cp header the solution includes inlined and resolved.",
        options = {
            {nil, "profile", "kv", "gcc", "Judge compiler: gcc, clang (Linux) or mingw (Windows, e.g. Codeforces)."},
            {nil, "std", "kv", "c++20", "Language level the judge compiles with."},
            {nil, "checks", "kv", "0", "1 or 0: CP_CHECKS (and libstdc++'s assertions) in the submission."},
            {nil, "input", "kv", "main.cpp", "Solution to bundle."},
            {nil, "output", "kv", "build/submit.cpp", "Single file to write."},
            {nil, "sample", "kv", nil, "Input file: run the bundled and unbundled solution on it and compare."}
        }
    }
    on_run(function ()
        import("core.base.option")
        import("lib.detect.find_program")
        import("bundler")

        local limit = 65535  -- bytes a judge accepts (Codeforces: 64 KiB)
        local profiles = {
            gcc   = {cxx = "g++", os = "__linux__", exe = ""},
            clang = {cxx = "clang++", os = "__linux__", exe = ""},
            -- Windows judges link statically; elsewhere wine runs the result and clang reads MinGW's headers.
            mingw = {cxx = is_host("windows") and "g++" or "x86_64-w64-mingw32-g++", os = "_WIN32", exe = ".exe",
                     link = {"-static"}, runner = not is_host("windows") and "wine" or nil,
                     clang = {"--target=x86_64-w64-mingw32"}},
        }
        local name    = option.get("profile")
        local profile = assert(profiles[name], "bundle: unknown --profile=" .. tostring(name) .. " (gcc, clang, mingw)")
        local checks  = option.get("checks")
        assert(checks == "0" or checks == "1", "bundle: --checks takes 0 or 1")
        local ctx = {name = name, profile = profile, std = "-std=" .. option.get("std"), checks = checks,
                     cxx = find_program(profile.cxx), clang = find_program("clang++"),
                     runner = profile.runner and find_program(profile.runner)}
        if not ctx.cxx then raise("bundle: profile %s needs %s on PATH", name, profile.cxx) end

        local root    = os.projectdir()
        local include = path.join(root, "include")
        local input   = path.absolute(option.get("input"), root)
        local output  = path.absolute(option.get("output"), root)
        ctx.tmp = os.tmpfile() .. ".bundle"  -- one per run: bundles may run side by side
        os.mkdir(ctx.tmp)

        local head, tail, count = bundler.inline(input, include)
        local full = bundler.strip(bundler.resolve(head, ctx)) .. "\n" .. tail
        if not full:endswith("\n") then full = full .. "\n" end
        if checks == "1" then full = "#define _GLIBCXX_ASSERTIONS 1\n" .. full end

        -- Candidates, smallest first: shaken and pruned until nothing changes, shaken once, as inlined.
        local candidates, kept = {}, nil
        if ctx.clang then
            local shaken, why, largest = bundler.shake(full, ctx)
            if shaken then
                kept = largest
                local text, rounds = shaken, 0
                while rounds < 8 do
                    local pruned, edits = bundler.prune(text, ctx)
                    if not pruned or edits == 0 then break end
                    local again, _, again_kept = bundler.shake(pruned, ctx)
                    if not again then break end
                    text, kept, rounds = again, again_kept, rounds + 1
                end
                if rounds > 0 then
                    table.insert(candidates, {text = text, what = "shaken and pruned",
                                              note = string.format("%s, then %d prune rounds", why, rounds)})
                end
                table.insert(candidates, {text = shaken, what = "shaken", note = why})
            else
                print("bundle: warning: no tree-shaking (%s)", why)
            end
        else
            print("bundle: warning: no tree-shaking (clang++ not found)")
        end
        table.insert(candidates, {text = full, what = "unshaken"})

        local sample = option.get("sample")
        if sample then
            ctx.sample = path.absolute(sample, root)
            if profile.runner and not ctx.runner then
                print("bundle: --sample skipped: running a %s program needs %s", name, profile.runner)
            else
                ctx.reference = bundler.reference(input, include, ctx)
            end
        end
        local text, chosen
        for i, c in ipairs(candidates) do
            local candidate = bundler.drop_unused_macros(c.text)
            local ok, why = bundler.verify(candidate, ctx)
            if ok then
                text, chosen = candidate, c
                break
            end
            local last = i == #candidates
            local kept_as = path.join(path.directory(output), "failed-" .. c.what:gsub("%s", "-") .. ".cpp")
            io.writefile(kept_as, candidate, {encoding = "binary"})
            if last then raise("bundle: the bundle %s (kept as %s)", why, path.relative(kept_as, root)) end
            print("bundle: warning: the %s bundle %s (kept as %s); trying it %s", c.what, why,
                  path.relative(kept_as, root), candidates[i + 1].what)
        end
        os.tryrm(ctx.tmp)

        print("%s: %d bytes, %d headers inlined, profile %s, %s, checks %s, %s%s; compiles%s",
              path.relative(output, root), #text, count, name, option.get("std"), checks == "1" and "on" or "off",
              chosen.what, chosen.note and (" (" .. chosen.note .. ")") or "",
              ctx.reference and ", same output and exit code as unbundled" or "")
        if #text > limit then
            if kept and chosen.what ~= "unshaken" then
                print("largest kept declarations:")
                for i = 1, math.min(10, #kept) do print("  %6d  %s", kept[i].size, kept[i].name) end
            end
            raise("bundle: %d bytes is over the %d-byte limit (%s not written)", #text, limit,
                  path.relative(output, root))
        end
        io.writefile(output, text, {encoding = "binary"})
    end)
task_end()
