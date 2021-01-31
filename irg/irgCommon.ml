(*
 * GLISS V2 -- translator from SimNML action to C
 * Copyright (c) 2021, IRIT - UPS <casse@irit.fr>
 *
 * GLISS V2 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GLISS V2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GLISS V2; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *)

(** Module providing commonly used and convenient functions. *)

(** Test if the string s1 starts with the substring s2.
	@param s1	Container string.
	@param s2	Sub-string.
	@return		True if s1 starts with s2, false else. *)	
let starts_with s1 s2 =
	(String.length s1) >=  (String.length s2) && (String.sub s1 0 (String.length s2)) = s2


(** Return the substring of s starting at index n. If n is bigger than
	the length of s, return an empty string.
	@param s	String to get substring from.
	@param n	Index to extract substring from.
	@return		Substring of s starting at index n. *)	
let tail s n =
	let l = String.length s in
	if n >= l then "" else String.sub s n (l - n)
