-- xmake l targetfile.lua <target>...: prints each target's built file, one per line.
import("core.project.config")
import("core.project.project")

function main(...)
    config.load()
    for _, name in ipairs({...}) do print(project.target(name):targetfile()) end
end
