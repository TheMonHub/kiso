--- @meta

--- KISO namespace.
--- @class kiso
kiso = {}

--- Contains all the global callbacks that can register to. Only the main thread has access to registering/unregistering
--- callbacks.
---
--- You can use `table.insert(list, value)` to register a function into the callbacks.
--- You can also use `table.remove(list, pos)` to unregister a function or clear the table to clear all the registered
--- callbacks. For a callback that only accept one function, you can override the variable itself to define what it
--- should do.
--- @class kisoCallbacks
kiso.callbacks = {}

--- @type fun(deltaTime: number)[]
--- Every function in this table gets called from the first to last order repeatedly by KISO right after the main.lua
--- finished executing. The callbacks will not be called if the program is exiting. deltaTime unit is in second.
kiso.callbacks.update = {}

--- @type fun(code: integer)[]
--- Every function in this table gets called from the last to first order when `kiso.exit(code)` is called.
--- The function can accept an argument being the exit code of the program.
--- @see kiso.exit
kiso.callbacks.onExit = {}

--- This function runs after the program being requested to exit.
--- The most typical way to use it is to run `kiso.exit(code)` if nothing is stopping the exit request.
--- If this is not defined then the program will automatically exit upon a request.
--- @param requestType kisoExitRequestTypeEnum Type of the exit request.
function kiso.callbacks.onExitRequest(requestType) end

--- This function runs after lua interpreter encountered an error or an invalid use of KISO API.
--- It is the function's responsibility to run one of the available in this function because KISO will not automatically
--- exit the program.
--- If any error occurred in this function, KISO will automatically run `kiso.abort(code)`.
--- @param errorMessage string Error message.
function kiso.callbacks.onError(errorMessage) end

--- @alias kisoVersion { ["major"]: integer, ["minor"]: integer, ["patch"]: integer, ["gitCommitHash"]: string | nil, ["gitBranch"]: string | nil }

--- Gets the version of KISO this project was built/run from.
--- @return kisoVersion
function kiso.getVersion() end

--- @alias kisoProgramArgs string[]

--- Gets the program arguments that were passed typically through command lines.
--- @return kisoProgramArgs
function kiso.getArgs() end

--- Sleeps and halt the current thread for a specified seconds
--- @param timeInSeconds number
function kiso.sleep(timeInSeconds) end

--- Gets a Monotonic system-wide clock time.
--- @return number monotonicTime
function kiso.getMonotonicTime() end

--- Set how fast does the callbacks get called per second. KISO will not limit the rate by default, consuming CPU
--- and resources. It is advised to set a limit to this. This will apply to every callbacks as every callbacks
--- are handled right before the update callback in the main thread except for a certain callback such as `onError`.
--- Use `0` for disabling the limit.
--- @param rate integer Limit of the callbacks rate per second.
--- @see kiso.getLimitCallbacksRate
function kiso.setLimitCallbacksRate(rate) end

--- Set how fast does the callbacks get called per second.
--- @return integer rate Limit of the callbacks rate per second.
--- @see kiso.setLimitCallbacksRate
function kiso.getLimitCallbacksRate() end

--- Stops every ongoing operations and calls every registered onExit callbacks then close the program with the specified
--- exit code. This function will wait until every ongoing operation, and onExit callbacks finished running.
--- This function will never return.
--- @param code integer Exit code of the program.
--- @see kiso.callbacks.onExit
--- @see kiso.abort to exit without calling any onExit functions
function kiso.exit(code) end

--- Does the same as `kiso.exit(code)` but will not call functions registered inside onExit. However KISO will still
--- wait for all ongoing operation to finish.
--- This function will never return.
--- @param code integer Exit code of the program.
--- @see kiso.callbacks.onExit
function kiso.abort(code) end

--- Kills the program immediately without waiting for anything and everything. This is a very destructive function when
--- used wrong. This can and will interrupt and abruptly stop any ongoing operation without cleaning up, even for KISO
--- itself too.
--- @param code integer Exit code of the program.
--- @see kiso.abort
function kiso.kill(code) end

--- KISO enumerators.
--- @class kisoEnum
kiso.enum = {}

--- Different type of the exit request.
--- @enum kisoExitRequestTypeEnum
kiso.enum.exitRequestType = {
    --- Interruption sent by the user. This is an equivalent to SIGINT in Unix and such.
    Interrupt = 2,
    --- Termination request. This is an equivalent to SIGTERM in Unix and such.
    Terminate = 15,
}