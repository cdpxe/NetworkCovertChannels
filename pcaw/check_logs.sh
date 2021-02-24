#!/bin/bash

# - checks logfile for successful transfer (report SUCCESS/FAILURE)
# - execute this script in the logs/ directory.
# (c) 2011 Steffen Wendzel


for filename in `/bin/ls bitrate_*_delay_*_raw.log`; do
	cat $filename | awk '{if ($2!="") print $2}' > tmpfile_raw
	cat $filename | sort -bg | awk '{if ($2!="") print $2}' > tmpfile_result

	diff -up tmpfile_raw tmpfile_result >/dev/null
	if [ "$?" = "0" ]; then
		echo "${filename}: SUCCESS"
	else
		echo "${filename}: FAIL!"
	fi
done


