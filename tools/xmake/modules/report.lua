-- The output of cp's tasks, in xmake's style: `<task>: …` lines, yellow warnings, errors through raise() (red), and
-- a green `<task> ok, spent <s>s` last, as `xmake build` ends.

local current, clock

-- Starts timing `task`, the name every line carries.
function start(task)
    current, clock = task, os.mclock()
end

-- Seconds since start(), or since `since` (an os.mclock() value).
function seconds(since)
    return (os.mclock() - (since or clock)) / 1000
end

-- A line of progress or results.
function info(fmt, ...)
    print(current .. ": " .. fmt, ...)
end

-- A problem that does not stop the task.
function warning(fmt, ...)
    cprint("${color.warning}warning: ${clear}" .. current .. ": " .. fmt, ...)
end

-- A passed (`ok`) or failed check: a green PASS or red FAIL before `fmt`.
function check(ok, fmt, ...)
    cprint((ok and "${color.success}PASS${clear} " or "${color.failure}FAIL${clear} ") .. fmt, ...)
end

-- The last line of a task that succeeded.
function done()
    cprint("${color.success}%s ok, spent %ss", current, seconds())
end
