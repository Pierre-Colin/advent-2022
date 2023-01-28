if [ $# -lt 1 ]
then
	echo usage: $0 S
	echo
	echo "S is your session id (see README.md for more info)"
	exit 1
fi

COOKIESTR="Cookie: session=$1"
OPTS="-Z"
for d in {1..25}
do
	OPTS+=" -o input-$d https://adventofcode.com/2022/day/$d/input"
done
curl -H "$COOKIESTR" $OPTS
