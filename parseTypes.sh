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
# v1.1
# ----
# + Added a "--output" switch which enables the user to write the contents
#   of flashTypes.h to a Raincoat compatible config file.  This means the
#   user doesnt need to rebuild raincoat to add support for more flashes.
#
#   The syntax for this is './parseTypes.sh --output filename'.  If the
#   filename is omitted it defaults to "raincoat.conf".
#
#   If the output switch is used, no flash details will be printed to the
#   terminal - only status messages.
#
# + Added a silly twiddler.
# + Added a banner.
#
# v1.0
# ----
# + This script should print out:
#   ManID: 0x??
#   ProdID: 0x??
#   Name: Manufacturer - Model
#   Size: Flash size in KB
###

flashCount="0"
version="v1.1"
tiddles=("\\" "|" "/" "-")
tiddler="0"

echo "parseTypes ${version} - The Raincoat flash parser (c) Thomas \"ShALLaX\" Pedley"
echo "-----------------------------------------------------------------------"
echo ""

if [ ! -f "flashtypes.h" ]; then
	echo "Couldn't find 'flashtypes.h'."
	exit 1
fi

if [ ! -z $1 ]; then
	if [ `echo $1 | tr A-Z a-z` = "--output" ] || [ `echo $1 | tr A-Z a-z` = "-o" ]; then
		if [ ! -z $2 ]; then
			output=$2
		else
			output="raincoat.conf"
		fi

		if [ -e ${output} ]; then
			echo -n "'${output}' already exists!  Are you sure? (y/n)> "
			read -n 1 confirm
			echo -e "\n"
			if [ `echo ${confirm} | tr A-Z a-z` != "y" ]; then
				echo -e "Aborting..."
				exit 1
			fi
		fi

		touch ${output} 1> /dev/null 2> /dev/null
		if [ $? != 0 ]; then
			echo "Error writing to '${output}'."
			echo "Do you have write permission?"
			exit 1
		fi

		rm -rf ${output}

		echo "###" > ${output}
		echo "# Created by parseTypes (c) Thomas \"ShALLaX\" Pedley" >> ${output}
		echo "###" >> ${output}
		echo "" >> ${output}

		echo -n "Parsing..."
	fi
fi

found="unknown"
for i in `cat "flashtypes.h"`; do
	if [ $found == "name" ]; then
		if [ -z ${output} ]; then
			echo -n "Size:   "
		fi
		found="size"
		size=$i
	elif [ $found == "prodid" ]; then
		if [ -z ${output} ]; then
			echo -n "Name:   "
		fi
		found="name"
		name=`echo $i | sed "s/\"//g" | sed "s/\,//g"`
	elif [ $found == "manid" ]; then
		if [ -z ${output} ]; then
			echo -n "ProdID: "
		fi
		found="prodid"
		prodid=`echo $i | sed "s/0x//g" | sed "s/\,//g"`
	elif [ $found == "begin" ]; then
		if [ $i != "0," ]; then
			if [ -z ${output} ]; then
				echo -n "ManID:  "
			fi
			found="manid"
			manid=`echo $i | sed "s/0x//g" | sed "s/\,//g"`
		else
			found="unknown"
		fi
	elif [ `echo $i | awk '{ print $1 }'` == "{" ]; then
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
			if [ -z ${output} ]; then
				echo -e "$size\n"
			fi

			if [ ! -z ${output} ]; then
				flashCount=`expr ${flashCount} + 1`
				echo "Flash = 0x${manid}${prodid},\"${name}\",0x${hex}" >> ${output}
				echo -ne "${tiddles[${tiddler}]}"
				echo -ne "\b"
				tiddler=`expr $tiddler + 1`
				if [ $tiddler = 4 ]; then
					tiddler=0
				fi
			fi
		else
			if [ -z ${output} ]; then
				echo $i | sed "s/,//" | sed "s/\"//g"
			fi
		fi
	fi
done

if [ ! -z ${output} ]; then
	echo "Done."
	echo "${flashCount} flashes written to '${output}'."
fi