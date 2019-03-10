local jobs = core.create_binary_heap(function(a, b)
	return a.expire < b.expire
end)
local time = 0.0

core.register_globalstep(function(dtime)
	time = time + dtime

	-- Collect all executable jobs before executing so that we miss any new
	-- timers added by a timer callback.
	local to_execute = {}
	while not jobs:is_empty() and time >= jobs:peek().expire do
		to_execute[#to_execute+1] = jobs:take()
	end
	for i = 1, #to_execute do
		local job = to_execute[i]
		core.set_last_run_mod(job.mod_origin)
		job.func(unpack(job.arg))
	end
end)

function core.after(after, func, ...)
	after = tonumber(after)
	assert(after and after == after and type(func) == "function",
		"Invalid core.after invocation")
	jobs:add({
		func = func,
		expire = time + after,
		arg = {...},
		mod_origin = core.get_last_run_mod()
	})
end
