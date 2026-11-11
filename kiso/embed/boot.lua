kiso.callbacks = {}
kiso.callbacks.onExit = {}
kiso.callbacks.update = {}

kiso.enum = {}

kiso.enum.exitRequestType = {
    Interrupt = 2,
    Terminate = 15,
}

function kiso.callbacks.onExitRequest(requestType)
    if requestType == kiso.enum.exitRequestType.Terminate then
        print("You dare tried terminate me")
    end
    kiso.exit(0)
end

function kiso.callbacks.onError(msg)
    local notNilMsg = msg or "There is no error message for this!"
    print("[ERROR] " .. notNilMsg)
    kiso.exit(1)
end