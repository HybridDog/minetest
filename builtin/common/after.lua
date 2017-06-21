local jobs = {}
local time = 0.0
local inafter_jobs = {}
local in_after = false

local function sortfunc(a, b)
	return a.expire < b.expire
end

-- Adds the inafter_jobs' fields to jobs
local function merge_jobs()
	-- Sort inafter_jobs with quicksort
	table.sort(inafter_jobs, sortfunc)

	-- Merge two sorted tables
	local i = 1
	local j = 1
	local next_jobs = {}
	local jbcnt = #jobs
	local inacnt = #inafter_jobs
	local nxtcnt = inacnt + jbcnt
	for k = 1, nxtcnt do
		if jobs[i].expire <= inafter_jobs[j].expire then
			next_jobs[k] = jobs[i]
			i = i+1
			if i > jbcnt then
				-- Add remaining fresh jobs
				k = k - j + 1
				for o = j, inacnt do
					next_jobs[k + o] = inafter_jobs[o]
				end
				break
			end
		else
			next_jobs[k] = inafter_jobs[j]
			j = j+1
			if j > inacnt then
				-- Add remaining jobs
				k = k - i + 1
				for o = i, jbcnt do
					next_jobs[k + o] = jobs[o]
				end
				break
			end
		end
	end
	inafter_jobs = {}
	jobs = next_jobs
end

core.register_globalstep(function(dtime)
	time = time + dtime

	if not jobs[1] or jobs[1].expire < time then
		return
	end

	in_after = true
	-- Call expired functions, except freshly added ones
	local job = jobs[1]
	repeat
		core.set_last_run_mod(job.mod_origin)
		job.func(unpack(job.arg))
		table.remove(jobs, 1)
		job = jobs[1]
	until not job or time < job.expire
	in_after = false

	if inafter_jobs[1] and job then
		-- core.after was called in a core.after function
		merge_jobs()
	end
end)

function core.after(after, func, ...)
	assert(tonumber(after) and type(func) == "function",
		"Invalid core.after invocation")
	local expire = time + after
	local data = {
		func = func,
		expire = expire,
		arg = {...},
		mod_origin = core.get_last_run_mod()
	}
	if in_after then
		-- Add the job without sorting to this table
		inafter_jobs[#inafter_jobs+1] = data
	else
		-- Add the job with insertion sort
		local next_bigger = #jobs+1
		for p = 1, #jobs do
			if jobs[p].expire > expire then
				next_bigger = p
				break
			end
		end
		table.insert(jobs, next_bigger, data)
	end
end
