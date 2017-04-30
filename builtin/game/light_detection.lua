
-- See daynightratio.h:23
local dnrvalues = {
	{4250 + 125, 150},
	{4500 + 125, 150},
	{4750 + 125, 250},
	{5000 + 125, 350},
	{5250 + 125, 500},
	{5500 + 125, 675},
	{5750 + 125, 875},
	{6000 + 125, 1000},
	{6250 + 125, 1000},
}
local function time_to_daynight_ratio(tod)
	tod = tod * 24000
	assert(tod >= 0 and tod <= 24000, "invalid time of day")
	if tod > 12000 then
		tod = 24000 - tod
	end
	for i = 1,#dnrvalues do
		if dnrvalues[i][1] > tod then
			if i == 1 then
				return dnrvalues[1][2]
			end
			local td0 = dnrvalues[i][1] - dnrvalues[i - 1][1]
			local f = (tod - dnrvalues[i - 1][1]) / td0
			return f * dnrvalues[i][2] + (1.0 - f) * dnrvalues[i - 1][2]
		end
	end
	return 1000
end

-- returns the daylight if it's not overwhelmed by artificial light
local function get_valid_daylight(param1)
	local daylight = param1 % 16
	if daylight == 0 then
		return 0
	end
	local nightlight = math.floor(param1 / 16)
	if daylight > nightlight then
		return daylight
	end
end

-- tells whether the node propagates light to the neighbour nodes
local function propagates_light(node)
	local def = core.registered_nodes[node.name]
	return def and def.sunlight_propagates
end

-- the offsets for neighbouring nodes with given order
local dirs = {
	{x=-1, y=0, z=0}, {x=1, y=0, z=0}, {x=0, y=0, z=-1}, {x=0, y=0, z=1},
	{x=0, y=-1, z=0}, {x=0, y=1, z=0}
}

-- walks nodes to find the daylight when it's not known
local function find_sunlight(pos)
	local found_light = 0
	local todo = {{pos, 0}}
	local sp = 1
	local dists = {}
	dists[core.hash_node_position(pos)] = 0
	while sp > 0 do
		local pos, dist = unpack(todo[sp])
		sp = sp - 1
		dist = dist + 1
		-- 15 distance would mean light 0
		if dist < 15 then
			for i = 1, 6 do
				local p = vector.add(pos, dirs[i])
				local h = core.hash_node_position(p)
				if not dists[h]
				or dist < dists[h] then
					-- position to walk
					local node = core.get_node(p)
					if propagates_light(node) then
						-- can walk here
						dists[h] = dist
						local light = get_valid_daylight(node.param1)
						if light then
							-- found a valid daylight
							local light = light - dist
							if light > found_light then
								-- update the known daylight at the start pos
								found_light = light
							end
						else
							-- no valid one found, walk it's neighbours
							sp = sp + 1
							todo[sp] = {p, dist}
						end
					else
						-- avoid testing propagation at the same position again
						dists[h] = -1
					end
				end
			end
		end
	end
	return found_light
end

-- See light.h:119
function core.get_natural_light(pos, tod)
	tod = tod or core.get_timeofday()
	local param1 = core.get_node(pos).param1
	local sun_factor = time_to_daynight_ratio(tod)
	local daylight = param1 % 16
	local nightlight = math.floor(param1 / 16)
	if daylight == nightlight then
		daylight = find_sunlight(pos)
	end
	return math.min(math.floor(sun_factor * daylight / 1000), 15)
end

function core.get_artificial_light(param1)
	return math.floor(param1 / 16)
end
