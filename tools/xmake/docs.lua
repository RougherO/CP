-- xmake docs [--check]: copy examples/*.cpp into the guide's marked code blocks, then build the Doxygen site.
task("docs")
    set_category("plugin")
    set_menu {
        usage = "xmake docs [options]",
        description = "Build the documentation site (README, guide and API) into build/docs/html.",
        options = {
            {nil, "check", "k", nil, "Only check that every marked code block matches its example file."}
        }
    }
    on_run(function ()
        import("core.base.option")
        import("lib.detect.find_program")
        local root  = os.projectdir()
        local pages = table.join({path.join(root, "README.md")}, os.files(path.join(root, "docs", "guide", "*.md")))
        local stale = {}
        -- A block marked `<!-- example: name.cpp -->` holds examples/name.cpp exactly.
        for _, page in ipairs(pages) do
            local text = io.readfile(page):gsub("\r\n", "\n")
            local _, markers = text:gsub("<!%-%- example: ", "")
            local synced, blocks = text:gsub("(<!%-%- example: ([%w_%-%.]+) %-%->\n```cpp\n)(.-)(```)", function (open, name, _, close)
                local file = path.join(root, "examples", name)
                if not os.isfile(file) then raise("docs: %s names a missing example %s", path.relative(page, root), name) end
                return open .. io.readfile(file):gsub("\r\n", "\n") .. close
            end)
            if blocks ~= markers then
                raise("docs: %s has %d example markers but %d marked ```cpp blocks", path.relative(page, root), markers, blocks)
            end
            if synced ~= text then
                table.insert(stale, path.relative(page, root))
                if not option.get("check") then io.writefile(page, synced) end
            end
        end
        if option.get("check") then
            if #stale > 0 then raise("docs: examples out of sync in %s (run xmake docs)", table.concat(stale, ", ")) end
            print("docs: every example block matches its file")
            return
        end
        for _, page in ipairs(stale) do print("docs: synced examples into %s", page) end
        local doxygen = find_program("doxygen")
        if not doxygen then raise("docs: needs doxygen on PATH") end
        os.tryrm(path.join(root, "build", "docs", "html"))
        os.mkdir(path.join(root, "build", "docs"))
        os.execv(doxygen, {"Doxyfile"}, {curdir = root})
        print("docs: build/docs/html/index.html")
    end)
task_end()
