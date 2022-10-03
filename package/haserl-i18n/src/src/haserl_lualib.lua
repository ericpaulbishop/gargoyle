-- --------------------------------------------------------------------------
-- haserl luascript library
-- $Id: haserl.c,v 1.32 2005/11/22 15:56:42 nangel Exp $
-- Copyright (c) 2003-2007    Nathan Angelacos (nangel@users.sourceforge.net)
--
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License, version 2,
-- as published by the Free Software Foundation.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
-- General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software
-- Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
--
-- --------------------------------------------------------------------------

haserl, FORM, ENV = {}, {}, {}

function haserl.setfield (f, v)
						-- From programming in Lua 1st Ed.
	local t = _G    			-- start with the table of globals
	for w, d in string.gfind(f, '([%w_%-]+)(.?)') do
		if (tonumber(w)) then
			w = tonumber(w)
		end
		if d == '.' then		-- not last field?
			t[w] = t[w] or {}	-- create table if absent
			t = t[w]		-- get the table
		else				-- last field
			t[w] = v		-- do the assignment
		end
	end
end

function haserl.getfield (f)
	local v = _G				-- start with the table of globals
	for w in string.gfind(f, '[%w_]+') do
		v = v[w]
	end
	return v
end

function haserl.myputenv(key, value) 
						-- convert key to dotted form
	key = string.gsub(key, '[\\]\\[]', '.' )
	key = string.gsub(key, '[\\.]+', '.' )
						-- and create a table if necessary
	haserl.setfield (key, value)
end

