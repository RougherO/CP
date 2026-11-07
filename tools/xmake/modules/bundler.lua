-- The stages of `xmake bundle` (tools/xmake/bundle.lua): inline, resolve, strip, shake, prune, verify.
import("core.base.json")

-- Files are read and written byte for byte: on Windows, text mode turns "\n" into "\r\n", which shifts clang's offsets.
local as_bytes = {encoding = "binary"}

-- The first `n` lines of `text`.
local function first_lines(text, n)
    local lines = tostring(text or ""):split("\n")
    return table.concat(lines, "\n", 1, math.min(n, #lines))
end

-- Stdout of `program args`, or nil and the start of its stderr.
local function capture(program, args)
    local failure
    local out = try {function () return os.iorunv(program, args) end,
                     catch {function (e) failure = type(e) == "table" and (e.stderr or e.errors) or e end}}
    if failure then return nil, first_lines(failure, 20) end
    return out
end

-- 1. The solution with every cp header inlined: `head` (through its last cp include, other #includes masked as
-- CP_BUNDLE_INCLUDE for step 2) and `tail` (the rest, as written); and the number of headers. Each header gets a guard
-- and is inlined again at later includes until one is outside every #if (a false #if would drop its only copy).
function inline(input, include)
    local settled, guards, count = {}, {}, 0
    local raw, masked, cut = {}, {}, 0
    local function emit(file, is_cp, depth)
        for _, line in ipairs(io.readfile(file, as_bytes):split("\n", {strict = true})) do
            local directive = line:match("^%s*#%s*(%a+)")
            local header    = directive == "include" and line:match('^%s*#%s*include%s*"(cp/[^"]+)"')
            if directive and directive:startswith("if") then
                depth = depth + 1
            elseif directive == "endif" then
                depth = depth - 1
            end
            if header then
                if not settled[header] then
                    if not guards[header] then
                        count = count + 1
                        guards[header] = "CP_BUNDLE_GUARD_" .. count
                    end
                    settled[header] = depth == 0
                    for _, l in ipairs({"#ifndef " .. guards[header], "#define " .. guards[header]}) do
                        table.insert(raw, l)
                        table.insert(masked, l)
                    end
                    emit(path.join(include, header), true, depth)  -- the guard itself is not a condition
                    table.insert(raw, "#endif")
                    table.insert(masked, "#endif")
                end
                if not is_cp then cut = #raw end
            elseif not (directive == "pragma" and line:match("^%s*#%s*pragma%s+once")) then
                table.insert(raw, line)
                table.insert(masked, directive == "include" and "CP_BUNDLE_INCLUDE" .. line:match("include(.*)$") or line)
            end
        end
    end
    emit(input, false, 0)
    return table.concat(masked, "\n", 1, cut), table.concat(raw, "\n", cut + 1, #raw), count
end

-- Macros `cxx std` predefines for `file`, by name.
local function macros(cxx, std, file)
    local set = {}
    local text, err = capture(cxx, {std, "-dM", "-E", "-x", "c++", file})
    if not text then raise("bundle: %s cannot preprocess:\n%s", cxx, err) end
    for line in text:gmatch("[^\n]+") do
        local n = line:match("^#define%s+([%w_]+)")
        if n then set[n] = line end
    end
    return set
end

-- 2. `head` with every #if resolved for this compiler and --std, cp's macros expanded nowhere (-fdirectives-only).
-- Returns it and the set of macros the standard library defines (step 3 drops their #defines).
function resolve(head, ctx)
    local version, empty = path.join(ctx.tmp, "version.cpp"), path.join(ctx.tmp, "empty.cpp")
    io.writefile(version, "#include <version>\n", as_bytes)
    io.writefile(empty, "\n", as_bytes)
    local predefined, with_library = macros(ctx.cxx, ctx.std, empty), macros(ctx.cxx, ctx.std, version)
    if not predefined[ctx.profile.os] then
        raise("bundle: %s does not target the %s profile's system (no %s)", ctx.cxx, ctx.name, ctx.profile.os)
    end
    local library = {}
    for n, line in pairs(with_library) do
        if not predefined[n] then table.insert(library, line) end
    end
    local library_h, source = path.join(ctx.tmp, "library.h"), path.join(ctx.tmp, "head.cpp")
    io.writefile(library_h, table.concat(library, "\n") .. "\n", as_bytes)
    io.writefile(source, head .. "\n", as_bytes)
    local resolved, err = capture(ctx.cxx, {ctx.std, "-E", "-P", "-fdirectives-only", "-dD", "-ffreestanding",
                                            "-include", library_h, "-DCP_CHECKS=" .. ctx.checks, "-w", source})
    if not resolved then raise("bundle: %s cannot preprocess the inlined headers:\n%s", ctx.cxx, err) end
    return resolved, with_library
end

-- `line` without a trailing `//` comment and with runs of blanks squeezed, both outside literals.
local function squeeze(line)
    local out, i, n = {}, 1, #line
    while i <= n do
        local c = line:sub(i, i)
        if c == '"' or c == "'" then  -- a literal, to its closing quote
            local j = i + 1
            while j <= n and line:sub(j, j) ~= c do j = j + (line:sub(j, j) == "\\" and 2 or 1) end
            table.insert(out, line:sub(i, j))
            i = j + 1
        elseif c:match("%d") and not line:sub(i - 1, i - 1):match("[%w_]") then  -- a number: `'` separates digits
            local j = i
            while j <= n and line:sub(j, j):match("[%w_.']") do j = j + 1 end
            table.insert(out, line:sub(i, j - 1))
            i = j
        elseif c:match("[%a_]") then
            local j = line:find("[^%w_]", i) or n + 1
            table.insert(out, line:sub(i, j - 1))
            i = j
        elseif line:sub(i, i + 1) == "//" then
            break
        elseif c:match("%s") then
            local j = line:find("%S", i) or n + 1
            table.insert(out, " ")
            i = j
        else
            table.insert(out, c)
            i = i + 1
        end
    end
    return table.concat(out):match("^%s*(.-)%s*$")
end

-- 3. The resolved text without comments, blank lines, the standard library's macros and duplicate #includes; the
-- masked #includes restored.
function strip(resolved, with_library)
    local out, includes = {}, {}
    for line in resolved:gmatch("[^\n]+") do
        local s       = squeeze(line)
        local defined = s:match("^#%s*define%s+([%w_]+)") or s:match("^#%s*undef%s+([%w_]+)")
        local inc     = s:match("^CP_BUNDLE_INCLUDE%s*(.*)$")
        if inc then
            if not includes[inc] then
                includes[inc] = true
                table.insert(out, "#include " .. inc)
            end
        elseif s ~= "" and not (defined and with_library[defined]) then
            table.insert(out, s)
        end
    end
    return table.concat(out, "\n")
end

-- Drops CP_ macros nothing uses.
function drop_unused_macros(text)
    local lines = text:split("\n", {strict = true})
    repeat
        local dropped = false
        local all = table.concat(lines, "\n")
        for i = #lines, 1, -1 do
            local n = lines[i]:match("^#define%s+(CP_[%w_]+)")
            if n then
                local _, uses = all:gsub("%f[%w_]" .. n .. "%f[^%w_]", "")
                if uses == 1 then
                    table.remove(lines, i)
                    dropped = true
                end
            end
        end
    until not dropped
    return table.concat(lines, "\n")
end

-- The file clang parses in steps 4 and 5.
local function ast_file(ctx) return path.join(ctx.tmp, "ast.cpp") end

-- clang's JSON AST of `text`'s cp declarations, one decoded object per top-level match; nil and clang's errors if
-- it cannot parse it.
local function ast(text, ctx)
    local file = ast_file(ctx)
    io.writefile(file, text, as_bytes)
    local args = table.join(ctx.profile.clang or {}, {ctx.std, "-fsyntax-only", "-w", "-Xclang", "-ast-dump=json",
                                                      "-Xclang", "-ast-dump-filter=cp::", file})
    local dump, err = capture(ctx.clang, args)
    if not dump then return nil, err end
    local objects = {}
    for obj in dump:gmatch("({\n.-\n})\n") do table.insert(objects, json.decode(obj)) end
    return objects
end

-- `p` (a clang source location) as its expansion location.
local function at(p) return p.expansionLoc or p end

-- `pattern` with Lua's magic characters escaped.
local function literal(pattern) return (pattern:gsub("[%^%$%(%)%%%.%[%]%*%+%-%?]", "%%%0")) end

-- 4. `text` without the cp declarations nothing reaches by name (spec §14.2). Returns it, a summary and the kept
-- declarations by size; nil and a reason if clang cannot parse `text`.
function shake(text, ctx)
    local objects, err = ast(text, ctx)
    if not objects then return nil, "clang++ could not parse the bundle:\n" .. err end
    local file = ast_file(ctx)
    local decls = {}
    local function walk(ns)
        for _, c in ipairs(ns.inner or {}) do
            local r = c.range
            if not c.isImplicit and r and r.begin and r["end"] then
                local b, e = at(r.begin), at(r["end"])
                local here = not b.includedFrom and not e.includedFrom and (b.file == nil or b.file == file)
                if here and b.offset and e.offset then
                    if c.kind == "NamespaceDecl" then
                        walk(c)
                    else
                        local n = c.name and (c.name:match("^<deduction guide for ([%w_]+)>$") or
                                              c.name:match("([%w_~]+[^:]*)$"))
                        table.insert(decls, {kind = c.kind, name = n, b = b.offset + 1, e = e.offset + (e.tokLen or 0)})
                    end
                end
            end
        end
    end
    -- A std specialization for a cp type: from its `template` to its closing brace.
    local function foreign(t)
        local b = at(t.range.begin)
        if b.includedFrom or (b.file and b.file ~= file) or not b.offset then return end
        local start = text:sub(1, b.offset):match(".*()template")
        local open = text:find("{", b.offset + 1, true)
        if not start or not open then return end
        local depth, i = 0, open
        repeat
            local c = text:sub(i, i)
            if c == "{" then depth = depth + 1 elseif c == "}" then depth = depth - 1 end
            i = i + 1
        until depth == 0 or i > #text
        table.insert(decls, {kind = "ForeignSpecialization", name = t.name, b = start, e = i - 1})
    end
    for _, t in ipairs(objects) do
        if t.kind == "NamespaceDecl" and (t.loc == nil or t.loc.file == nil or t.loc.file == file) then
            walk(t)
        elseif t.kind == "CXXRecordDecl" and t.isImplicit and t.range then
            foreign(t)
        end
    end
    -- Outermost declarations only. A template and its specialization can share one range (a declared-only variable
    -- template): the template comes first, so the result does not depend on table.sort's order.
    local function specialization(d) return d.kind:find("Specialization") and 1 or 0 end
    table.sort(decls, function (x, y)
        if x.b ~= y.b then return x.b < y.b end
        if x.e ~= y.e then return x.e > y.e end
        return specialization(x) < specialization(y)
    end)
    local outer, reach = {}, 0
    for _, d in ipairs(decls) do
        if d.b > reach then
            table.insert(outer, d)
            reach = d.e
        end
    end
    -- Reachability by name, from everything outside cp's declarations.
    local used, names, pos, roots, classes = {}, {}, 1, {}, {}
    for _, d in ipairs(outer) do
        table.insert(roots, text:sub(pos, d.b - 1))
        pos = d.e + 1
        if d.name and (d.kind == "CXXRecordDecl" or d.kind == "ClassTemplateDecl") then classes[d.name] = true end
    end
    table.insert(roots, text:sub(pos))
    local function use(s) for id in s:gmatch("[%a_][%w_]*") do used[id] = true end end
    use(table.concat(roots, "\n"))
    local changed = true
    while changed do
        changed = false
        for _, d in ipairs(outer) do
            if not d.kept then
                local body = text:sub(d.b, d.e)
                local keep = d.name == nil or d.kind == "UsingDirectiveDecl" or d.kind == "StaticAssertDecl" or
                             used[d.name] or d.kind == "ForeignSpecialization"
                -- A specialization for a cp class nothing uses can never be picked; its other names it brings along.
                if keep and d.kind:find("Specialization") then
                    local pattern = (body:match("^(.-)[{=;]") or body):gsub("%s:%s.*$", "")  -- not the bases
                    for id in pattern:gmatch("[%a_][%w_]*") do
                        if classes[id] and id ~= d.name and not names[id] then keep = false break end
                    end
                end
                if not keep and d.name:startswith("operator") then
                    for id in body:gmatch("[%a_][%w_]*") do
                        if names[id] then keep = true break end
                    end
                end
                if keep then
                    d.kept, changed = true, true
                    use(body)
                    if d.name then names[d.name] = true end
                end
            end
        end
    end
    -- Cut last first, with each one's `;` and leading `[[attributes]]` / `__extension__` (outside clang's range).
    local result, dropped = text, 0
    for i = #outer, 1, -1 do
        local d = outer[i]
        if not d.kept then
            local b, e = d.b, d.e
            local after = result:find("[^ \n]", e + 1)
            if after and result:sub(after, after) == ";" then e = after end
            while true do
                local q = b
                while q > 1 and result:sub(q - 1, q - 1):match("[ \n]") do q = q - 1 end
                local ext = q - #"__extension__"
                if result:sub(ext, q - 1) == "__extension__" and not result:sub(ext - 1, ext - 1):match("[%w_]") then
                    b = ext
                else
                    if result:sub(q - 2, q - 1) ~= "]]" then break end
                    local open = result:sub(1, q - 1):match(".*()%[%[")
                    if not open then break end
                    b = open
                end
            end
            result = result:sub(1, b - 1) .. result:sub(e + 1)
            dropped = dropped + 1
        end
    end
    -- Drop emptied namespaces; restore those still named (`using namespace cp::aliases;`).
    local emptied = {}
    local function empty(ns)
        emptied[ns] = true
        return ""
    end
    repeat
        local n, m
        result, n = result:gsub("%f[%w_]inline namespace ([%w_:]+) {%s*}\n?", empty)
        result, m = result:gsub("%f[%w_]namespace ([%w_:]+) {%s*}\n?", empty)
    until n + m == 0
    result = result:gsub("\n\n+", "\n")
    local restore = {}
    for ns in pairs(emptied) do
        local named = "%f[%w_:]" .. literal(ns) .. "%f[^%w_:]"
        if not result:find("namespace " .. literal(ns) .. " {") and result:find(named) then
            table.insert(restore, "namespace " .. ns .. " {}")
        end
    end
    table.sort(restore)
    if #restore > 0 then result = table.concat(restore, "\n") .. "\n" .. result end
    local kept = {}
    for _, d in ipairs(outer) do
        if d.kept then table.insert(kept, {name = d.name or d.kind, size = d.e - d.b + 1}) end
    end
    table.sort(kept, function (x, y) return x.size > y.size end)
    return result, dropped .. " of " .. #outer .. " declarations dropped", kept
end

local functions = {FunctionDecl = true, CXXMethodDecl = true, CXXConstructorDecl = true, CXXConversionDecl = true,
                   CXXDestructorDecl = true}

-- 5. One round of dead-code removal on what clang instantiated: a function body nothing instantiates becomes `;`, an
-- unreferenced plain function or friend goes, an `if constexpr` branch no instantiation takes becomes `{}`, and a class
-- template nothing instantiates becomes a declaration. Returns the text and the number of edits; nil and a reason if
-- clang cannot parse `text`.
function prune(text, ctx)
    local objects, err = ast(text, ctx)
    if not objects then return nil, "clang++ could not parse the bundle:\n" .. err end
    local file = ast_file(ctx)
    -- 0-based offset, token length and whether it is outside `file`.
    local function off(p)
        p = at(p)
        return p.offset, p.tokLen or 1, p.includedFrom ~= nil or (p.file ~= nil and p.file ~= file)
    end
    local function body_of(n)
        for _, c in ipairs(n.inner or {}) do
            if c.kind == "CompoundStmt" then return c end
        end
    end
    local defs, pattern_ifs, taken, live, refd, templates = {}, {}, {}, {}, {}, {}
    local in_template, friend_ctx = false, nil

    -- Records each `if constexpr` of a pattern with its branches, or which branches an instantiation keeps.
    local function scan_ifs(stmt, instance)
        if stmt.kind == "IfStmt" and stmt.isConstexpr then
            local where = off(stmt.range.begin)
            local kids, first = {}, 2 + (stmt.hasInit and 1 or 0) + (stmt.hasVar and 1 or 0)  -- after the condition
            for i = first, math.min(first + 1, #(stmt.inner or {})) do table.insert(kids, stmt.inner[i]) end
            if instance then
                taken[where] = taken[where] or {false, false}
                for i, k in ipairs(kids) do
                    if not (k.kind == "CompoundStmt" and not k.inner) then taken[where][i] = true end
                end
            else
                local branches = {}
                for _, k in ipairs(kids) do
                    local b0 = off(k.range.begin)
                    local e0, l0 = off(k.range["end"])
                    table.insert(branches, {b = b0, e = e0 + l0, empty = k.kind == "CompoundStmt" and not k.inner,
                                            statement = k.kind ~= "CompoundStmt"})
                end
                pattern_ifs[where] = branches
            end
        end
        for _, c in ipairs(stmt.inner or {}) do
            if type(c) == "table" then scan_ifs(c, instance) end
        end
    end

    local function visit_fn(n, concrete, outer)
        local r = n.range
        if not r or not r.begin then return end
        local db, _, outside = off(r.begin)
        if outside or not db then return end
        if n.isUsed or n.isReferenced then refd[db] = true end
        local b = body_of(n)
        if not b then return end
        local bb = off(b.range.begin)
        local be, bl = off(b.range["end"])
        if concrete and (in_template or n.isUsed or n.isImplicit) then live[bb] = true end  -- instances share offsets
        if in_template then scan_ifs(b, concrete) end
        if not concrete and n.isImplicit then live[bb] = true end
        if not n.isImplicit and not defs[bb] then
            local ob = outer and off(outer.range.begin) or db
            local oe, ol = off((outer or n).range["end"])
            local init = false
            for _, c in ipairs(n.inner or {}) do
                if c.kind == "CXXCtorInitializer" then init = true end
            end
            defs[bb] = {kind = n.kind, name = n.name or "", decl = db, body = {bb, be + bl}, whole = {ob, oe + ol},
                        virtual = n.virtual, plain = concrete and not in_template, init = init,
                        friend = (outer and outer.kind == "FriendDecl") or friend_ctx ~= nil}
        end
    end

    local function walk(n, concrete, in_spec, ns_scope)
        local k = n.kind
        if k == "ClassTemplateDecl" then
            local record, instantiated = nil, false
            for _, c in ipairs(n.inner or {}) do
                if c.kind == "CXXRecordDecl" then
                    record = c
                    walk(c, false, in_spec)
                elseif c.kind == "ClassTemplateSpecializationDecl" then
                    instantiated = true
                    walk(c, true, true)
                end
            end
            if ns_scope and not instantiated and record and record.completeDefinition and record.loc then
                local b, _, outside = off(n.range.begin)
                local name, len = off(record.loc)
                local e, l = off(n.range["end"])
                if not outside and b and name then table.insert(templates, {b = b, name_end = name + len, e = e + l}) end
            end
            return
        end
        if k == "ClassTemplatePartialSpecializationDecl" then
            for _, c in ipairs(n.inner or {}) do walk(c, false, in_spec) end
            return
        end
        if k == "FunctionTemplateDecl" then
            local index = 0
            for _, c in ipairs(n.inner or {}) do
                if functions[c.kind] then
                    in_template = true
                    visit_fn(c, concrete and index > 0, index == 0 and (friend_ctx or n) or nil)
                    in_template = false
                    if index == 0 and (n.isUsed or n.isReferenced) and c.range then refd[off(c.range.begin)] = true end
                    index = index + 1
                end
            end
            return
        end
        if k == "FriendDecl" then
            for _, c in ipairs(n.inner or {}) do
                if functions[c.kind] then
                    in_template = in_spec or not concrete
                    visit_fn(c, concrete, n)
                    in_template = false
                else
                    friend_ctx = n
                    walk(c, concrete, in_spec)
                    friend_ctx = nil
                end
            end
            return
        end
        if functions[k] then
            in_template = in_spec or not concrete
            visit_fn(n, concrete, nil)
            in_template = false
            return
        end
        if k == "NamespaceDecl" or k == "CXXRecordDecl" or k == "ClassTemplateSpecializationDecl" or
           k == "LinkageSpecDecl" then
            for _, c in ipairs(n.inner or {}) do
                walk(c, concrete, in_spec or k == "ClassTemplateSpecializationDecl", k == "NamespaceDecl")
            end
        end
    end
    for _, o in ipairs(objects) do walk(o, true, false, true) end

    -- Edits as {begin, end, replacement}: 0-based, end exclusive. "" also takes a following `;`.
    local edits = {}
    for bb, d in pairs(defs) do
        if not live[bb] and not d.virtual then
            local unreferenced = not refd[d.decl]
            local deletable = d.kind ~= "CXXConstructorDecl" and d.kind ~= "CXXDestructorDecl" and
                              not d.name:startswith("operator=")
            if d.friend then
                if unreferenced then table.insert(edits, {d.whole[1], d.whole[2], ""}) end
            elseif d.plain then
                if unreferenced and deletable then table.insert(edits, {d.whole[1], d.whole[2], ""}) end
            elseif not (d.kind == "CXXConstructorDecl" and d.init) then
                table.insert(edits, {d.body[1], d.body[2], ";"})
            end
        end
    end
    for where, branches in pairs(pattern_ifs) do
        local t = taken[where]
        if t then  -- else no instance reached it: its function is handled above
            for i, br in ipairs(branches) do
                if not t[i] and not br.empty then
                    local e = br.e
                    if br.statement then  -- a lone statement's range stops before its `;`
                        local semi = text:find("[^%s]", e + 1)
                        if semi and text:sub(semi, semi) == ";" then e = semi end
                    end
                    table.insert(edits, {br.b, e, "{}"})
                end
            end
        end
    end
    for _, t in ipairs(templates) do
        table.insert(edits, {t.b, t.e, text:sub(t.b + 1, t.name_end) .. ";", declaration = true})
    end
    -- Outermost edits only, applied last first.
    table.sort(edits, function (x, y) return x[1] < y[1] or (x[1] == y[1] and x[2] > y[2]) end)
    local kept, reach = {}, -1
    for _, e in ipairs(edits) do
        if e[1] >= reach then
            table.insert(kept, e)
            reach = e[2]
        end
    end
    local result = text
    for i = #kept, 1, -1 do
        local b, e, rep = kept[i][1], kept[i][2], kept[i][3]
        if rep == "" or kept[i].declaration then  -- its own `;` too
            local semi = result:find("[^%s]", e + 1)
            if semi and result:sub(semi, semi) == ";" then e = semi end
        end
        result = result:sub(1, b) .. rep .. result:sub(e + 1)
    end
    return result, #kept
end

-- 6. Whether `text` compiles and, with a sample, behaves as `ctx.reference`: true, or false and why.
function verify(text, ctx)
    local source = path.join(ctx.tmp, "submit.cpp")
    local binary = path.join(ctx.tmp, "submit" .. ctx.profile.exe)
    io.writefile(source, text, as_bytes)
    local _, err = capture(ctx.cxx, table.join({ctx.std, "-O2", "-o", binary, source}, ctx.profile.link or {}))
    if err then return false, "does not compile:\n" .. err end
    if ctx.reference then
        local got = run(binary, ctx)
        if got.code ~= ctx.reference.code then
            return false, string.format("exits with %d, not %d, on %s", got.code, ctx.reference.code, ctx.sample)
        end
        if got.out ~= ctx.reference.out then return false, "gives different output on " .. ctx.sample end
    end
    return true
end

-- `program`'s exit code and output on the sample.
function run(program, ctx)
    local out = path.join(ctx.tmp, "out.txt")
    local opt = {stdin = ctx.sample, stdout = out, stderr = path.join(ctx.tmp, "err.txt"), envs = {WINEDEBUG = "-all"},
                 try = true}
    local code = ctx.runner and os.execv(ctx.runner, {program}, opt) or os.execv(program, {}, opt)
    return {code = code, out = io.readfile(out, as_bytes)}
end

-- The unbundled solution's exit code and output on the sample.
function reference(input, include, ctx)
    local binary = path.join(ctx.tmp, "unbundled" .. ctx.profile.exe)
    local _, err = capture(ctx.cxx, table.join({ctx.std, "-O2", "-I" .. include, "-DCP_CHECKS=" .. ctx.checks,
                                                "-o", binary, input}, ctx.checks == "1" and {"-D_GLIBCXX_ASSERTIONS"} or {},
                                               ctx.profile.link or {}))
    if err then raise("bundle: the solution does not compile:\n%s", err) end
    return run(binary, ctx)
end
