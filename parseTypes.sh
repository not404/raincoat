#!/bin/bash
###
#  flashtypes.h parser
#  Copyright (C) Thomas "ShALLaX" Pedley (gentoox@shallax.com)
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
###

###
# Pretty horrible, but will be functional if people stick to the formatting
# of the flashtypes.h file
#
# This script should print out:
# ManID: 0x??
# ProdID: 0x??
# Name: Manufacturer - Model
# Size: Flash size in KB
###

found="unknown"
for i in `cat flashtypes.h`; do
	if [ $found == "name" ]; then
		echo -n "Size:   "
		found="size"
	elif [ $found == "prodid" ]; then
		echo -n "Name:   "
		found="name"
	elif [ $found == "manid" ]; then
		echo -n "ProdID: "
		found="prodid"
	elif [ $found == "begin" ]; then
		if [ $i != "0," ]; then
			echo -n "ManID:  "
			found="manid"
		else
			found="unknown"
		fi
	elif [ $i == "{" ]; then
	   found="begin";
	elif [ $found == "manid" ] && [ $i == "0," ]; then
		found="unknown"
	fi

	if [ $found != "unknown" ] && [ $i != "{" ] && [ $i != "}," ]; then
		if [ $found == "size" ]; then
			hex=`echo $i | sed "s/0x//"`
			
			size=`echo "ibase=16; $hex" | bc`
			size=`echo "$size/1024" | bc`
			size="${size}KB"
			echo -e "$size\n"
		else
			echo $i | sed "s/,//" | sed "s/\"//g"
		fi
	fi
done
