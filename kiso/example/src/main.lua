print("Hello, world!")

local version = kiso.getVersion()
print("KISO version: " .. version.major .. "." .. version.minor .. "." .. version.patch)

local args = kiso.getArgs()
print("Program arguments")
for i,v in ipairs(args) do
    print("[" .. i .. "] = " .. v)
end

table.insert(kiso.callbacks.onExit, function(code)
    print("1 | Exiting with code: " .. code)
end)

table.insert(kiso.callbacks.onExit, function(code)
    print("2 | Exiting with code: " .. code)
end)

local num = 0
table.insert(kiso.callbacks.update, function(deltaTime)
    kiso.sleep(0)
    print("FPS:", 1/deltaTime)
    print("DELTA:", deltaTime)
    print("MONO:", kiso.getMonotonicTime())
    num = num + 1
end)

kiso.setLimitUpdateRate(30)