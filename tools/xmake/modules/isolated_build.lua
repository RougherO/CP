-- Configures and builds the project in a directory of its own, leaving the user's `xmake f` settings untouched.

-- Options that run xmake in `dir`'s own configuration.
local function context(dir)
    return {curdir = os.projectdir(), envs = {XMAKE_CONFIGDIR = path.join(dir, "config"), CP_ISOLATED_BUILD = "1"}}
end

-- Configures `dir` with the `xmake f` arguments `config`, builds `group`; returns the options for later xmake calls.
function build(dir, config, group)
    local opt = context(dir)
    os.execv(os.programfile(), table.join({"f", "--builddir=" .. dir}, config), opt)
    os.execv(os.programfile(), {"build", "-q", "-g", group}, opt)
    return opt
end

-- The release build with the benchmarks in build/bench/<toolchain>, `group` built; returns the options and the directory.
function bench(toolchain, group)
    local dir = path.join(os.projectdir(), "build", "bench", toolchain)
    return build(dir, {"-m", "release", "--toolchain=" .. toolchain, "--bench=y"}, group), dir
end

-- The absolute paths of the built `targets`, in the build that `opt` runs in.
function targetfiles(opt, targets)
    local out = os.iorunv(os.programfile(), table.join({"l", path.join(os.scriptdir(), "targetfile.lua")}, targets), opt)
    local files = {}
    for _, line in ipairs(out:split("\n")) do
        table.insert(files, path.absolute(line:trim(), os.projectdir()))
    end
    return files
end
