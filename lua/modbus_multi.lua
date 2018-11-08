-----------------------------------------------------------------------
-- 使用说明：                                                        --
-- 用户需要自行完成一下2个函数的实现                                 --
-- 1、定时下发数据任务初始化函数：device_timer_init(dev)【可选】     --
-- 2、对设备上传数据进行解析（包括心跳等）：device_data_analyze(dev) --
-----------------------------------------------------------------------


-------------------------------------------------------------------------------
-- 注册C函数                                                                 --
-- u2f 将32位整型内存数据转换为浮点数（不同于值转换）                        --
-- 类似C代码 *(float*)(&u)                                                   --
-- function u2f(u)                                                           --
-- @param   u   number   整数值                                              --
-- @return  成功返回浮点数值，否则返回nil                                    --
-- @example local u = 123                                                    --
--          local f = u2f( 123 )                                             --
--                                                                           --
-- time 获取时间戳，距离（00:00:00 UTC, January 1, 1970）的毫秒数            --
-- function time()                                                           --
-- @return  返回当前时间戳                                                   --
-- @example local t = time()                                                 --
--                                                                           --
-- year 获取年（year-1900）                                                  --
-- function year(t)                                                          --
-- @param   t   number   时间戳，距离（00:00:00 UTC, January 1, 1970）的秒数 --
-- @return  返回年                                                           --
-- @example local y = year( t )                                              --
--                                                                           --
-- month 获取月（0-11）                                                      --
-- function month(t)                                                         --
-- @param   t   number   时间戳，距离（00:00:00 UTC, January 1, 1970）的秒数 --
-- @return  返回月                                                           --
-- @example local m = month( t )                                             --
--                                                                           --
-- day 获取日（1-31）                                                        --
-- function day(t)                                                           --
-- @param   t   number   时间戳，距离（00:00:00 UTC, January 1, 1970）的秒数 --
-- @return  返回月                                                           --
-- @example local d = day( t )                                               --
--                                                                           --
-- hour 获取时（0-23）                                                       --
-- function hour(t)                                                          --
-- @param   t   number   时间戳，距离（00:00:00 UTC, January 1, 1970）的秒数 --
-- @return  返回时                                                           --
-- @example local h = hour( t )                                              --
--                                                                           --
-- minute 获取分（0-59）                                                     --
-- function minute(t)                                                        --
-- @param   t   number   时间戳，距离（00:00:00 UTC, January 1, 1970）的秒数 --
-- @return  返回分                                                           --
-- @example local m = minute( t )                                            --
--                                                                           --
-- second 获取秒（0-59）                                                     --
-- function second(t)                                                        --
-- @param   t   number   时间戳，距离（00:00:00 UTC, January 1, 1970）的秒数 --
-- @return  返回秒                                                           --
-- @example local m = second( t )                                            --
-------------------------------------------------------------------------------

--------------------------------------------------------
-- 将bytes string转换hex string                       --
-- @param   s   string   bytes string                 --
-- @return  返回hex string，类似"0A0B0C0D..."         --
-- @example local hex = to_hex("\2\2\0\150\0\37\206") --
--------------------------------------------------------
function to_hex(s)
	local i
	local t

	t={s:byte(1,s:len())}
	for i=1,#t do
		t[i]=string.format('%02X',t[i])
	end

	return table.concat(t)
end

-----------------------------------------------
-- 将object序列化成字符串                    --
-- @param   o   boolean|number|string|table  --
-- @return  返回序列化string                 --
-- @example local str = to_str({x=100})      --
-----------------------------------------------
function to_str(o)
	local i=1
	local t={}
	local f

	f=function(x)
		local y=type(x)
		if y=="number" then
			t[i]=x
			i=i+1
		elseif y=="boolean" then
			t[i]=tostring(x)
			i=i+1
		elseif y=="string" then
			t[i]="\""
			t[i+1]=x
			t[i+2]="\""
			i=i+3
		elseif y=="table" then
			t[i]="{"
			i=i+1

			local z=true
			for k,v in pairs(x) do
				if z then
					z=false
					t[i]="\""
					t[i+1]=k
					t[i+2]="\""
					t[i+3]=":"
					i=i+4
					f(v)
				else
					t[i]=","
					t[i+1]="\""
					t[i+2]=k
					t[i+3]="\""
					t[i+4]=":"
					i=i+5
					f(v)
				end
			end

			t[i]="}"
			i=i+1
		else
			t[i]="nil"
			i=i+1
		end
	end
	f(o)

	return table.concat(t)
end

----------------------------------------------------------------------------------------------------------
-- 添加值数据点到table中                                                                                --
-- @param   t   table                                                                                   --
--          i   string                      数据流或数据流模板名称                                      --
--          a   number                      毫秒级时间戳，距离（00:00:00 UTC, January 1, 1970）的毫秒； --
--                                          如果值为0，表示使用当前时间                                 --
--          v   boolean|number|string|table 布尔值、数值、字符串、json                                  --
--          c   string                      用于标识数据点归属(设备AuthCode,可选)                       --
--                                          如果值为“”或nil，表示数据点归属建立TCP连接的设备            --
-- @return  成功返回true，否则返回false                                                                 --
-- @example local ok = add_val(t,"dsname",0,100)                                                        --
----------------------------------------------------------------------------------------------------------
function add_val(t, i, a, v, c)
	if type(t)~="table" then
		return false
	elseif type(i)~="string" then
		return false
	elseif type(a)~="number" then
		return false
	else
		local o = type(v)
		if o~="boolean" and o~="number" and o~="string" and o~="table" then
			return false
		end

		local n = {i=i,v=v}
		if a~=0 and a~=nil then
			n["a"]=a
		end
		if c~=nil then
			n["c"]=c
		end

		-- list push_back --
		if t.h==nil then
			t.h={nil,n}
			t.t=t.h
		else
			t.t[1]={nil,n}
			t.t=t.t[1]
		end
	end

	return true
end

----------------------------------------------------------------------------------------------------------
-- 添加二进制数据点到table中                                                                            --
-- @param   t   table                                                                                   --
--          i   string                      数据流或数据流模板名称                                      --
--          a   number                      毫秒级时间戳，距离（00:00:00 UTC, January 1, 1970）的毫秒； --
--                                          如果值为0，表示使用当前时间                                 --
--          b   string                      二进制数据（hex string），类似"0A0B0C0D..."                 --
--          d   boolean|number|string|table 用于描述b（可选）；数值、字符串、json                       --
--          c   string                      用于标识数据点归属(设备AuthCode,可选)                       --
--                                          如果值为“”或nil，表示数据点归属建立TCP连接的设备            --
-- @return  成功返回true，否则返回false                                                                 --
-- @example local ok = add_val(t,"dsname",0,"0A0B0C0D...",{...})                                        --
----------------------------------------------------------------------------------------------------------
function add_bin(t, i, a, b, d)
	if type(t)~="table" then
		return false
	elseif type(i)~="string" then
		return false
	elseif type(a)~="number" then
		return false
	elseif type(b)~="string" then
		return false
	else
		local o=type(d)
		if o~="nil" and o~="string" and o~="table" then
			return false
		end

		local n={i=i,b=to_hex(b)}
		if a~=0 and a~=nil then
			n["a"]=a
		end
		if d~=nil then
			n["d"]=d
		end
		if c~=nil then
			n["c"]=c
		end

		-- list push_back --
		if t.h==nil then
			t.h={nil,n}
			t.t=t.h
		else
			t.t[1]={nil,n}
			t.t=t.t[1]
		end
	end

	return true
end

--------------------------------------------------------------
-- 将table序列化成json字符串                                --
-- @param   t   table   通过add_val、add_bin构建起来的table --
-- @return  返回序列化json字符串                            --
-- @example local json = to_json(t)                         --
--------------------------------------------------------------
function to_json(t)
	local i=1
	local o={}
	local n

	o[i]="["
	i=i+1
	n=t.h
	while n~=nil do
		if n[2]~=nil then
			o[i]=to_str(n[2])
			i=i+1
		end

		n=n[1]
		if n~=nil then
			o[i]=","
			i=i+1
		end
	end
	o[i]="]"

	return table.concat(o)
end

------------------------------------
-- begin-添加用户自定义值或函数等 --

local hi={
0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,
0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,
0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,
0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,
0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,
0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,
0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,
0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,
0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,
0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,
0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,
0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,
0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x01,0xC0,
0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,
0xC0,0x80,0x41,0x00,0xC1,0x81,0x40,0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,
0x00,0xC1,0x81,0x40,0x01,0xC0,0x80,0x41,0x01,0xC0,0x80,0x41,0x00,0xC1,0x81,
0x40
}
local lo = {
0x00,0xC0,0xC1,0x01,0xC3,0x03,0x02,0xC2,0xC6,0x06,0x07,0xC7,0x05,0xC5,0xC4,
0x04,0xCC,0x0C,0x0D,0xCD,0x0F,0xCF,0xCE,0x0E,0x0A,0xCA,0xCB,0x0B,0xC9,0x09,
0x08,0xC8,0xD8,0x18,0x19,0xD9,0x1B,0xDB,0xDA,0x1A,0x1E,0xDE,0xDF,0x1F,0xDD,
0x1D,0x1C,0xDC,0x14,0xD4,0xD5,0x15,0xD7,0x17,0x16,0xD6,0xD2,0x12,0x13,0xD3,
0x11,0xD1,0xD0,0x10,0xF0,0x30,0x31,0xF1,0x33,0xF3,0xF2,0x32,0x36,0xF6,0xF7,
0x37,0xF5,0x35,0x34,0xF4,0x3C,0xFC,0xFD,0x3D,0xFF,0x3F,0x3E,0xFE,0xFA,0x3A,
0x3B,0xFB,0x39,0xF9,0xF8,0x38,0x28,0xE8,0xE9,0x29,0xEB,0x2B,0x2A,0xEA,0xEE,
0x2E,0x2F,0xEF,0x2D,0xED,0xEC,0x2C,0xE4,0x24,0x25,0xE5,0x27,0xE7,0xE6,0x26,
0x22,0xE2,0xE3,0x23,0xE1,0x21,0x20,0xE0,0xA0,0x60,0x61,0xA1,0x63,0xA3,0xA2,
0x62,0x66,0xA6,0xA7,0x67,0xA5,0x65,0x64,0xA4,0x6C,0xAC,0xAD,0x6D,0xAF,0x6F,
0x6E,0xAE,0xAA,0x6A,0x6B,0xAB,0x69,0xA9,0xA8,0x68,0x78,0xB8,0xB9,0x79,0xBB,
0x7B,0x7A,0xBA,0xBE,0x7E,0x7F,0xBF,0x7D,0xBD,0xBC,0x7C,0xB4,0x74,0x75,0xB5,
0x77,0xB7,0xB6,0x76,0x72,0xB2,0xB3,0x73,0xB1,0x71,0x70,0xB0,0x50,0x90,0x91,
0x51,0x93,0x53,0x52,0x92,0x96,0x56,0x57,0x97,0x55,0x95,0x94,0x54,0x9C,0x5C,
0x5D,0x9D,0x5F,0x9F,0x9E,0x5E,0x5A,0x9A,0x9B,0x5B,0x99,0x59,0x58,0x98,0x88,
0x48,0x49,0x89,0x4B,0x8B,0x8A,0x4A,0x4E,0x8E,0x8F,0x4F,0x8D,0x4D,0x4C,0x8C,
0x44,0x84,0x85,0x45,0x87,0x47,0x46,0x86,0x82,0x42,0x43,0x83,0x41,0x81,0x80,
0x40
}

------------------
-- modbus crc16 --
------------------
function crc16(s)
	local i = 0xff
	local o = 0xff
	local t = {s:byte(1,-3)}
	local j,p=s:byte(-2,-1)

	for x=1,#t do
		local k=i~t[x]

		i=o~hi[k+1]
		o=lo[k+1]
	end

	if i==j and o==p then
		return true
	elseif o==j and i==p then
		return true
	end

	return false
end

-- end-添加用户自定义值或函数等   --
------------------------------------

------------------------------------------------------------------------------------------
-- 设置定时下发设备的数据（可选）                                                       --
-- @param  dev    user_data   设备管理器                                                --
-- @return 无                                                                           --
-- @notice 此函数为回调函数，不可在脚本内调用                                           --
-- @readme dev提供一下几个函数：                                                        --
--         dev:add(interval,name,data)添加定时下发数据                                  --
--           @param   interval   number   数据下发的时间间隔（秒）                      --
--                    name       string   名称（须保证唯一性）                          --
--                    data       string   数据（二进制数据），使用lua转义字符串         --
--           @return  成功返回true，否则返回false                                       --
--           @notice  定时数据下发的平均频率不超过1，及1/interval_1+...+1/interval_n<=1 --
--           @example local ok = dev:add(10,"test","\1\1\0\150\0\37\253\29")            --
--         dev:timeout(sec)设置下发数据的设备响应超时时间（秒）                         --
--           @param   sec        int      响应超时时间（秒）                            --
--                                        如果值为0，表示不检测设备响应超时             --
--           @return  无                                                                --
--           @example dev:timeout(3)                                                    --
--         dev:response()设备响应成功                                                   --
--           @param   无                                                                --
--           @return  无                                                                --
--           @example dev:response()                                                    --
--         dev:send(data)下发数据到设备                                                 --
--           @param   data   string   数据（二进制数据），使用lua转义字符串             --
--           @return  无                                                                --
--           @example dev:send("\2\2\0\150\0\37\206\89")                                --
------------------------------------------------------------------------------------------
function device_timer_init(dev)
	-- 添加用户自定义代码 --
	dev:timeout(0)
	dev:add(11,"dev0","\0\3\0\1\0\2\26\148")
	dev:add(11,"dev1","\1\3\0\1\0\2\203\149")
	dev:add(11,"dev2","\2\3\0\1\0\2\248\149")
end

-----------------------------------------------------------------------------------------------------------
-- 解析设备上传数据                                                                                      --
-- @param  dev    user_data   设备管理器                                                                 --
-- @return size表示已解析设备上传数据的字节数，json表示解析后的数据点集合，格式如下：                    --
--         [                                                                                             --
--           {                                                                                           --
--             "i" : "dsname1",          // 数据流或数据流模板名称1                                      --
--             "a" : 1234567890,         // 毫秒级时间戳，距离（00:00:00 UTC, January 1, 1970）的毫秒    --
--                                       // 如果值为0，表示使用当前时间                                  --
--             "v" : 123 | "123" | {...} // 布尔值、数值、字符串、json                                   --
--             "b" : "0A0B0C0D..."       // 二进制数据（16进制字符串），与v互斥，不同时存在              --
--             "d" : xxx | "xxx" | {...} // 用于描述b（可选）；布尔值、数值、字符串、json                --
--             "c" : "authcode1"         // 用于标识数据点归属(设备AuthCode,可选)                        --
--                                       // 如果为“”或不存在，表示数据点归属建立TCP连接的设备            --
--           }                                                                                           --
--           ...                                                                                         --
--           {                                                                                           --
--             "i" : "dsnamen",          // 数据流或数据流模板名称1                                      --
--             "a" : 1234567890,         // 毫秒级时间戳，距离（00:00:00 UTC, January 1, 1970）的毫秒    --
--                                       // 如果值为0，表示使用当前时间                                  --
--             "v" : 123 | "123" | {...} // 布尔值、数值、字符串、json                                   --
--             "b" : "0A0B0C0D..."       // 二进制数据（16进制字符串），与v互斥，不同时存在              --
--             "d" : xxx | "xxx" | {...} // 用于描述b（可选）；布尔值、数值、字符串、json                --
--             "c" : "authcoden"         // 用于标识数据点归属(设备AuthCode,可选)                        --
--                                       // 如果为“”或不存在，表示数据点归属建立TCP连接的设备            --
--           }                                                                                           --
--         ]                                                                                             --
-- @notice 此函数为回调函数，不可在脚本内调用                                                            --
-- @readme dev提供一下几个函数：                                                                         --
--         dev:add(interval,name,data)添加定时下发数据                                                   --
--           @param   interval number   数据下发的时间间隔（秒）                                         --
--                    name     string   名称（须保证唯一性）                                             --
--                    data     string   数据（二进制数据），使用lua转义字符串                            --
--           @return  成功返回true，否则返回false                                                        --
--           @notice  定时数据下发的平均频率不超过1，及1/interval_1+...+1/interval_n<=1                  --
--           @example local ok = dev:add(10,"test","\1\1\0\150\0\37\253\29")                             --
--         dev:timeout(sec)设置下发数据的设备响应超时时间（秒）                                          --
--           @param   sec      int      响应超时时间（秒）                                               --
--                                      如果值为0，表示不检测设备响应超时                                --
--           @return  无                                                                                 --
--           @example dev:timeout(3)                                                                     --
--         dev:response()设备响应成功                                                                    --
--           @param   无                                                                                 --
--           @return  无                                                                                 --
--           @example dev:response()                                                                     --
--         dev:send(data)下发数据到设备                                                                  --
--           @param   data   string   数据（二进制数据），使用lua转义字符串                              --
--           @return  无                                                                                 --
--           @example dev:send("\2\2\0\150\0\37\206\89")                                                 --
--         dev:size()获取设备数据大小（字节数）                                                          --
--           @param   无                                                                                 --
--           @return  返回设备数据大小（字节数）                                                         --
--           @example local sz = dev:size()                                                              --
--         dev:byte(pos)获取pos对应位置的设备数据（字节）                                                --
--           @param   pos   number   指定的获取位置，取值范围[1,dev:size()+1)                            --
--           @return  成功返回设备数据（int），否则返回nil                                               --
--           @example local data = dev:byte(1)                                                           --
--         dev:bytes(pos,count)获取从pos开始，count个设备数据                                            --
--           @param   pos   number   指定的获取起始位置，取值范围[1,dev:size()+1)                        --
--                    count number   指定的获取数据总数，取值范围[0,dev:size()+1-pos]                    --
--           @return  成功返回设备数据（string），否则返回nil                                            --
--           @example local datas = dev:bytes(1,dev:size())                                              --
-----------------------------------------------------------------------------------------------------------
function device_data_analyze(dev)
	local t={}
	local a=0

	-- 添加用户自定义代码 --
	local p=0
	local l=dev:size()
	local n={"dev1","dev2"}

	while l > 2 do
		local d=dev:byte(p+1)
		local c=dev:byte(p+2)
		if c~=1 and c~=2 and c~=3 and c~=4 then
			l=l-1
			p=p+1
		else
			local s=5+dev:byte(p+3)
			if s>l then
				break
			end

			local b=dev:bytes(p+1,s)
			if crc16(b) then
				if c==3 then
					local i=0
					local j={}
					local k={b:byte(4,-3)}
					for x=1,#k do
						if x&1==0 then
							i=(i<<8)+k[x]
							j[x>>1]=i
						else
							i=k[x]
						end
					end

					i=0
					k={}

					if d > 0 then
						-- A0 --
						add_val(t,"reg0",a,j[1],n[d])
						-- A1 --
						add_val(t,"reg1",a,j[2],n[d])
						-- A0+A1 --
						add_val(t,"add",a,j[1]+j[2],n[d])
						-- A0-A1 --
						add_val(t,"sub",a,j[1]-j[2],n[d])
						-- A0*A1 --
						add_val(t,"mul",a,j[1]*j[2],n[d])
						-- A0/A1 --
						add_val(t,"div",a,j[1]/j[2],n[d])
					else
						-- A0 --
						add_val(t,"reg0",a,j[1])
						-- A1 --
						add_val(t,"reg1",a,j[2])
						-- A0+A1 --
						add_val(t,"add",a,j[1]+j[2])
						-- A0-A1 --
						add_val(t,"sub",a,j[1]-j[2])
						-- A0*A1 --
						add_val(t,"mul",a,j[1]*j[2])
						-- A0/A1 --
						add_val(t,"div",a,j[1]/j[2])
					end
				end

				l=l-s
				p=p+s

				break
			else
				l=l-1
				p=p+1
			end
		end
	end
	dev:response()

	-- return $1,$2 --
	return p,to_json(t)
end