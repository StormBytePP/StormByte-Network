#! /bin/bash

if [ $# -eq 1 ]; then
	git pull --rebase &> /dev/null
	git push &> /dev/null # In case there is pending work
	git tag -a $1 -m "Version $1 released" &> /dev/null;
	git push origin $1 &> /dev/null;
	echo "Version $1 released and pushed";
else
	echo "Only 1 argument is expected and should be version string!";
fi
