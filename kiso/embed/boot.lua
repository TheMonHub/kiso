kiso.callbacks = {}
kiso.callbacks.onExit = {}
kiso.callbacks.update = {}

kiso.enum = {}

package.cpath = package.cpath:gsub(";.*", "")
package.path = "./?.lua;./?.luac"

kiso.enum.exitRequestType = {
    Interrupt = 2,
    Terminate = 15,
}

function kiso.callbacks.onExitRequest(requestType)
    kiso.exit(0)
end

function kiso.callbacks.onError(msg)
    local notNilMsg = msg or ""
    print("[ERROR] " .. notNilMsg)
    kiso.exit(1)
end